//
// Created by Antigravity on 21/07/2026.
//

#ifndef DATASUPPORTAPI_WAECMERCHANTAUTHDTO_H
#define DATASUPPORTAPI_WAECMERCHANTAUTHDTO_H

#include <string>

namespace data_support::dto::waec_voucher {

    class WaecMerchantAuthDto {
    public:

        WaecMerchantAuthDto() = default;
        explicit WaecMerchantAuthDto(const Json::Value& json);

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const std::string& getMerchantIdentifier() const { return merchantIdentifier; }
        [[nodiscard]] const std::string& getPin() const { return pin; }


        // Setters
        void setMerchantIdentifier(const std::string& value) { merchantIdentifier = value; }
        void setPin(const std::string& value) { pin = value; }

    private:
        std::string merchantIdentifier;
        std::string pin;


    };

    inline void WaecMerchantAuthDto::fromJson(const Json::Value& json) {

        if (json.isMember("merchantIdentifier") && !json["merchantIdentifier"].isNull()) {
            setMerchantIdentifier(json["merchantIdentifier"].asString());
        }

        if (json.isMember("pin") && !json["pin"].isNull()) {
            setPin(json["pin"].asString());
        }

    }

    class WaecMerchantDto {
    public:

        WaecMerchantDto() = default;
        explicit WaecMerchantDto(const Json::Value& json);

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const std::string& getMerchantName() const { return merchantName_; }
        [[nodiscard]] const std::string& getPhoneNumber() const { return phoneNumber_; }
        [[nodiscard]] const std::string& getEmail() const { return email_; }
        [[nodiscard]] const std::string& getPin() const { return pin_; }


        // Setters
        void setMerchantName(const std::string& value) { merchantName_ = value; }
        void setPhoneNumber(const std::string& value) { phoneNumber_ = value; }
        void setEmail(const std::string& value) { email_ = value; }
        void setPin(const std::string& value) { pin_ = value; }

    private:
        std::string merchantName_;
        std::string phoneNumber_;
        std::string email_;
        std::string pin_;


    };

    inline void WaecMerchantDto::fromJson(const Json::Value& json) {

        if (json.isMember("merchantName") && !json["merchantName"].isNull()) {
            setMerchantName(json["merchantName"].asString());
        }

        if (json.isMember("phoneNumber") && !json["phoneNumber"].isNull()) {
            setPhoneNumber(json["phoneNumber"].asString());
        }

        if (json.isMember("email") && !json["email"].isNull()) {
            setEmail(json["email"].asString());
        }

        if (json.isMember("pin") && !json["pin"].isNull()) {
            setPin(json["pin"].asString());
        }

    }

    class MerchantTransactionsRequestDto {
        public:

        MerchantTransactionsRequestDto() = default;
        explicit MerchantTransactionsRequestDto(const Json::Value& json);

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const trantor::Date& getStartDate() const { return startDate_; }
        [[nodiscard]] const trantor::Date& getEndDate() const { return endDate_; }
        [[nodiscard]] int32_t getPageNo() const { return pageNo_; }
        [[nodiscard]] int32_t getPageSize() const { return pageSize_; }
        [[nodiscard]] const std::string& getMerchantIdentifier() const { return merchantIdentifier_; }
        [[nodiscard]] const std::string& getPin() const { return pin_; }

        // Setters
        void setStartDate(const trantor::Date& value) { startDate_ = value; }
        void setEndDate(const trantor::Date& value) { endDate_ = value; }
        void setPageNo(const int32_t value) { pageNo_ = value; }
        void setPageSize(const int32_t value) { pageSize_ = value; }
        void setMerchantIdentifier(const std::string& value) { merchantIdentifier_ = value; }
        void setPin(const std::string& value) { pin_ = value; }

    private:
        trantor::Date startDate_;
        trantor::Date endDate_;
        int32_t pageNo_ {1};
        int32_t pageSize_ {20};
        std::string merchantIdentifier_;
        std::string pin_;

    };

