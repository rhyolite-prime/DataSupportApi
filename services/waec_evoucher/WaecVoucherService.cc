//
// Created by Antigravity on 22/07/2026.
//

#include "WaecVoucherService.h"
#include "models/BusinessAccounts.h"
#include "models/WaecMerchants.h"
#include "utils/IdGeneratorUtils.h"
#include <drogon/orm/CoroMapper.h>
#include <sstream>
#include <regex>
#include "bcrypt.h"
#include "MerchantCommissionWithdrawals.h"
#include "MerchantJournal.h"
#include "WaecVouchers.h"
#include "WaecVoucherSales.h"
#include "plugins/DataSupportServicePlugin.h"
#include "dto/SendEmailDto.h"

using namespace drogon::orm;

namespace data_support::services {

    drogon::Task<dto::BaseApiResponse> WaecVoucherService::getAll(const std::string &businessId, int pageNo, int pageSize, const std::string &voucherType, const std::string &query) {
        dto::BaseApiResponse res;
        co_return res;
    }

    drogon::Task<dto::BaseApiResponse> WaecVoucherService::importVouchers(const std::string &businessId, const dto::waec_voucher::WaecVoucherDto &dto) {

    dto::BaseApiResponse response;

    try {
        // --- 1. Validate input ---
        if (dto.getVoucherList().empty()) {
            response.message = "Voucher list cannot be empty.";
            response.success = false;
            co_return response;
        }

        if (dto.getRetailPrice() <= 0) {
            response.message = "Retail price must be greater than zero.";
            response.success = false;
            co_return response;
        }

        if (dto.getWholesalePrice() <= 0) {
            response.message = "Wholesale price must be greater than zero.";
            response.success = false;
            co_return response;
        }

        if (dto.getBatchId().empty()) {
            response.message = "Batch ID is required.";
            response.success = false;
            co_return response;
        }

        auto dbClient = drogon::app().getDbClient();
        drogon::orm::CoroMapper<drogon_model::DataSupportDb::WaecVouchers> voucherMapper(dbClient);

        // --- 2. Check if business exists ---
        drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> bizMapper(dbClient);
        try {
            auto business = co_await bizMapper.findByPrimaryKey(businessId);
        } catch (const drogon::orm::UnexpectedRows& e) {
            response.message = "Business account not found.";
            response.success = false;
            co_return response;
        }

        // --- 3. Validate batch ID uniqueness (optional - check if already used) ---
        auto batchCheckCriteria = drogon::orm::Criteria(drogon_model::DataSupportDb::WaecVouchers::Cols::_batch_id, drogon::orm::CompareOperator::EQ, dto.getBatchId());
        auto existingBatch = co_await voucherMapper.count(batchCheckCriteria);
        if (existingBatch > 0) {
            response.message = "Batch ID '" + dto.getBatchId() + "' already exists. Please use a unique batch ID.";
            response.success = false;
            co_return response;
        }

        for (const auto& voucherDto : dto.getVoucherList()) {

            drogon_model::DataSupportDb::WaecVouchers voucher;
            voucher.setBusinessId(businessId);
            voucher.setBatchId(dto.getBatchId());
            voucher.setSerialNumber(voucherDto.get("serialNumber", "").asString());
            voucher.setPin(voucherDto.get("pin", "").asString());
            voucher.setRetailPrice(std::to_string(dto.getRetailPrice()));
            voucher.setWholesalePrice(std::to_string(dto.getWholesalePrice()));
            voucher.setCreatedAt(trantor::Date::now());

            co_await voucherMapper.insert(voucher);
        }

    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "[importVouchers] Database error: " << e.base().what();
        response.message = "A database error occurred. Please try again.";
        response.success = false;
    } catch (const std::exception &e) {
        LOG_ERROR << "[importVouchers] Unexpected error: " << e.what();
        response.message = "An unexpected error occurred: " + std::string(e.what());
        response.success = false;
    }

    co_return response;
}


    drogon::Task<dto::BaseApiResponse> WaecVoucherService::initializeSingleVoucherSale(const std::string &businessId, const dto::waec_voucher::SingleWaecVoucherSaleDto &dto) {

        dto::BaseApiResponse response;

        try {

            auto dbClient = drogon::app().getDbClient();
            CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> bizMapper(dbClient);
            CoroMapper<drogon_model::DataSupportDb::WaecVouchers> waecVouchersMapper(dbClient);
            CoroMapper<drogon_model::DataSupportDb::WaecVoucherSales> waecVoucherSalesMapper(dbClient);
            auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);
            auto configStr = businessAccount.getValueOfConfigurationData();

            Json::Value configJson;
            Json::CharReaderBuilder readerBuilder;
            std::string errs;
            std::istringstream iss(configStr);

            if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
                LOG_ERROR << "Failed to parse ConfigurationData: " << errs;
                response.message = "Failed to initialize deposit.";
                response.success = false;
                co_return response;
            }

            Criteria searchCriteria = Criteria(drogon_model::DataSupportDb::WaecVouchers::Cols::_is_sold, CompareOperator::EQ, false) && drogon::orm::Criteria(drogon_model::DataSupportDb::WaecVouchers::Cols::_voucher_type, CompareOperator::EQ, dto.getVoucherType());

            auto unsoldWaecVoucher = co_await waecVouchersMapper.findOne(searchCriteria);


            //retrieve the default payment gateway for this business
            std::string defaultPaymentServiceProvider = configJson["DefaultPaymentServiceProvider"].asString();

            std::string clientRef = utils::IdGeneratorUtils::generateTimeStampReference();
            drogon_model::DataSupportDb::WaecVoucherSales voucherSale;
            voucherSale.setBusinessId(businessId);
            voucherSale.setPaymentReference(clientRef);
            voucherSale.setCommissionEarned("0");
            voucherSale.setVoucherId(unsoldWaecVoucher.getValueOfId());
            voucherSale.setMerchantId("");
            voucherSale.setBuyerPhone(dto.getBuyerPhoneNo());
            voucherSale.setBuyerEmail(dto.getBuyerEmail().empty() ? "" : dto.getBuyerPhoneNo()+"@rhyoliteprime.com");
            voucherSale.setStatus(constants::PaymentStatus::PENDING);
            voucherSale.setSaleType(constants::WaecVoucherSaleType::INDIVIDUAL);
            voucherSale.setAmountPaid(unsoldWaecVoucher.getValueOfRetailPrice());

            auto plugin = drogon::app().getPlugin<plugins::DataSupportServicePlugin>();
            auto &hubtelApi = plugin->getHubtelApi();
            auto &paystackApi = plugin->getPaystackApi();

            if (dto.getSource() == "web") {

                if (defaultPaymentServiceProvider == "hubtel") {

                    // initialize checkout with hubtel..
                    dto::HubtelWebCheckoutDto checkoutDto;
                    checkoutDto.phoneNo = dto.getBuyerPhoneNo();
                    checkoutDto.totalAmount = 0;
                    checkoutDto.description = "WAEC eVoucher";
                    checkoutDto.callbackUrl = "https://ultra-magnus.rhyoliteprime.com/api/v1/hubtel-callback-processor/waec-evoucher-sale/checkout/"+businessId+"/single";
                    checkoutDto.returnUrl = "https://wssd.rhyoliteprime.com";
                    checkoutDto.cancellationUrl = "https://wssd.rhyoliteprime.com";
                    checkoutDto.clientReference = clientRef;

                    auto hubtelResponse = co_await hubtelApi.initializeWebCheckout(businessId, checkoutDto);

                    response.message = hubtelResponse.message;
                    response.success = (hubtelResponse.code == 0 || hubtelResponse.code == 200 || hubtelResponse.code == 201);
                    response.result["checkoutUrl"] = hubtelResponse.checkoutUrl;

                    co_return response;

                }

                if (defaultPaymentServiceProvider == "pay-stack") {

                    dto::PaystackWebCheckoutDto checkoutDto;

                    checkoutDto.email = dto.getBuyerPhoneNo()+"@rhyoliteprime.com";
                    checkoutDto.amount = 0; // retrieve the cost of a voucher * 100;
                    checkoutDto.reference = clientRef;
                    checkoutDto.callbackUrl = "https://ultra-magnus.rhyoliteprime.com/api/v1/paystack-callback-processor/waec-evoucher-sale/checkout/"+businessId+"/single";

                    auto paystackResponse = co_await paystackApi.initializeWebCheckout(businessId,checkoutDto );

                    response.message = paystackResponse.message;
                    response.success = (paystackResponse.code == 0 || paystackResponse.code == 200 || paystackResponse.code == 201);
                    response.result["checkoutUrl"] = paystackResponse.checkoutUrl;

                    co_return response;
                }

            }

