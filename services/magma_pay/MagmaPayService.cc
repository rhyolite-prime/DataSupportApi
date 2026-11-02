//
// MagmaNet — unified payment gateway facade.
//

#include "MagmaPayService.h"

#include <drogon/utils/Utilities.h>

#include <algorithm>
#include <sstream>

#include "services/magma_net/MagmaSettings.h"
#include "services/magma_net/MagmaSupport.h"
#include "utils/DbUtils.h"

using namespace drogon;
using namespace data_support::utils;

namespace data_support::services {

    namespace {
        std::string authHeader(const std::string &username, const std::string &password) {
            const std::string raw = username + ":" + password;
            return "Basic " + drogon::utils::base64Encode(
                                 reinterpret_cast<const unsigned char *>(raw.data()), raw.size());
        }

        bool statusIsPaid(const std::string &value) {
            const std::string status = lowerCopy(trimCopy(value));
            return status == "paid" || status == "success" || status == "successful" ||
                   status == "completed" || status == "approved";
        }

        bool statusIsPending(const std::string &value) {
            const std::string status = lowerCopy(trimCopy(value));
            return status == "pending" || status == "processing" || status == "initiated" ||
                   status == "awaiting_payment" || status.empty();
        }
    }

    std::string MagmaPayService::normaliseChannel(const std::string &paymentMethod) {
        const std::string method = lowerCopy(paymentMethod);
        if (method.find("mtn") != std::string::npos) {
            return "mtn-gh";
        }
        if (method.find("telecel") != std::string::npos ||
            method.find("vodafone") != std::string::npos) {
            return "vodafone-gh"; // Telecel Ghana's Hubtel channel id
        }
        if (method.find("airteltigo") != std::string::npos ||
            method.find("tigo") != std::string::npos) {
            return "tigo-gh";
        }
        return "mtn-gh";
    }

    std::string MagmaPayService::providerFor(const std::string &paymentMethod) {
        return lowerCopy(paymentMethod) == "card" ? "PAYSTACK" : "HUBTEL";
    }