    inline void MerchantTransactionsRequestDto::fromJson(const Json::Value& json) {

        if (json.isMember("startDate") && !json["startDate"].isNull()) {
            std::string startDateStr = json["startDate"].asString();

            // Replace 'T' with space and add seconds
            std::replace(startDateStr.begin(), startDateStr.end(), 'T', ' ');
            startDateStr += ":00"; // -> 2025-12-31 06:10:00

            setStartDate(trantor::Date::fromDbString(startDateStr));
        }

        if (json.isMember("endDate") && !json["endDate"].isNull()) {
            std::string endDateStr = json["endDate"].asString();

            // Replace 'T' with space and add seconds
            std::replace(endDateStr.begin(), endDateStr.end(), 'T', ' ');
            endDateStr += ":00"; // -> 2025-12-31 06:10:00

            setEndDate(trantor::Date::fromDbString(endDateStr));
        }

        if (json.isMember("merchantIdentifier") && !json["merchantIdentifier"].isNull()) {
            setMerchantIdentifier(json["merchantIdentifier"].asString());
        }

        if (json.isMember("pin") && !json["pin"].isNull()) {
            setPin(json["pin"].asString());
        }

        if (json.isMember("pageNo") && !json["pageNo"].isNull()) {
            setPageNo(json["pageNo"].asInt());
        }

        if (json.isMember("pageSize") && !json["pageSize"].isNull()) {
            setPageSize(json["pageSize"].asInt());
        }

    }


    class MerchantStatementRequestDto {
        public:

        MerchantStatementRequestDto() = default;
        explicit MerchantStatementRequestDto(const Json::Value& json);

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const trantor::Date& getStartDate() const { return startDate_; }
        [[nodiscard]] const trantor::Date& getEndDate() const { return endDate_; }
        [[nodiscard]] const std::string& getMerchantIdentifier() const { return merchantIdentifier_; }
        [[nodiscard]] const std::string& getPin() const { return pin_; }

        // Setters
        void setStartDate(const trantor::Date& value) { startDate_ = value; }
        void setEndDate(const trantor::Date& value) { endDate_ = value; }
        void setMerchantIdentifier(const std::string& value) { merchantIdentifier_ = value; }
        void setPin(const std::string& value) { pin_ = value; }

    private:
        trantor::Date startDate_;
        trantor::Date endDate_;
        std::string merchantIdentifier_;
        std::string pin_;

    };

    inline void MerchantStatementRequestDto::fromJson(const Json::Value& json) {

        if (json.isMember("startDate") && !json["startDate"].isNull()) {
            std::string startDateStr = json["startDate"].asString();

            // Replace 'T' with space and add seconds
            std::replace(startDateStr.begin(), startDateStr.end(), 'T', ' ');
            startDateStr += ":00"; // -> 2025-12-31 06:10:00

            setStartDate(trantor::Date::fromDbString(startDateStr));
        }

        if (json.isMember("endDate") && !json["endDate"].isNull()) {
            std::string endDateStr = json["endDate"].asString();

            // Replace 'T' with space and add seconds
            std::replace(endDateStr.begin(), endDateStr.end(), 'T', ' ');
            endDateStr += ":00"; // -> 2025-12-31 06:10:00

            setEndDate(trantor::Date::fromDbString(endDateStr));
        }

        if (json.isMember("merchantIdentifier") && !json["merchantIdentifier"].isNull()) {
            setMerchantIdentifier(json["merchantIdentifier"].asString());
        }


        if (json.isMember("pin") && !json["pin"].isNull()) {
            setPin(json["pin"].asString());
        }

    }

    class CashoutMerchantCommissionDto {
        public:

        CashoutMerchantCommissionDto() = default;
        explicit CashoutMerchantCommissionDto(const Json::Value& json);

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] double getAmount() const { return amount_; }
        [[nodiscard]] const std::string& getMerchantIdentifier() const { return merchantIdentifier_; }
        [[nodiscard]] const std::string& getPin() const { return pin_; }