            // direct receive money
            dto::DirectReceiveMoneyDto receiveDto;
            receiveDto.setCustomerName("Customer_"+dto.getBuyerPhoneNo());
            receiveDto.setCustomerMsisdn(dto.getBuyerPhoneNo());
            receiveDto.setCustomerEmail(dto.getBuyerPhoneNo() + "@rhyoliteprime.com");
            receiveDto.setAmount(0);
            receiveDto.setPrimaryCallbackUrl("https://ultra-magnus.rhyoliteprime.com/api/v1/hubtel-callback-processor/waec-evoucher-sale/direct-receive-money/"+businessId+"/single");
            receiveDto.setDescription("WAEC eVoucher Purchase");
            receiveDto.setProvider(dto.getProvider());
            receiveDto.setClientReference(clientRef);
            receiveDto.setChannel("ussd-pos");

            auto hubtelResponse = co_await hubtelApi.initializeDirectReceiveMoney(businessId, receiveDto);

            response.message = hubtelResponse.message;
            response.success = (hubtelResponse.code == 0 || hubtelResponse.code == 200 || hubtelResponse.code == 201); // Based on standard success codes

            co_return response;
        }
        catch (const std::exception &e) {
            LOG_ERROR << "Error initializing web checkout deposit: " << e.what();
            response.message = "Failed to initialize web checkout deposit.";
            response.success = true;
            response.success = false;
            co_return response;
        }


    }


    drogon::Task<dto::BaseApiResponse> WaecVoucherService::completeSingleVoucherSale(const std::string &accountId, const std::string &paymentReference) {

        dto::BaseApiResponse response;

        auto dbClient = drogon::app().getDbClient();
        CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> bizMapper(dbClient);
        CoroMapper<drogon_model::DataSupportDb::WaecVoucherSales> waecVoucherSalesMapper(dbClient);

        Criteria bizSearchCriteria = Criteria(drogon_model::DataSupportDb::BusinessAccounts::Cols::_account_id, CompareOperator::EQ, accountId);

        auto businessAccount = co_await bizMapper.findOne(bizSearchCriteria);
        auto configStr = businessAccount.getValueOfConfigurationData();

        Json::Value configJson;
        Json::CharReaderBuilder readerBuilder;
        std::string errs;
        std::istringstream iss(configStr);

        if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
            LOG_ERROR << "Failed to parse ConfigurationData: " << errs;
            response.message = "Failed to initialize deposit.";
            response.success = false;
            co_return response;
        }

        Criteria weacVoucherSaleSearchCriteria = Criteria(drogon_model::DataSupportDb::WaecVoucherSales::Cols::_status, CompareOperator::EQ, static_cast<int>(constants::PaymentStatus::PENDING)) && Criteria(drogon_model::DataSupportDb::WaecVoucherSales::Cols::_payment_reference, CompareOperator::EQ, paymentReference);

        auto waecVoucherSale = co_await waecVoucherSalesMapper.findOne(weacVoucherSaleSearchCriteria);

        //retrieve the default payment gateway for this business...
        std::string defaultPaymentServiceProvider = configJson["DefaultPaymentServiceProvider"].asString();

        co_return response;
    }


    drogon::Task<dto::BaseApiResponse> WaecVoucherService::initializeBulkVoucherSale(const std::string &businessId, const dto::waec_voucher::BulkWaecVoucherSaleDto &dto) {
        dto::BaseApiResponse res;
        co_return res;
    }

    drogon::Task<dto::BaseApiResponse> WaecVoucherService::completeBulkVoucherSale(const std::string &accountId, const std::string &paymentReference) {
        dto::BaseApiResponse res;
        co_return res;
    }

    drogon::Task<dto::BaseApiResponse> WaecVoucherService::generateMerchantAffiliateSaleLink(const std::string &businessId, const std::string &merchantIdentifier, const std::string &pin) {
        dto::BaseApiResponse res;
        co_return res;
    }

    drogon::Task<dto::BaseApiResponse> WaecVoucherService::initializeAffiliateVoucherSale(const std::string &businessId, const dto::waec_voucher::AffiliateWaecVoucherSaleDto &dto) {
        dto::BaseApiResponse res;
        co_return res;
    }

    drogon::Task<dto::BaseApiResponse> WaecVoucherService::completeAffiliateVoucherSale(const std::string &accountId, const std::string &paymentReference) {

        dto::BaseApiResponse res;
        co_return res;

    }

    drogon::Task<dto::BaseApiResponse> WaecVoucherService::createMerchantAccount(const std::string &businessId, const dto::waec_voucher::WaecMerchantDto &dto) {

        dto::BaseApiResponse response;

        try {
            auto dbClient = drogon::app().getDbClient();

            // --- 1. Validate required fields ---
            if (dto.getMerchantName().empty() || dto.getPhoneNumber().empty() || dto.getPin().empty()) {
                response.message = "Merchant name, phone number, and PIN are required.";
                response.success = false;
                co_return response;
            }

            // --- 2. Load business account config to get the encryption key ---
            CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> bizMapper(dbClient);
            auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);


            // --- 3. Check for duplicate phone number within this business ---
            CoroMapper<drogon_model::DataSupportDb::WaecMerchants> merchantMapper(dbClient);

            auto phoneCriteria =
                Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_business_id, drogon::orm::CompareOperator::EQ, businessId) &&
                Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_phone_number, drogon::orm::CompareOperator::EQ, dto.getPhoneNumber());

            auto existingByPhone = co_await merchantMapper.findBy(phoneCriteria);
            if (!existingByPhone.empty()) {
                response.message = "A merchant account with this phone number already exists.";
                response.success = false;
                co_return response;
            }

            // --- 4. Check for duplicate email (if provided) ---
            if (!dto.getEmail().empty()) {
                auto emailCriteria = Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_business_id, CompareOperator::EQ, businessId) && Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_email, CompareOperator::EQ, dto.getEmail());

                auto existingByEmail = co_await merchantMapper.findBy(emailCriteria);

                if (!existingByEmail.empty()) {
                    response.message = "A merchant account with this email already exists.";
                    response.success = false;
                    co_return response;
                }
            }

            // --- 5. Build and persist the new merchant record ---
            drogon_model::DataSupportDb::WaecMerchants merchant;
            merchant.setId(utils::IdGeneratorUtils::generateRandomSixDigit());
            merchant.setBusinessId(businessId);
            merchant.setMerchantName(dto.getMerchantName());
            merchant.setPhoneNumber(dto.getPhoneNumber());
            merchant.setPinHash(bcrypt::generateHash(dto.getPin(), 8));

            if (!dto.getEmail().empty()) {
                merchant.setEmail(dto.getEmail());
            }

            auto savedMerchant = co_await merchantMapper.insert(merchant);

            // --- 6. Return success ---
            response.result = savedMerchant.toJson();
            response.message = "Merchant account created successfully.";
            response.success = true;

            LOG_INFO << "[createMerchantAccount] Created merchant: " << savedMerchant.getValueOfId()
                     << " for business: " << businessId;

        } catch (const drogon::orm::UnexpectedRows &e) {
            LOG_ERROR << "[createMerchantAccount] Business account not found for id=" << businessId
                      << ": " << e.what();
            response.message = "Business account not found.";
            response.success = false;
        } catch (const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << "[createMerchantAccount] Database error: " << e.base().what();
            response.message = "A database error occurred. Please try again.";
            response.success = false;
        } catch (const std::exception &e) {
            LOG_ERROR << "[createMerchantAccount] Unexpected error: " << e.what();
            response.message = "An unexpected error occurred. Please try again.";
            response.success = false;
        }

        co_return response;
    }


    drogon::Task<dto::BaseApiResponse> WaecVoucherService::initializeResetMerchantAccountPin(const std::string &businessId, const std::string &merchantIdentifier) {

        dto::BaseApiResponse response;

        try {
            // --- 1. Validate input ---
            if (merchantIdentifier.empty()) {
                response.message = "Merchant identifier is required.";
                response.success = false;
                co_return response;
            }

            auto dbClient = drogon::app().getDbClient();
            CoroMapper<drogon_model::DataSupportDb::WaecMerchants> merchantMapper(dbClient);

            // --- 2. Resolve merchant by phone or email ---
            static const std::regex phoneRegex(R"(^\+?[0-9][0-9\s\-]{6,}$)");
            const bool isPhone = std::regex_match(merchantIdentifier, phoneRegex);

            auto identifierCriteria = Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_business_id,
                                      CompareOperator::EQ, businessId) &&
                (isPhone ? Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_phone_number,
                                            CompareOperator::EQ, merchantIdentifier) : Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_email,
                                            CompareOperator::EQ, merchantIdentifier));

            auto results = co_await merchantMapper.findBy(identifierCriteria);

            if (results.empty()) {
                // Deliberately vague to avoid account enumeration
                response.message = "If an account exists for this identifier, an OTP has been sent.";
                response.success = true;
                co_return response;
            }

            auto merchant = results.front();

            // --- 3. Generate 6-digit OTP and store in Redis with 5-minute TTL ---
            const std::string otp = utils::IdGeneratorUtils::generateRandomSixDigit();
            const std::string redisKey = "waec_pin_reset:" + businessId + ":" + merchantIdentifier;
            constexpr int OTP_TTL_SECONDS = 300;

            // --- 4. Deliver OTP via the appropriate channel ---
            auto plugin = drogon::app().getPlugin<plugins::DataSupportServicePlugin>();
            auto &redis  = plugin->getRedisCacheManager();
            co_await redis.setValueWithTtl(redisKey, otp, OTP_TTL_SECONDS);

            const std::string otpMessage =
                "Your WAEC e-Voucher PIN reset OTP is: " + otp +
                ". This code expires in 5 minutes. Do not share it with anyone.";

            const std::string otpEmailBody =
                "<p>Your WAEC e-Voucher PIN reset OTP is: <strong>" + otp + "</strong></p>"
                "<p>This code expires in 5 minutes. Do not share it with anyone.</p>";


            if (!isPhone) {
                // --- 4a. Identifier is an email — use EmailApi ---
                dto::SendEmailDto emailDto;
                emailDto.setTo(merchantIdentifier);
                emailDto.setSubject("WAEC e-Voucher PIN Reset OTP");
                emailDto.setBody(otpEmailBody);

                auto emailResult = co_await plugin->getEmailApi().sendEmailAsync(businessId, emailDto);
                if (!emailResult.success) {
                    LOG_WARN << "[initializeResetMerchantAccountPin] Email delivery failed for "
                             << merchantIdentifier << ": " << emailResult.message;
                }

            } else {
                // --- 4b. Identifier is a phone — use HubtelApi SMS ---
                // Load per-business Hubtel SMS credentials from ConfigurationData
                auto dbClient = drogon::app().getDbClient();
                drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> bizMapper(dbClient);
                auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);

                Json::Value configJson;
                Json::CharReaderBuilder readerBuilder;
                std::string errs;
                std::istringstream iss(businessAccount.getValueOfConfigurationData());

                bool smsSent = false;
                if (Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
                    const auto &smsConfig = configJson["HubtelSmsApi"];
                    if (!smsConfig.isNull()) {
                        const std::string clientId     = smsConfig["ClientId"].asString();
                        const std::string clientSecret = smsConfig["ClientSecret"].asString();
                        const std::string senderId     = smsConfig["SenderId"].asString();

                        if (!clientId.empty() && !clientSecret.empty() && !senderId.empty()) {
                            smsSent = co_await plugin->getHubtelApi().sendSms(
                                merchantIdentifier, clientSecret, clientId, senderId, otpMessage);

                            if (!smsSent) {
                                LOG_WARN << "[initializeResetMerchantAccountPin] SMS delivery failed for "
                                         << merchantIdentifier;
                            }
                        } else {
                            LOG_WARN << "[initializeResetMerchantAccountPin] HubtelSmsApi credentials "
                                        "incomplete in business config for businessId=" << businessId;
                        }
                    } else {
                        LOG_WARN << "[initializeResetMerchantAccountPin] HubtelSmsApi config section "
                                    "missing for businessId=" << businessId;
                    }
                } else {
                    LOG_ERROR << "[initializeResetMerchantAccountPin] Failed to parse ConfigurationData: "
                              << errs;
                }

                // --- 4c. Fallback: also notify merchant's registered email if present ---
                const std::string &merchantEmail = merchant.getValueOfEmail();
                if (!merchantEmail.empty()) {
                    dto::SendEmailDto emailDto;
                    emailDto.setTo(merchantEmail);
                    emailDto.setSubject("WAEC e-Voucher PIN Reset OTP");
                    emailDto.setBody(otpEmailBody);

                    auto emailResult = co_await plugin->getEmailApi().sendEmailAsync(businessId, emailDto);
                    if (!emailResult.success) {
                        LOG_WARN << "[initializeResetMerchantAccountPin] Fallback email delivery "
                                    "failed for " << merchantEmail << ": " << emailResult.message;
                    }
                }
            }


            LOG_INFO << "[initializeResetMerchantAccountPin] OTP issued for merchant: "
                     << merchant.getValueOfId() << " in business: " << businessId;

            response.message = "If an account exists for this identifier, an OTP has been sent.";
            response.success = true;

        } catch (const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << "[initializeResetMerchantAccountPin] Database error: " << e.base().what();
            response.message = "A database error occurred. Please try again.";
            response.success = false;
        } catch (const std::exception &e) {
            LOG_ERROR << "[initializeResetMerchantAccountPin] Unexpected error: " << e.what();
            response.message = "An unexpected error occurred. Please try again.";
            response.success = false;
        }

        co_return response;
    }


    drogon::Task<dto::BaseApiResponse> WaecVoucherService::resetMerchantAccountPin(const std::string &businessId, const dto::waec_voucher::ResetMerchantPinDto &dto) {
        dto::BaseApiResponse response;

        try {
            // --- 1. Validate required fields ---
            if (dto.getMerchantIdentifier().empty() || dto.getOtpCode().empty() || dto.getNewPin().empty()) {
                response.message = "Merchant identifier, OTP code, and new PIN are required.";
                response.success = false;
                co_return response;
            }

            // --- 2. Retrieve and consume OTP from Redis ---
            const std::string redisKey = "waec_pin_reset:" + businessId + ":" + dto.getMerchantIdentifier();
            auto plugin = drogon::app().getPlugin<plugins::DataSupportServicePlugin>();
            auto &redis = plugin->getRedisCacheManager();

            // shouldRemove=true: atomically fetches and deletes, making OTP single-use
            const std::string storedOtp = co_await redis.getValue(redisKey, true);

            if (storedOtp.empty()) {
                response.message = "OTP has expired or is invalid. Please request a new one.";
                response.success = false;
                co_return response;
            }

            // --- 3. Validate OTP ---
            if (storedOtp != dto.getOtpCode()) {
                response.message = "Invalid OTP. Please check the code and try again.";
                response.success = false;
                co_return response;
            }

            // --- 4. Locate the merchant ---
            auto dbClient = drogon::app().getDbClient();
            drogon::orm::CoroMapper<drogon_model::DataSupportDb::WaecMerchants> merchantMapper(dbClient);

            static const std::regex phoneRegex(R"(^\+?[0-9][0-9\s\-]{6,}$)");
            const bool isPhone = std::regex_match(dto.getMerchantIdentifier(), phoneRegex);

            auto identifierCriteria =
                drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_business_id,
                                      drogon::orm::CompareOperator::EQ, businessId) &&
                (isPhone
                    ? drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_phone_number,
                                            drogon::orm::CompareOperator::EQ, dto.getMerchantIdentifier())
                    : drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_email,
                                            drogon::orm::CompareOperator::EQ, dto.getMerchantIdentifier()));

            auto results = co_await merchantMapper.findBy(identifierCriteria);
            if (results.empty()) {
                response.message = "Merchant account not found.";
                response.success = false;
                co_return response;
            }

            auto merchant = results.front();

            // --- 5. Hash the new PIN and persist ---
            merchant.setPinHash(bcrypt::generateHash(dto.getNewPin(), 8));
            co_await merchantMapper.update(merchant);

            response.message = "PIN reset successfully.";
            response.success = true;

            LOG_INFO << "[resetMerchantAccountPin] PIN reset for merchant: "
                     << merchant.getValueOfId() << " in business: " << businessId;

        } catch (const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << "[resetMerchantAccountPin] Database error: " << e.base().what();
            response.message = "A database error occurred. Please try again.";
            response.success = false;
        } catch (const std::exception &e) {
            LOG_ERROR << "[resetMerchantAccountPin] Unexpected error: " << e.what();
            response.message = "An unexpected error occurred. Please try again.";
            response.success = false;
        }

        co_return response;
    }


    drogon::Task<dto::BaseApiResponse> WaecVoucherService::changeMerchantAccountPin(const std::string &businessId, const dto::waec_voucher::ChangeMerchantPinDto &dto) {
        dto::BaseApiResponse response;

        try {
            // --- 1. Validate required fields ---
            if (dto.getMerchantIdentifier().empty() || dto.getPin().empty() || dto.getNewPin().empty()) {
                response.message = "Merchant identifier, current PIN, and new PIN are required.";
                response.success = false;
                co_return response;
            }

            if (dto.getPin() == dto.getNewPin()) {
                response.message = "New PIN must be different from the current PIN.";
                response.success = false;
                co_return response;
            }

            auto dbClient = drogon::app().getDbClient();
            drogon::orm::CoroMapper<drogon_model::DataSupportDb::WaecMerchants> merchantMapper(dbClient);

            // --- 2. Resolve merchant by phone number or email (regex: phone starts with digits) ---
            static const std::regex phoneRegex(R"(^\+?[0-9][0-9\s\-]{6,}$)");
            const bool isPhone = std::regex_match(dto.getMerchantIdentifier(), phoneRegex);

            drogon::orm::Criteria identifierCriteria =
                drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_business_id, drogon::orm::CompareOperator::EQ, businessId) && (isPhone ? drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_phone_number, drogon::orm::CompareOperator::EQ, dto.getMerchantIdentifier()) : drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_email,
                                            drogon::orm::CompareOperator::EQ, dto.getMerchantIdentifier()));

            auto results = co_await merchantMapper.findBy(identifierCriteria);

            if (results.empty()) {
                response.message = "Merchant account not found.";
                response.success = false;
                co_return response;
            }

            auto merchant = results.front();

            // --- 3. Verify the current PIN against the stored bcrypt hash ---
            if (!bcrypt::validatePassword(dto.getPin(), merchant.getValueOfPinHash())) {
                response.message = "Current PIN is incorrect.";
                response.success = false;
                co_return response;
            }

            // --- 4. Hash the new PIN and persist ---
            merchant.setPinHash(bcrypt::generateHash(dto.getNewPin(), 8));
            co_await merchantMapper.update(merchant);

            response.message = "PIN changed successfully.";
            response.success = true;

            LOG_INFO << "[changeMerchantAccountPin] Updated PIN for merchant: "
                     << merchant.getValueOfId() << " in business: " << businessId;

        } catch (const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << "[changeMerchantAccountPin] Database error: " << e.base().what();
            response.message = "A database error occurred. Please try again.";
            response.success = false;
        } catch (const std::exception &e) {
            LOG_ERROR << "[changeMerchantAccountPin] Unexpected error: " << e.what();
            response.message = "An unexpected error occurred. Please try again.";
            response.success = false;
        }

        co_return response;
    }


    drogon::Task<dto::BaseApiResponse> WaecVoucherService::updateMerchantAccountEmail(const std::string &businessId, const dto::waec_voucher::UpdateMerchantEmailDto &dto) {

     dto::BaseApiResponse response;

    try {
        // --- Validation ---
        if (dto.getMerchantIdentifier().empty() || dto.getPin().empty() || dto.getNewEmail().empty()) {
            response.message = "Merchant identifier, PIN, and new email are required.";
            response.success = false;
            co_return response;
        }

        // --- Validate email format ---
        static const std::regex emailRegex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
        if (!std::regex_match(dto.getNewEmail(), emailRegex)) {
            response.message = "Invalid email format.";
            response.success = false;
            co_return response;
        }

        auto dbClient = drogon::app().getDbClient();
        CoroMapper<drogon_model::DataSupportDb::WaecMerchants> merchantMapper(dbClient);

        // --- Find merchant ---
        static const std::regex phoneRegex(R"(^\+?[0-9][0-9\s\-]{6,}$)");
        const bool isPhone = std::regex_match(dto.getMerchantIdentifier(), phoneRegex);

        Criteria identifierCriteria =
            Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_business_id, drogon::orm::CompareOperator::EQ, businessId) &&
            (isPhone ?
                Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_phone_number, drogon::orm::CompareOperator::EQ, dto.getMerchantIdentifier()) :
                Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_email, drogon::orm::CompareOperator::EQ, dto.getMerchantIdentifier()));

        auto results = co_await merchantMapper.findBy(identifierCriteria);

        if (results.empty()) {
            response.message = "Merchant account not found.";
            response.success = false;
            co_return response;
        }

        auto merchant = results.front();

        // --- Verify PIN ---
        if (!bcrypt::validatePassword(dto.getPin(), merchant.getValueOfPinHash())) {
            response.message = "Invalid PIN.";
            response.success = false;
            co_return response;
        }

        // --- Check for duplicate email ---
        auto emailCriteria =
            Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_business_id,
                                  drogon::orm::CompareOperator::EQ, businessId) &&
            Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_email,
                                  drogon::orm::CompareOperator::EQ, dto.getNewEmail()) &&
            Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_id,
                                  drogon::orm::CompareOperator::NE, merchant.getValueOfId());

        auto existingByEmail = co_await merchantMapper.findBy(emailCriteria);
        if (!existingByEmail.empty()) {
            response.message = "Email already in use by another merchant.";
            response.success = false;
            co_return response;
        }

        // --- Update email ---
        std::string oldEmail = merchant.getValueOfEmail();
        merchant.setEmail(dto.getNewEmail());
        co_await merchantMapper.update(merchant);

        // --- Prepare response ---
        Json::Value result;
        result["merchantId"] = merchant.getValueOfId();
        result["email"] = dto.getNewEmail();
        result["oldEmail"] = oldEmail;

        response.result = result;
        response.message = "Email updated successfully.";
        response.success = true;

        LOG_INFO << "[updateMerchantAccountEmail] Updated email for merchant: "
                 << merchant.getValueOfId()
                 << " from: " << oldEmail
                 << " to: " << dto.getNewEmail();

    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "[updateMerchantAccountEmail] Database error: " << e.base().what();
        response.message = "Database error occurred.";
        response.success = false;
    } catch (const std::exception &e) {
        LOG_ERROR << "[updateMerchantAccountEmail] Error: " << e.what();
        response.message = "An error occurred.";
        response.success = false;
    }

    co_return response;


    }


    drogon::Task<dto::BaseApiResponse> WaecVoucherService::updateMerchantSettlementAccount(const std::string &businessId, const dto::waec_voucher::UpdateMerchantSettlementAccountDto &dto) {

        dto::BaseApiResponse response;

        try {
            // --- 1. Validate required fields ---
            if (dto.getMerchantIdentifier().empty() || dto.getPin().empty()) {
                response.message = "Merchant identifier and PIN are required.";
                response.success = false;
                co_return response;
            }

            if (dto.getAccountType().empty()) {
                response.message = "Account type is required.";
                response.success = false;
                co_return response;
            }

            // --- 2. Validate account type ---
            static const std::set<std::string> validAccountTypes = {
                "MOMO", "BANK"
            };

            if (validAccountTypes.find(dto.getAccountType()) == validAccountTypes.end()) {
                response.message = "Invalid account type. Must be MOMO, BANK, or WALLET.";
                response.success = false;
                co_return response;
            }

            // --- 3. Validate account provider ---
            if (dto.getAccountProvider().empty()) {
                response.message = "Account provider is required.";
                response.success = false;
                co_return response;
            }

            // --- 4. Validate bank account number for BANK type ---
            if (dto.getAccountType() == "BANK" && dto.getBankAccountNumber().empty()) {
                response.message = "Bank account number is required for BANK account type.";
                response.success = false;
                co_return response;
            }

            auto dbClient = drogon::app().getDbClient();
            drogon::orm::CoroMapper<drogon_model::DataSupportDb::WaecMerchants> merchantMapper(dbClient);

            // --- 5. Find merchant ---
            static const std::regex phoneRegex(R"(^\+?[0-9][0-9\s\-]{6,}$)");
            const bool isPhone = std::regex_match(dto.getMerchantIdentifier(), phoneRegex);

            drogon::orm::Criteria identifierCriteria =
                drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_business_id,
                                      drogon::orm::CompareOperator::EQ, businessId) &&
                (isPhone ?
                    drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_phone_number,
                                         drogon::orm::CompareOperator::EQ, dto.getMerchantIdentifier()) :
                    drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_email,
                                         drogon::orm::CompareOperator::EQ, dto.getMerchantIdentifier()));

            auto merchantResults = co_await merchantMapper.findBy(identifierCriteria);
            if (merchantResults.empty()) {
                response.message = "Merchant account not found.";
                response.success = false;
                co_return response;
            }

            auto merchant = merchantResults.front();

            // --- 6. Verify PIN ---
            if (!bcrypt::validatePassword(dto.getPin(), merchant.getValueOfPinHash())) {
                response.message = "Invalid PIN.";
                response.success = false;
                co_return response;
            }

            // --- 7. Check if merchant is active ---

            if (merchant.getValueOfStatus() != constants::StatusType::ACTIVE) {
                response.message = "Merchant account is not active. Please contact support.";
                response.success = false;
                co_return response;
            }

            // --- 8. Store old values for logging and response ---
            std::string oldAccountType =  merchant.getValueOfAccountType();
            std::string oldAccountProvider = merchant.getValueOfAccountProvider();
            std::string oldBankAccountNumber = merchant.getValueOfBankAccountNumber();

            // --- 9. Update settlement account details ---
            merchant.setAccountType(dto.getAccountType());
            merchant.setAccountProvider(dto.getAccountProvider());

            // If account type is BANK, set the bank account number, otherwise clear it
            if (dto.getAccountType() == "BANK") {
                merchant.setBankAccountNumber(dto.getBankAccountNumber());
            } else {
                // For MOMO or WALLET, clear the bank account number
                merchant.setBankAccountNumber("");
            }

            // --- 10. Set updated_at timestamp ---
            merchant.setUpdatedAt(trantor::Date::now());

            // --- 11. Save changes ---
            co_await merchantMapper.update(merchant);

            // --- 12. Prepare response ---
            Json::Value result;
            result["merchantId"] = merchant.getValueOfId();
            result["merchantName"] = merchant.getValueOfMerchantName();
            result["phoneNumber"] = merchant.getValueOfPhoneNumber();
            result["email"] = merchant.getValueOfEmail();
            result["accountType"] = merchant.getValueOfAccountType();
            result["accountProvider"] = merchant.getValueOfAccountProvider();
            result["bankAccountNumber"] = merchant.getValueOfBankAccountNumber();

            // Include old values for reference
            result["oldAccountType"] = oldAccountType;
            result["oldAccountProvider"] = oldAccountProvider;
            result["oldBankAccountNumber"] = oldBankAccountNumber;

            result["updatedAt"] = merchant.getValueOfUpdatedAt().toDbStringLocal();

            response.result = result;
            response.message = "Settlement account updated successfully.";
            response.success = true;

            LOG_INFO << "[updateMerchantSettlementAccount] Updated settlement account for merchant: "
                     << merchant.getValueOfId()
                     << " in business: " << businessId
                     << " from type: " << oldAccountType
                     << " to: " << dto.getAccountType()
                     << " provider: " << dto.getAccountProvider();

            // --- 13. Optional: Send confirmation email/SMS ---
            // You might want to send a notification to the merchant
            // about the account update for security purposes

        } catch (const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << "[updateMerchantSettlementAccount] Database error: " << e.base().what();
            response.message = "A database error occurred. Please try again.";
            response.success = false;
        } catch (const std::exception &e) {
            LOG_ERROR << "[updateMerchantSettlementAccount] Unexpected error: " << e.what();
            response.message = "An unexpected error occurred. Please try again.";
            response.success = false;
        }

        co_return response;
   }


    drogon::Task<dto::BaseApiResponse> WaecVoucherService::getMerchantSummaryStats(const std::string &businessId, const dto::waec_voucher::WaecMerchantAuthDto &dto) {

    dto::BaseApiResponse response;

    try {
        // --- 1. Validate required fields ---
        if (dto.getMerchantIdentifier().empty() || dto.getPin().empty()) {
            response.message = "Merchant identifier and PIN are required.";
            response.success = false;
            co_return response;
        }

        auto dbClient = drogon::app().getDbClient();
        drogon::orm::CoroMapper<drogon_model::DataSupportDb::WaecMerchants> merchantMapper(dbClient);
        drogon::orm::CoroMapper<drogon_model::DataSupportDb::WaecVoucherSales> salesMapper(dbClient);

        // --- 2. Find merchant ---
        static const std::regex phoneRegex(R"(^\+?[0-9][0-9\s\-]{6,}$)");
        const bool isPhone = std::regex_match(dto.getMerchantIdentifier(), phoneRegex);

        drogon::orm::Criteria identifierCriteria =
            drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_business_id,
                                  drogon::orm::CompareOperator::EQ, businessId) &&
            (isPhone ?
                drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_phone_number,
                                     drogon::orm::CompareOperator::EQ, dto.getMerchantIdentifier()) :
                drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_email,
                                     drogon::orm::CompareOperator::EQ, dto.getMerchantIdentifier()));

        auto merchantResults = co_await merchantMapper.findBy(identifierCriteria);
        if (merchantResults.empty()) {
            response.message = "Merchant account not found.";
            response.success = false;
            co_return response;
        }

        auto merchant = merchantResults.front();

        // --- 3. Verify PIN ---
        if (!bcrypt::validatePassword(dto.getPin(), merchant.getValueOfPinHash())) {
            response.message = "Invalid PIN.";
            response.success = false;
            co_return response;
        }

        // Use auto to prevent compilation errors if ID is generated as an integer type
        auto merchantId = merchant.getValueOfId();

        // --- 4. Get ALL time-based statistics in ONE query using Postgres FILTER ---
        std::string statsQuery = R"(
            SELECT
                COUNT(*) as lifetime_count,
                COALESCE(SUM(amount_paid::numeric), 0) as lifetime_amount,
                COALESCE(SUM(commission_earned::numeric), 0) as lifetime_comm,

                COUNT(*) FILTER (WHERE created_at >= date_trunc('day', CURRENT_TIMESTAMP)) as today_count,
                COALESCE(SUM(amount_paid::numeric) FILTER (WHERE created_at >= date_trunc('day', CURRENT_TIMESTAMP)), 0) as today_amount,
                COALESCE(SUM(commission_earned::numeric) FILTER (WHERE created_at >= date_trunc('day', CURRENT_TIMESTAMP)), 0) as today_comm,

                COUNT(*) FILTER (WHERE created_at >= date_trunc('month', CURRENT_TIMESTAMP)) as month_count,
                COALESCE(SUM(amount_paid::numeric) FILTER (WHERE created_at >= date_trunc('month', CURRENT_TIMESTAMP)), 0) as month_amount,
                COALESCE(SUM(commission_earned::numeric) FILTER (WHERE created_at >= date_trunc('month', CURRENT_TIMESTAMP)), 0) as month_comm,

                COUNT(*) FILTER (WHERE created_at >= date_trunc('year', CURRENT_TIMESTAMP)) as year_count,
                COALESCE(SUM(amount_paid::numeric) FILTER (WHERE created_at >= date_trunc('year', CURRENT_TIMESTAMP)), 0) as year_amount,
                COALESCE(SUM(commission_earned::numeric) FILTER (WHERE created_at >= date_trunc('year', CURRENT_TIMESTAMP)), 0) as year_comm
            FROM waec_voucher_sales
            WHERE merchant_id = $1
        )";

        auto statsResult = co_await dbClient->execSqlCoro(statsQuery, merchantId);

        // --- Helper lambda for safe generic parsing of numeric strings from Drogon ---
        auto parseDouble = [](const auto& val) -> double {
            if constexpr (std::is_arithmetic_v<std::decay_t<decltype(val)>>) {
                return static_cast<double>(val);
            } else {
                try { return std::stod(val); } catch(...) { return 0.0; }
            }
        };

        // --- 5. Get available balance safely using Drogon's pointer getters ---
        double availableBalance = 0.0;
        auto balancePtr = merchant.getTotalCommissionEarned();
        if (balancePtr) {
            availableBalance = parseDouble(*balancePtr);
        }

        // --- 6. Get recent transactions (last 5) ---
        auto recentSales = co_await salesMapper.orderBy(
            drogon_model::DataSupportDb::WaecVoucherSales::Cols::_created_at,
            drogon::orm::SortOrder::DESC
        ).limit(5).findBy(
            drogon::orm::Criteria(drogon_model::DataSupportDb::WaecVoucherSales::Cols::_merchant_id,
                                  drogon::orm::CompareOperator::EQ, merchantId)
        );

        Json::Value recentTransactions(Json::arrayValue);
        for (const auto& sale : recentSales) {
            Json::Value tx;
            tx["id"] = sale.getValueOfId();
            tx["amount"] = parseDouble(sale.getValueOfAmountPaid());
            tx["commission"] = parseDouble(sale.getValueOfCommissionEarned());
            tx["buyerPhone"] = sale.getValueOfBuyerPhone();
            tx["buyerEmail"] = sale.getValueOfBuyerEmail();
            tx["createdAt"] = sale.getValueOfCreatedAt().toDbStringLocal();
            recentTransactions.append(tx);
        }

        // --- 7. Build response ---
        Json::Value result;

        // Merchant Info
        Json::Value merchantInfo;
        merchantInfo["id"] = merchant.getValueOfId();
        merchantInfo["name"] = merchant.getValueOfMerchantName();
        merchantInfo["phoneNumber"] = merchant.getValueOfPhoneNumber();
        merchantInfo["email"] = merchant.getValueOfEmail();
        merchantInfo["status"] = merchant.getValueOfStatus();
        merchantInfo["joinedAt"] = merchant.getValueOfCreatedAt().toDbStringLocal();
        result["merchant"] = merchantInfo;

        // Populate Statistics safely from the single DB row
        Json::Value stats;
        if (!statsResult.empty()) {
            auto row = statsResult[0];

            stats["today"]["transactions"] = row["today_count"].as<int32_t>();
            stats["today"]["amount"]       = row["today_amount"].as<double>();
            stats["today"]["commission"]   = row["today_comm"].as<double>();

            stats["thisMonth"]["transactions"] = row["month_count"].as<int32_t>();
            stats["thisMonth"]["amount"]       = row["month_amount"].as<double>();
            stats["thisMonth"]["commission"]   = row["month_comm"].as<double>();

            stats["thisYear"]["transactions"] = row["year_count"].as<int32_t>();
            stats["thisYear"]["amount"]       = row["year_amount"].as<double>();
            stats["thisYear"]["commission"]   = row["year_comm"].as<double>();

            stats["lifetime"]["transactions"] = row["lifetime_count"].as<int32_t>();
            stats["lifetime"]["amount"]       = row["lifetime_amount"].as<double>();
            stats["lifetime"]["commission"]   = row["lifetime_comm"].as<double>();

            // Balance Summary
            Json::Value balance;
            balance["availableBalance"] = availableBalance;
            balance["totalCommissionEarned"] = row["lifetime_comm"].as<double>();
            balance["totalSalesAmount"] = row["lifetime_amount"].as<double>();
            balance["totalTransactions"] = row["lifetime_count"].as<int32_t>();
            result["balance"] = balance;
        }

        result["statistics"] = stats;
        result["recentTransactions"] = recentTransactions;

        // Account Information (Safely handling null pointers without Try-Catch)
        Json::Value accountInfo;

        auto typePtr = merchant.getAccountType();
        accountInfo["accountType"] = (typePtr && !typePtr->empty()) ? *typePtr : "MOMO";

        auto providerPtr = merchant.getAccountProvider();
        accountInfo["accountProvider"] = providerPtr ? *providerPtr : "";

        auto accNumPtr = merchant.getBankAccountNumber();
        accountInfo["bankAccountNumber"] = accNumPtr ? *accNumPtr : "";

        result["settlementAccount"] = accountInfo;

        // --- 8. Set response ---
        response.result = result;
        response.message = "Merchant summary statistics retrieved successfully.";
        response.success = true;

        // Check if ID is a string or cast to string for the log if it's an integer
        LOG_INFO << "[getMerchantSummaryStats] Retrieved summary stats for merchant in business: " << businessId;

    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "[getMerchantSummaryStats] Database error: " << e.base().what();
        response.message = "A database error occurred. Please try again.";
        response.success = false;
    } catch (const std::exception &e) {
        LOG_ERROR << "[getMerchantSummaryStats] Unexpected error: " << e.what();
        response.message = "An unexpected error occurred. Please try again.";
        response.success = false;
    }

    co_return response;
}


    drogon::Task<dto::BaseApiResponse> WaecVoucherService::getMerchantTransactions(const std::string &businessId, const dto::waec_voucher::MerchantTransactionsRequestDto &dto) {

    dto::BaseApiResponse response;

    try {
        // --- 1. Validate required fields ---
        if (dto.getMerchantIdentifier().empty() || dto.getPin().empty()) {
            response.message = "Merchant identifier and PIN are required.";
            response.success = false;
            co_return response;
        }

        auto dbClient = drogon::app().getDbClient();
        drogon::orm::CoroMapper<drogon_model::DataSupportDb::WaecMerchants> merchantMapper(dbClient);
        drogon::orm::CoroMapper<drogon_model::DataSupportDb::WaecVoucherSales> salesMapper(dbClient);

        // --- 2. Find merchant ---
        static const std::regex phoneRegex(R"(^\+?[0-9][0-9\s\-]{6,}$)");
        const bool isPhone = std::regex_match(dto.getMerchantIdentifier(), phoneRegex);

        drogon::orm::Criteria identifierCriteria =
            drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_business_id,
                                  drogon::orm::CompareOperator::EQ, businessId) &&
            (isPhone ?
                drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_phone_number,
                                     drogon::orm::CompareOperator::EQ, dto.getMerchantIdentifier()) :
                drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_email,
                                     drogon::orm::CompareOperator::EQ, dto.getMerchantIdentifier()));

        auto merchantResults = co_await merchantMapper.findBy(identifierCriteria);
        if (merchantResults.empty()) {
            response.message = "Merchant account not found.";
            response.success = false;
            co_return response;
        }

        auto merchant = merchantResults.front();

        // --- 3. Verify PIN ---
        if (!bcrypt::validatePassword(dto.getPin(), merchant.getValueOfPinHash())) {
            response.message = "Invalid PIN.";
            response.success = false;
            co_return response;
        }

        // --- 4. Build query criteria for sales ---
        drogon::orm::Criteria salesCriteria =
            drogon::orm::Criteria(drogon_model::DataSupportDb::WaecVoucherSales::Cols::_merchant_id,
                                  drogon::orm::CompareOperator::EQ, merchant.getValueOfId());

        // Add date range if provided
        salesCriteria = salesCriteria &&
                drogon::orm::Criteria(drogon_model::DataSupportDb::WaecVoucherSales::Cols::_created_at,
                                      drogon::orm::CompareOperator::GE, dto.getStartDate()) &&
                drogon::orm::Criteria(drogon_model::DataSupportDb::WaecVoucherSales::Cols::_created_at,
                                      drogon::orm::CompareOperator::LE, dto.getEndDate());

        // --- 5. Get total count for pagination ---
        auto countResult = co_await salesMapper.count(salesCriteria);
        int64_t totalRecords = countResult;

        auto salesResults =  co_await salesMapper.paginate(dto.getPageNo(), dto.getPageSize()).orderBy(drogon_model::DataSupportDb::WaecVoucherSales::Cols::_created_at, drogon::orm::SortOrder::DESC).findBy(salesCriteria);

        // --- 7. Build response as JSON directly ---
        Json::Value result;
        Json::Value transactions(Json::arrayValue);

        double totalAmountPaid = 0.0;
        double totalCommissionEarned = 0.0;

        for (const auto& sale : salesResults) {
            Json::Value tx;
            tx["id"] = sale.getValueOfId();

            tx["voucherId"] = sale.getValueOfVoucherId();

            tx["merchantId"] = sale.getValueOfMerchantId();

            tx["buyerPhone"] = sale.getValueOfBuyerPhone();

            tx["buyerEmail"] = sale.getValueOfBuyerEmail();

            tx["saleType"] = sale.getValueOfSaleType();

            double amount = std::stod(sale.getValueOfAmountPaid());
            tx["amountPaid"] = amount;
            totalAmountPaid += amount;

            double commission = std::stod(sale.getValueOfCommissionEarned());
            tx["commissionEarned"] = commission;
            totalCommissionEarned += commission;

            tx["paymentReference"] = sale.getValueOfPaymentReference();

            tx["createdAt"] = sale.getValueOfCreatedAt().toDbStringLocal();

            transactions.append(tx);
        }

        result["transactions"] = transactions;
        result["summary"]["totalAmountPaid"] = totalAmountPaid;
        result["summary"]["totalCommissionEarned"] = totalCommissionEarned;
        result["summary"]["totalTransactions"] = static_cast<int32_t>(salesResults.size());
        result["pageNo"] = dto.getPageNo();
        result["pageSize"] = dto.getPageSize();
        result["totalRecords"] = totalRecords;
        result["pagination"]["totalPages"] = (totalRecords + dto.getPageSize() - 1) / dto.getPageSize();

        response.result = result;
        response.message = "Transactions retrieved successfully.";
        response.success = true;

        LOG_INFO << "[getMerchantTransactions] Retrieved " << salesResults.size()
                 << " transactions for merchant: " << merchant.getValueOfId();

    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "[getMerchantTransactions] Database error: " << e.base().what();
        response.message = "A database error occurred. Please try again.";
        response.success = false;
    } catch (const std::exception &e) {
        LOG_ERROR << "[getMerchantTransactions] Unexpected error: " << e.what();
        response.message = "An unexpected error occurred. Please try again.";
        response.success = false;
    }

    co_return response;
}


    drogon::Task<dto::BaseApiResponse> WaecVoucherService::getMerchantStatement(
    const std::string &businessId,
    const dto::waec_voucher::MerchantStatementRequestDto &dto) {

    dto::BaseApiResponse response;

    try {
        // --- 1. Validate required fields ---
        if (dto.getMerchantIdentifier().empty() || dto.getPin().empty()) {
            response.message = "Merchant identifier and PIN are required.";
            response.success = false;
            co_return response;
        }


        if (dto.getStartDate() > dto.getEndDate()) {
            response.message = "Start date must be before end date.";
            response.success = false;
            co_return response;
        }

        auto dbClient = drogon::app().getDbClient();
        drogon::orm::CoroMapper<drogon_model::DataSupportDb::WaecMerchants> merchantMapper(dbClient);
        drogon::orm::CoroMapper<drogon_model::DataSupportDb::WaecVoucherSales> salesMapper(dbClient);

        // --- 3. Find merchant ---
        static const std::regex phoneRegex(R"(^\+?[0-9][0-9\s\-]{6,}$)");
        const bool isPhone = std::regex_match(dto.getMerchantIdentifier(), phoneRegex);

        drogon::orm::Criteria identifierCriteria =
            drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_business_id,
                                  drogon::orm::CompareOperator::EQ, businessId) &&
            (isPhone ?
                drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_phone_number,
                                     drogon::orm::CompareOperator::EQ, dto.getMerchantIdentifier()) :
                drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_email,
                                     drogon::orm::CompareOperator::EQ, dto.getMerchantIdentifier()));

        auto merchantResults = co_await merchantMapper.findBy(identifierCriteria);
        if (merchantResults.empty()) {
            response.message = "Merchant account not found.";
            response.success = false;
            co_return response;
        }

        auto merchant = merchantResults.front();

        // --- 4. Verify PIN ---
        if (!bcrypt::validatePassword(dto.getPin(), merchant.getValueOfPinHash())) {
            response.message = "Invalid PIN.";
            response.success = false;
            co_return response;
        }

        // --- 5. Build query criteria for sales ---
        drogon::orm::Criteria salesCriteria =
            drogon::orm::Criteria(drogon_model::DataSupportDb::WaecVoucherSales::Cols::_merchant_id,
                                  drogon::orm::CompareOperator::EQ, merchant.getValueOfId()) &&
            drogon::orm::Criteria(drogon_model::DataSupportDb::WaecVoucherSales::Cols::_created_at,
                                  drogon::orm::CompareOperator::GE, dto.getStartDate()) &&
            drogon::orm::Criteria(drogon_model::DataSupportDb::WaecVoucherSales::Cols::_created_at,
                                  drogon::orm::CompareOperator::LE, dto.getEndDate());

        // --- 6. Get all sales for the period (no pagination for statement) ---
        auto salesResults = co_await salesMapper.orderBy(drogon_model::DataSupportDb::WaecVoucherSales::Cols::_created_at, drogon::orm::SortOrder::DESC).findBy(salesCriteria);


        // --- 7. Build statement response as JSON directly ---
        Json::Value result;
        Json::Value transactions(Json::arrayValue);

        double totalSales = 0.0;
        double totalCommission = 0.0;
        double runningBalance = 0.0;

        // Add header with merchant info
        Json::Value header;
        header["merchantId"] = merchant.getValueOfId();
        header["merchantName"] = merchant.getValueOfMerchantName();
        header["phoneNumber"] = merchant.getValueOfPhoneNumber();
        header["email"] = merchant.getValueOfEmail();
        header["statementPeriod"]["startDate"] = dto.getStartDate().toDbStringLocal();
        header["statementPeriod"]["endDate"] = dto.getEndDate().toDbStringLocal();
        header["openingBalance"] = 0.0; // You can calculate this from previous period if needed
        result["header"] = header;

        // Process transactions
        for (const auto& sale : salesResults) {
            Json::Value tx;

            // Add all fields (matching the transaction response format)
            tx["id"] = sale.getValueOfId();
            tx["voucherId"] = sale.getValueOfVoucherId();
            tx["merchantId"] = sale.getValueOfMerchantId();
            tx["buyerPhone"] = sale.getValueOfBuyerPhone();
            tx["buyerEmail"] = sale.getValueOfBuyerEmail();
            tx["saleType"] = sale.getValueOfSaleType();

            double amount = std::stod(sale.getValueOfAmountPaid());
            tx["amountPaid"] = amount;
            totalSales += amount;

            double commission = std::stod(sale.getValueOfCommissionEarned());
            tx["commissionEarned"] = commission;
            totalCommission += commission;

            tx["paymentReference"] = sale.getValueOfPaymentReference();
            tx["createdAt"] = sale.getValueOfCreatedAt().toDbStringLocal();

            // Calculate running balance
            // Assuming amount paid increases balance (adjust based on your business logic)
            runningBalance += amount;
            tx["runningBalance"] = runningBalance;

            transactions.append(tx);
        }

        result["transactions"] = transactions;

        // Summary
        Json::Value summary;
        summary["totalSales"] = totalSales;
        summary["totalCommissionEarned"] = totalCommission;
        summary["totalTransactions"] = static_cast<int32_t>(salesResults.size());
        summary["closingBalance"] = runningBalance;

        if (!salesResults.empty()) {
            auto firstSale = salesResults.front();
            auto lastSale = salesResults.back();
            summary["firstTransactionDate"] = firstSale.getValueOfCreatedAt().toDbStringLocal();
            summary["lastTransactionDate"] = lastSale.getValueOfCreatedAt().toDbStringLocal();
        }

        result["summary"] = summary;

        response.result = result;
        response.message = "Merchant statement generated successfully.";
        response.success = true;

        LOG_INFO << "[getMerchantStatement] Generated statement for merchant: "
                 << merchant.getValueOfId()
                 << " from " << dto.getStartDate().toDbStringLocal()
                 << " to " << dto.getEndDate().toDbStringLocal()
                 << " with " << salesResults.size() << " transactions";

    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "[getMerchantStatement] Database error: " << e.base().what();
        response.message = "A database error occurred. Please try again.";
        response.success = false;
    } catch (const std::exception &e) {
        LOG_ERROR << "[getMerchantStatement] Unexpected error: " << e.what();
        response.message = "An unexpected error occurred. Please try again.";
        response.success = false;
    }

    co_return response;
}


    drogon::Task<dto::BaseApiResponse> WaecVoucherService::initializeMerchantCommissionWithdrawal(const std::string &businessId, const dto::waec_voucher::CashoutMerchantCommissionDto &dto) {
        dto::BaseApiResponse response;


        // --- 1. Validate required fields ---
        if (dto.getMerchantIdentifier().empty() || dto.getPin().empty()) {
            response.message = "Merchant identifier, PIN are required.";
            response.success = false;
            co_return response;
        }


        auto dbClient = drogon::app().getDbClient();
        drogon::orm::CoroMapper<drogon_model::DataSupportDb::WaecMerchants> merchantMapper(dbClient);

        // --- 2. Resolve merchant by phone number or email (regex: phone starts with digits) ---
        static const std::regex phoneRegex(R"(^\+?[0-9][0-9\s\-]{6,}$)");
        const bool isPhone = std::regex_match(dto.getMerchantIdentifier(), phoneRegex);

        Criteria identifierCriteria = Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_business_id, drogon::orm::CompareOperator::EQ, businessId) && (isPhone ? drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_phone_number, drogon::orm::CompareOperator::EQ, dto.getMerchantIdentifier()) : drogon::orm::Criteria(drogon_model::DataSupportDb::WaecMerchants::Cols::_email, CompareOperator::EQ, dto.getMerchantIdentifier()));

        auto results = co_await merchantMapper.findBy(identifierCriteria);

        if (results.empty()) {
            response.message = "Merchant account not found.";
            response.success = false;
            co_return response;
        }

        auto merchant = results.front();

        // --- Verify PIN ---
        if (!bcrypt::validatePassword(dto.getPin(), merchant.getValueOfPinHash())) {
            response.message = "Invalid PIN.";
            response.success = false;
            co_return response;
        }

        // --- 4. Check merchant status ---
        if (merchant.getValueOfStatus() != constants::StatusType::ACTIVE) {
            response.message = "Merchant account is not active. Please contact support.";
            response.success = false;
            co_return response;
        }

        // --- 5. Check available balance ---
        double availableBalance = 0.0;
        auto balancePtr = merchant.getCommissionBalance();
        if (balancePtr) {
            try {
                availableBalance = std::stod(*balancePtr);
            } catch (...) {
                availableBalance = 0.0;
            }
        }

        if (availableBalance < dto.getAmount()) {
            response.message = "Insufficient balance. Available: " + std::to_string(availableBalance) +
                               ", Requested: " + std::to_string(dto.getAmount());
            response.success = false;
            co_return response;
        }

        CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> bizMapper(dbClient);
        CoroMapper<drogon_model::DataSupportDb::MerchantCommissionWithdrawals> mcwMapper(dbClient);
        CoroMapper<drogon_model::DataSupportDb::MerchantJournal> journalMapper(dbClient);

        auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);

        std::string clientRef = utils::IdGeneratorUtils::generateTimeStampReference();
        //write a commission withdrawal attempt in MerchantCommissionWithdrawals table

        drogon_model::DataSupportDb::MerchantCommissionWithdrawals merchantCommissionWithdrawal;
        merchantCommissionWithdrawal.setBusinessId(businessId);
        merchantCommissionWithdrawal.setMerchantId(merchant.getValueOfId());
        merchantCommissionWithdrawal.setAmount(std::to_string(dto.getAmount()));
        merchantCommissionWithdrawal.setDestinationAccount(merchant.getValueOfPhoneNumber());
        merchantCommissionWithdrawal.setAccountType(merchant.getValueOfAccountType());
        merchantCommissionWithdrawal.setProvider(merchant.getValueOfAccountProvider());
        merchantCommissionWithdrawal.setStatus(constants::PaymentStatus::PENDING);
        merchantCommissionWithdrawal.setFailureReason("");
        merchantCommissionWithdrawal.setExternalReference(clientRef);

        auto savedWithdrawal = co_await mcwMapper.insert(merchantCommissionWithdrawal);

        //write to journal
        double currentBalance = availableBalance; // merchant's current balance before deduction
        double balanceAfter = currentBalance - dto.getAmount();

        drogon_model::DataSupportDb::MerchantJournal journal;
        journal.setBusinessId(businessId);
        journal.setMerchantId(merchant.getValueOfId());
        journal.setTransactionType(constants::TransactionType::WITHDRAWAL);
        journal.setAmount(std::to_string(dto.getAmount()));
        journal.setBalanceBefore(std::to_string(currentBalance));
        journal.setBalanceAfter(std::to_string(balanceAfter));
        journal.setDescription("Commission withdrawal - " + clientRef);
        co_await journalMapper.insert(journal);

        auto plugin = drogon::app().getPlugin<plugins::DataSupportServicePlugin>();
        auto &hubtelApi = plugin->getHubtelApi();

        dto::DirectSendMomoDto directSendMomoDto;
        directSendMomoDto.setRecipientName(merchant.getValueOfMerchantName());
        directSendMomoDto.setRecipientMsisdn(merchant.getValueOfPhoneNumber());
        directSendMomoDto.setCustomerEmail(merchant.getValueOfEmail());
        directSendMomoDto.setAmount(dto.getAmount());
        directSendMomoDto.setPrimaryCallbackUrl("https://ultra-magnus.rhyoliteprime.com/api/v1/hubtel-callback-processor/waec-evoucher-commissions/direct-send-mobile-money/"+ businessAccount.getValueOfAccountId());
        directSendMomoDto.setDescription("Merchant Commission" + merchant.getValueOfPhoneNumber());
        directSendMomoDto.setProvider(merchant.getValueOfAccountProvider());
        directSendMomoDto.setClientReference(clientRef);

        auto hubtelResponse = co_await hubtelApi.initializeDirectSendToMomo(businessId, directSendMomoDto);

        bool hubtelSuccess = (hubtelResponse.code == 0 || hubtelResponse.code == 200 || hubtelResponse.code == 201);

        if (hubtelSuccess) {
            LOG_INFO << "[initializeMerchantCommissionWithdrawalToMomo] Step 11: Hubtel DirectSendToMomo SUCCEEDED for clientRef=[" << clientRef << "]. Withdrawal initiated.";

            savedWithdrawal.setStatus(constants::PaymentStatus::PROCESSING);
            co_await mcwMapper.update(savedWithdrawal);

            response.success = true;
            response.message = "Withdrawal to MoMo has been initiated successfully.\nYou will be notified once the process is complete.";
        } else {
            LOG_ERROR << "[initializeMerchantCommissionWithdrawalToMomo] Step 11: Hubtel DirectSendToMomo FAILED for clientRef=[" << clientRef
                      << "] (code=[" << hubtelResponse.code << "] message=[" << hubtelResponse.message
                      << "]). Triggering Reversal reversal.";

            // --- 10a. Update withdrawal status to FAILED ---
            savedWithdrawal.setStatus(constants::PaymentStatus::FAILED);
            savedWithdrawal.setFailureReason("Hubtel error: " + hubtelResponse.message);
            co_await mcwMapper.update(savedWithdrawal);

            // --- 10b. Reverse the journal entry ---
            // We inserted a journal with balanceAfter = currentBalance - amount.
            // Now we insert a reversal journal that adds the amount back.
            double balanceBeforeReversal = balanceAfter; // current balance after the (failed) deduction
            double balanceAfterReversal = currentBalance; // restore to original

            drogon_model::DataSupportDb::MerchantJournal reversalJournal;
            reversalJournal.setBusinessId(businessId);
            reversalJournal.setMerchantId(merchant.getValueOfId());
            reversalJournal.setTransactionType(constants::TransactionType::REVERSAL); // must be defined
            reversalJournal.setAmount(std::to_string(dto.getAmount()));
            reversalJournal.setBalanceBefore(std::to_string(balanceBeforeReversal));
            reversalJournal.setBalanceAfter(std::to_string(balanceAfterReversal));
            reversalJournal.setDescription("Reversal of failed withdrawal (Hubtel error) - " + clientRef);
            co_await journalMapper.insert(reversalJournal);

            LOG_INFO << "[initializeMerchantCommissionWithdrawalToMomo] Step 11: Flexy reversal dispatched for clientRef=[" << clientRef << "]";

            response.success = false;
            response.message = "Failed to initiate withdrawal. Please try again later.";
        }

        co_return response;
    }

    drogon::Task<dto::BaseApiResponse> WaecVoucherService::completeMerchantCommissionWithdrawal(const std::string &businessId, const std::string &reference) {
        dto::BaseApiResponse res;
        co_return res;
    }

    drogon::Task<dto::BaseApiResponse> WaecVoucherService::getAvailableVoucherCount(const std::string &businessId) {
        dto::BaseApiResponse res;
        co_return res;
    }





}