    Task<PaymentInitiation> MagmaPayService::initiateMomo(const std::string &businessId,
                                                          const std::string &msisdn,
                                                          const std::string &customerName,
                                                          const std::string &paymentMethod,
                                                          double amount,
                                                          const std::string &internalReference,
                                                          const std::string &description,
                                                          const std::string &callbackUrl) {
        PaymentInitiation result;
        result.internalReference = internalReference;
        result.amount = amount;
        result.provider = "HUBTEL";

        if (MagmaSettings::instance().sandboxPayments) {
            result.success = true;
            result.provider = "SANDBOX";
            result.providerReference = "SBX-" + internalReference;
            result.instruction = "Sandbox payment auto-approved";
            result.message = "Sandbox mode: payment approved without contacting Hubtel";
            co_return result;
        }

        try {
            auto config = co_await magma::businessConfig(businessId);
            const auto &hubtel = config["HubtelPaymentApi"];
            if (hubtel.isNull()) {
                result.message = "Hubtel is not configured for this business";
                co_return result;
            }

            const std::string username = hubtel.get("ApiIdOrUsername", "").asString();
            const std::string password = hubtel.get("ApiKeyOrPassword", "").asString();
            const std::string accountId = hubtel.get("PosSalesAccountId", "").asString();
            if (username.empty() || password.empty() || accountId.empty()) {
                result.message = "Hubtel credentials are incomplete";
                co_return result;
            }

            Json::Value payload;
            payload["CustomerName"] = customerName.empty() ? "MagmaNet Customer" : customerName;
            payload["CustomerMsisdn"] = msisdn;
            payload["CustomerEmail"] = "customer@magmanet.gh";
            payload["Channel"] = normaliseChannel(paymentMethod);
            payload["Amount"] = amount;
            payload["PrimaryCallbackUrl"] = callbackUrl;
            payload["Description"] = description;
            payload["ClientReference"] = internalReference;

            auto client = HttpClient::newHttpClient("https://rmp.hubtel.com");
            auto req = HttpRequest::newHttpRequest();
            req->setMethod(Post);
            req->setPath("/merchantaccount/merchants/" + accountId + "/receive/mobilemoney");
            req->setContentTypeCode(CT_APPLICATION_JSON);
            req->setBody(toJsonString(payload));
            req->addHeader("Authorization", authHeader(username, password));

            auto response = co_await client->sendRequestCoro(req, 20.0);
            if (response == nullptr) {
                result.message = "No response from Hubtel";
                co_return result;
            }

            result.providerReference = internalReference;
            bool parsed = false;
            const auto body = parseJson(std::string(response->getBody()), &parsed);
            result.message = body.isObject() && body.isMember("Message")
                                 ? jsonString(body, "Message")
                                 : std::string(response->getBody()).substr(0, 300);

            if (response->getStatusCode() == k200OK || response->getStatusCode() == k201Created) {
                result.success = true;
                result.instruction =
                    "Approve the payment prompt on your phone. If no prompt appears, dial *170# "
                    "and choose Approve Payment.";
                if (body.isObject() && body.isMember("Data") && body["Data"].isObject()) {
                    if (body["Data"].isMember("TransactionId")) {
                        result.providerReference = jsonString(body["Data"], "TransactionId");
                    }
                }
            }
            co_return result;
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaPay] initiateMomo error: " << e.what();
            result.message = "Unable to start the mobile money payment";
            co_return result;
        }
    }

    Task<PaymentInitiation> MagmaPayService::initiateCard(const std::string &businessId,
                                                          const std::string &email,
                                                          double amount,
                                                          const std::string &internalReference,
                                                          const std::string &callbackUrl) {
        PaymentInitiation result;
        result.internalReference = internalReference;
        result.amount = amount;
        result.provider = "PAYSTACK";

        if (MagmaSettings::instance().sandboxPayments) {
            result.success = true;
            result.provider = "SANDBOX";
            result.providerReference = "SBX-" + internalReference;
            result.instruction = "Sandbox payment auto-approved";
            result.message = "Sandbox mode: payment approved without contacting Paystack";
            co_return result;
        }

        try {
            auto config = co_await magma::businessConfig(businessId);
            const auto &paystack = config["PaystackApi"];
            if (paystack.isNull() || paystack.get("ApiKey", "").asString().empty()) {
                result.message = "Paystack is not configured for this business";
                co_return result;
            }
            const std::string apiKey = paystack.get("ApiKey", "").asString();

            Json::Value payload;
            payload["amount"] = static_cast<int64_t>(amount * 100); // pesewas
            payload["email"] = email.empty() ? "customer@magmanet.gh" : email;
            payload["currency"] = "GHS";
            payload["reference"] = internalReference;
            payload["callback_url"] = callbackUrl;

            auto client = HttpClient::newHttpClient("https://api.paystack.co");
            auto req = HttpRequest::newHttpRequest();
            req->setMethod(Post);
            req->setPath("/transaction/initialize");
            req->setContentTypeCode(CT_APPLICATION_JSON);
            req->setBody(toJsonString(payload));
            req->addHeader("Authorization", "Bearer " + apiKey);

            auto response = co_await client->sendRequestCoro(req, 20.0);
            if (response == nullptr) {
                result.message = "No response from Paystack";
                co_return result;
            }

            bool parsed = false;
            const auto body = parseJson(std::string(response->getBody()), &parsed);
            if (response->getStatusCode() == k200OK && body.isObject() &&
                body["status"].asBool() && body["data"].isObject()) {
                result.success = true;
                result.providerReference = body["data"]["reference"].asString();
                result.targetUrl = body["data"]["authorization_url"].asString();
                result.instruction = "Complete the card payment in your browser";
                result.message = "Card checkout initialised";
            } else {
                result.message = body.isObject() && body.isMember("message")
                                     ? body["message"].asString()
                                     : "Paystack rejected the request";
            }
            co_return result;
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaPay] initiateCard error: " << e.what();
            result.message = "Unable to start the card payment";
            co_return result;
        }
    }

    Task<PaymentVerification> MagmaPayService::verify(const std::string &businessId,
                                                      const std::string &provider,
                                                      const std::string &providerReference,
                                                      const std::string &internalReference) {
        PaymentVerification result;
        result.providerReference = providerReference;

        if (MagmaSettings::instance().sandboxPayments ||
            lowerCopy(provider) == "sandbox") {
            result.paid = true;
            result.status = "PAID";
            result.message = "Sandbox payment";
            co_return result;
        }

        try {
            auto config = co_await magma::businessConfig(businessId);
            const std::string providerKey = lowerCopy(provider);

            if (providerKey == "paystack") {
                const auto &paystack = config["PaystackApi"];
                if (paystack.isNull()) {
                    result.message = "Paystack is not configured";
                    co_return result;
                }
                const std::string reference =
                    providerReference.empty() ? internalReference : providerReference;
                auto client = HttpClient::newHttpClient("https://api.paystack.co");
                auto req = HttpRequest::newHttpRequest();
                req->setMethod(Get);
                req->setPath("/transaction/verify/" + reference);
                req->addHeader("Authorization", "Bearer " + paystack.get("ApiKey", "").asString());

                auto response = co_await client->sendRequestCoro(req, 20.0);
                if (response == nullptr) {
                    result.message = "No response from Paystack";
                    co_return result;
                }
                const auto body = parseJson(std::string(response->getBody()));
                result.raw = body;
                if (body.isObject() && body["data"].isObject()) {
                    const auto &data = body["data"];
                    result.status = jsonString(data, "status");
                    result.amount = jsonDouble(data, "amount", 0.0) / 100.0;
                    result.paid = lowerCopy(result.status) == "success";
                    result.pending = !result.paid;
                }
                co_return result;
            }

            // Default: Hubtel
            const auto &hubtel = config["HubtelPaymentApi"];
            if (hubtel.isNull()) {
                result.message = "Hubtel is not configured";
                co_return result;
            }
            const std::string accountId = hubtel.get("PosSalesAccountId", "").asString();
            auto client = HttpClient::newHttpClient("https://api-txnstatus.hubtel.com");
            auto req = HttpRequest::newHttpRequest();
            req->setMethod(Get);
            req->setPath("/transactions/" + accountId + "/status");
            req->setParameter("clientReference",
                              providerReference.empty() ? internalReference : providerReference);
            req->addHeader("Authorization",
                           authHeader(hubtel.get("ApiIdOrUsername", "").asString(),
                                      hubtel.get("ApiKeyOrPassword", "").asString()));

            auto response = co_await client->sendRequestCoro(req, 20.0);
            if (response == nullptr) {
                result.message = "No response from Hubtel";
                co_return result;
            }
            const auto body = parseJson(std::string(response->getBody()));
            result.raw = body;
            if (body.isObject() && body["Data"].isObject()) {
                const auto &data = body["Data"];
                result.status = jsonString(data, "TransactionStatus");
                result.amount = jsonDouble(data, "Amount", 0.0);
                result.providerReference = jsonString(data, "TransactionId");
                result.paid = statusIsPaid(result.status);
                result.pending = statusIsPending(result.status);
            } else {
                result.status = jsonString(body, "Status");
                result.paid = statusIsPaid(result.status);
                result.pending = statusIsPending(result.status);
            }
            co_return result;
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaPay] verify error: " << e.what();
            result.message = "Unable to verify the payment";
            co_return result;
        }
    }

    Task<PaymentInitiation> MagmaPayService::payoutMomo(const std::string &businessId,
                                                        const std::string &msisdn,
                                                        const std::string &recipientName,
                                                        double amount,
                                                        const std::string &internalReference,
                                                        const std::string &description,
                                                        const std::string &callbackUrl,
                                                        const std::string &paymentMethod) {
        PaymentInitiation result;
        result.internalReference = internalReference;
        result.amount = amount;
        result.provider = "HUBTEL";

        if (MagmaSettings::instance().sandboxPayments) {
            result.success = true;
            result.provider = "SANDBOX";
            result.providerReference = "SBX-PAYOUT-" + internalReference;
            result.message = "Sandbox payout recorded";
            co_return result;
        }

        try {
            auto config = co_await magma::businessConfig(businessId);
            const auto &hubtel = config["HubtelPaymentApi"];
            if (hubtel.isNull()) {
                result.message = "Hubtel is not configured";
                co_return result;
            }
            const std::string username = hubtel.get("ApiIdOrUsername", "").asString();
            const std::string password = hubtel.get("ApiKeyOrPassword", "").asString();
            const std::string accountId = hubtel.get("PosSalesAccountId", "").asString();

            Json::Value payload;
            payload["RecipientName"] = recipientName.empty() ? "MagmaNet Agent" : recipientName;
            payload["RecipientMsisdn"] = msisdn;
            payload["CustomerEmail"] = "agent@magmanet.gh";
            payload["Channel"] = normaliseChannel(paymentMethod);
            payload["Amount"] = amount;
            payload["PrimaryCallbackUrl"] = callbackUrl;
            payload["Description"] = description;
            payload["ClientReference"] = internalReference;

            auto client = HttpClient::newHttpClient("https://rmp.hubtel.com");
            auto req = HttpRequest::newHttpRequest();
            req->setMethod(Post);
            req->setPath("/merchantaccount/merchants/" + accountId + "/send/mobilemoney");
            req->setContentTypeCode(CT_APPLICATION_JSON);
            req->setBody(toJsonString(payload));
            req->addHeader("Authorization", authHeader(username, password));

            auto response = co_await client->sendRequestCoro(req, 20.0);
            if (response == nullptr) {
                result.message = "No response from Hubtel";
                co_return result;
            }
            result.providerReference = internalReference;
            result.success = response->getStatusCode() == k200OK ||
                             response->getStatusCode() == k201Created;
            result.message = result.success ? "Payout submitted" : "Payout rejected by Hubtel";
            co_return result;
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaPay] payoutMomo error: " << e.what();
            result.message = "Unable to submit payout";
            co_return result;
        }
    }

} // namespace data_support::services