        // Setters
        void setAmount(const double value) { amount_ = value; }
        void setMerchantIdentifier(const std::string& value) { merchantIdentifier_ = value; }
        void setPin(const std::string& value) { pin_ = value; }

    private:
        double amount_ {5};
        std::string merchantIdentifier_;
        std::string pin_;

    };

    inline void CashoutMerchantCommissionDto::fromJson(const Json::Value& json) {

        if (json.isMember("merchantIdentifier") && !json["merchantIdentifier"].isNull()) {
            setMerchantIdentifier(json["merchantIdentifier"].asString());
        }

        if (json.isMember("pin") && !json["pin"].isNull()) {
            setPin(json["pin"].asString());
        }

        if (json.isMember("amount") && !json["pin"].isNull()) {
            setAmount(json["pin"].asDouble());
        }

    }

    class WaecVoucherDto {
    public:

        WaecVoucherDto() = default;
        explicit WaecVoucherDto(const Json::Value& json);

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const std::string& getBatchId() const { return batchId; }
        [[nodiscard]] double getRetailPrice() const { return retailPrice; }
        [[nodiscard]] double getWholesalePrice() const { return wholesalePrice; }
        [[nodiscard]] const Json::Value& getVoucherList() const { return voucherList; }


        // Setters
        void setBatchId(const std::string& value) { batchId = value; }
        void setRetailPrice(const double value) { retailPrice = value; }
        void setWholesalePrice(const double value) { wholesalePrice = value; }
        void setVoucherList(const Json::Value& value) { voucherList = value; }


    private:
        std::string batchId;
        Json::Value voucherList;
        double retailPrice{0.0};
        double wholesalePrice{0.0};

    };

    inline void WaecVoucherDto::fromJson(const Json::Value& json) {

        if (json.isMember("batchId") && !json["batchId"].isNull()) {
            setBatchId(json["batchId"].asString());
        }

        if (json.isMember("voucherList") && !json["voucherList"].isNull()) {
            setVoucherList(json["voucherList"]);
        }

        if (json.isMember("retailPrice") && !json["retailPrice"].isNull()) {
            setRetailPrice(json["retailPrice"].asDouble());
        }

        if (json.isMember("wholesalePrice") && !json["wholesalePrice"].isNull()) {
            setWholesalePrice(json["wholesalePrice"].asDouble());
        }

    }

    class UpdateMerchantSettlementAccountDto {
        public:

        UpdateMerchantSettlementAccountDto() = default;
        explicit UpdateMerchantSettlementAccountDto(const Json::Value& json);

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const std::string& getMerchantIdentifier() const { return merchantIdentifier; }
        [[nodiscard]] const std::string& getPin() const { return pin; }
        [[nodiscard]] const std::string& getAccountType() const { return accountType; }
        [[nodiscard]] const std::string& getAccountProvider() const { return accountProvider; }
        [[nodiscard]] const std::string& getBankAccountNumber() const { return bankAccountNumber; }

        // Setters
        void setMerchantIdentifier(const std::string& value) { merchantIdentifier = value; }
        void setPin(const std::string& value) { pin = value; }
        void setAccountType(const std::string& value) { accountType = value; }
        void setAccountProvider(const std::string& value) { accountProvider = value; }
        void setBankAccountNumber(const std::string& value) { bankAccountNumber = value; }


    private:
        std::string merchantIdentifier;
        std::string pin;
        std::string accountType;
        std::string accountProvider;
        std::string bankAccountNumber; // optional; the merchant account phone number will be the default mobile money number


    };

    inline void UpdateMerchantSettlementAccountDto::fromJson(const Json::Value& json) {

        if (json.isMember("merchantIdentifier") && !json["merchantIdentifier"].isNull()) {
            setMerchantIdentifier(json["merchantIdentifier"].asString());
        }

        if (json.isMember("pin") && !json["pin"].isNull()) {
            setPin(json["pin"].asString());
        }

        if (json.isMember("accountType") && !json["accountType"].isNull()) {
            setAccountType(json["accountType"].asString());
        }

        if (json.isMember("accountProvider") && !json["accountProvider"].isNull()) {
            setAccountProvider(json["accountProvider"].asString());
        }

        if (json.isMember("bankAccountNumber") && !json["bankAccountNumber"].isNull()) {
            setBankAccountNumber(json["bankAccountNumber"].asString());
        }



    }


    class ChangeMerchantPinDto : public WaecMerchantAuthDto {

    public:
        ChangeMerchantPinDto() = default;
        explicit ChangeMerchantPinDto(const Json::Value& json) { fromJson(json); }

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const std::string& getNewPin() const { return newPin; }

        // Setters
        void setNewPin(const std::string& value) { newPin = value; }


    private:
        std::string newPin;
    };

    inline void ChangeMerchantPinDto::fromJson(const Json::Value& json) {
        WaecMerchantAuthDto::fromJson(json);
        if (json.isMember("newPin") && !json["newPin"].isNull()) {
            setNewPin(json["newPin"].asString());
        }
    }

    class ResetMerchantPinDto : public WaecMerchantAuthDto {

    public:

        ResetMerchantPinDto() = default;
        explicit ResetMerchantPinDto(const Json::Value& json) { fromJson(json); }

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const std::string& getOtpCode() const { return otpCode; }
        [[nodiscard]] const std::string& getNewPin() const { return newPin; }

        // Setters
        void setOtpCode(const std::string& value) { otpCode = value; }
        void setNewPin(const std::string& value) { newPin = value; }


    private:
        std::string otpCode;
        std::string newPin;
    };

    inline void ResetMerchantPinDto::fromJson(const Json::Value& json) {
        WaecMerchantAuthDto::fromJson(json);
        if (json.isMember("otpCode") && !json["otpCode"].isNull()) {
            setOtpCode(json["otpCode"].asString());
        }
        if (json.isMember("newPin") && !json["newPin"].isNull()) {
            setNewPin(json["newPin"].asString());
        }
    }

    class UpdateMerchantEmailDto: public WaecMerchantAuthDto {
        public:
        UpdateMerchantEmailDto() = default;
        explicit UpdateMerchantEmailDto(const Json::Value& json) { fromJson(json); }

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const std::string& getNewEmail() const { return newEmail; }

        // Setters
        void setNewEmail(const std::string& value) { newEmail = value; }


    private:
        std::string newEmail;

    };

    inline void UpdateMerchantEmailDto::fromJson(const Json::Value& json) {
        WaecMerchantAuthDto::fromJson(json);
        if (json.isMember("newEmail") && !json["newEmail"].isNull()) {
            setNewEmail(json["newEmail"].asString());
        }
    }


    //buying from e-voucher service directly

    class SingleWaecVoucherSaleDto {
    public:
        SingleWaecVoucherSaleDto() = default;
        explicit SingleWaecVoucherSaleDto(const Json::Value& json) { fromJson(json); }

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const std::string& getBuyerPhoneNo() const { return buyerPhoneNo; }
        [[nodiscard]] const std::string& getBuyerEmail() const { return buyerEmail; }
        [[nodiscard]] const std::string& getProvider() const { return provider; }
        [[nodiscard]] const std::string& getSource() const { return source; }
        [[nodiscard]] int getVoucherType() const { return voucherType; }

        // Setters
        void setBuyerPhoneNo(const std::string& value) { buyerPhoneNo = value; }
        void setBuyerEmail(const std::string& value) { buyerEmail = value; }
        void setProvider(const std::string& value) { provider = value; }
        void setSource(const std::string& value) { source = value; }
        void setVoucherType(const int value) { voucherType = value; }



    private:
        std::string buyerPhoneNo;
        std::string source;
        std::string provider;
        std::string buyerEmail; //alternative if the user doesnt receive SMS : will be triggered.
        int quantity {1};
        int voucherType {0};

    };

    inline void SingleWaecVoucherSaleDto::fromJson(const Json::Value& json) {
        if (json.isMember("buyerPhoneNo") && !json["buyerPhoneNo"].isNull()) {
            setBuyerPhoneNo(json["buyerPhoneNo"].asString());
        }
        if (json.isMember("buyerEmail") && !json["buyerEmail"].isNull()) {
            setBuyerEmail(json["buyerEmail"].asString());
        }
        if (json.isMember("provider") && !json["provider"].isNull()) {
            setProvider(json["provider"].asString());
        }
        if (json.isMember("source") && !json["source"].isNull()) {
            setSource(json["source"].asString());
        }
        if (json.isMember("voucherType") && !json["voucherType"].isNull()) {
            setVoucherType(json["voucherType"].asInt());
        }
    }

    //merchant buying bulk vouchers from the evoucher service
    class BulkWaecVoucherSaleDto : public WaecMerchantDto {
        public:

        BulkWaecVoucherSaleDto() = default;
        explicit BulkWaecVoucherSaleDto(const Json::Value& json) { fromJson(json); }

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] int getQuantity() const { return quantity; }
        [[nodiscard]] int getVoucherType() const { return voucherType; }

        // Setters
        void setQuantity(const int value) { quantity = value; }
        void setVoucherType(const int value) { voucherType = value; }

    private:
        int quantity {25};
        int voucherType {0};
    };

    inline void BulkWaecVoucherSaleDto::fromJson(const Json::Value& json) {
        WaecMerchantDto::fromJson(json);
        if (json.isMember("quantity") && !json["quantity"].isNull()) {
            setQuantity(json["quantity"].asInt());
        }
        if (json.isMember("voucherType") && !json["voucherType"].isNull()) {
            setVoucherType(json["voucherType"].asInt());
        }
    }


    // affiliate or referral voucher sale
    class AffiliateWaecVoucherSaleDto : public WaecMerchantDto {
        public:

        AffiliateWaecVoucherSaleDto() = default;
        explicit AffiliateWaecVoucherSaleDto(const Json::Value& json) { fromJson(json); }

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] int getQuantity() const { return quantity; }
        [[nodiscard]] const std::string& getRecipientPhone() const { return recipientPhone; }
        [[nodiscard]] const std::string& getRecipientEmail() const { return recipientEmail; }


        // Setters
        void setQuantity(const int value) { quantity = value; }
        void setRecipientPhone(const std::string& value) { recipientPhone = value; }
        void setRecipientEmail(const std::string& value) { recipientEmail = value; }

    private:
        std::string recipientPhone;
        std::string recipientEmail; //optional
        int quantity{1};

    };

    inline void AffiliateWaecVoucherSaleDto::fromJson(const Json::Value& json) {
        WaecMerchantDto::fromJson(json);
        if (json.isMember("quantity") && !json["quantity"].isNull()) {
            setQuantity(json["quantity"].asInt());
        }
        if (json.isMember("recipientPhone") && !json["recipientPhone"].isNull()) {
            setRecipientPhone(json["recipientPhone"].asString());
        }
        if (json.isMember("recipientEmail") && !json["recipientEmail"].isNull()) {
            setRecipientEmail(json["recipientEmail"].asString());
        }
    }


    class WaecVoucher {
    public:
        std::string serialNumber;
        std::string pin;

        WaecVoucher() = default;

        explicit WaecVoucher(const Json::Value& json) {
            fromJson(json);
        }

        void fromJson(const Json::Value& json) {
            if (json.isMember("serialNumber") && !json["serialNumber"].isNull()) {
                serialNumber = json["serialNumber"].asString();
            }
            if (json.isMember("pin") && !json["pin"].isNull()) {
                pin = json["pin"].asString();
            }
        }

        [[nodiscard]] Json::Value toJson() const {
            Json::Value json;
            json["serialNumber"] = serialNumber;
            json["pin"] = pin;
            return json;
        }

    };


}
#endif //DATASUPPORTAPI_WAECMERCHANTAUTHDTO_H
