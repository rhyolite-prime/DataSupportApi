//
// Created by Emmanuel Addo-Odame
//

#ifndef DATASUPPORTAPI_DIRECTRECEIVEMONEYDTO_H
#define DATASUPPORTAPI_DIRECTRECEIVEMONEYDTO_H

#include <string>
#include <json/json.h>

namespace data_support::dto {

    class DirectReceiveMoneyDto {
        std::string customerName;
        std::string customerMsisdn;
        std::string customerEmail;
        double amount;
        std::string primaryCallbackUrl;
        std::string description;
        std::string provider;

        // Extra fields inferred from HubtelApi.cc
        std::string channel;
        std::string clientReference;

    public:
        DirectReceiveMoneyDto() : amount(0.0) {}

        // Getters
        [[nodiscard]] const std::string& getCustomerName() const { return customerName; }
        [[nodiscard]] const std::string& getCustomerMsisdn() const { return customerMsisdn; }
        [[nodiscard]] const std::string& getCustomerEmail() const { return customerEmail; }
        [[nodiscard]] double getAmount() const { return amount; }
        [[nodiscard]] const std::string& getPrimaryCallbackUrl() const { return primaryCallbackUrl; }
        [[nodiscard]] const std::string& getDescription() const { return description; }
        [[nodiscard]] const std::string& getProvider() const { return provider; }
        [[nodiscard]] const std::string& getChannel() const { return channel; }
        [[nodiscard]] const std::string& getClientReference() const { return clientReference; }

        // Setters
        void setCustomerName(const std::string& val) { customerName = val; }
        void setCustomerMsisdn(const std::string& val) { customerMsisdn = val; }
        void setCustomerEmail(const std::string& val) { customerEmail = val; }
        void setAmount(double val) { amount = val; }
        void setPrimaryCallbackUrl(const std::string& val) { primaryCallbackUrl = val; }
        void setDescription(const std::string& val) { description = val; }
        void setProvider(const std::string& val) { provider = val; }
        void setChannel(const std::string& val) { channel = val; }
        void setClientReference(const std::string& val) { clientReference = val; }
    };

    class DirectSendMomoDto {
        std::string recipientName;
        std::string recipientMsisdn;
        std::string customerEmail;
        double amount {0};
        std::string primaryCallbackUrl;
        std::string description;
        std::string provider;
        std::string clientReference;

    public:

        // Getters
        [[nodiscard]] const std::string& getRecipientName() const { return recipientName; }
        [[nodiscard]] const std::string& getRecipientMsisdn() const { return recipientMsisdn; }
        [[nodiscard]] const std::string& getCustomerEmail() const { return customerEmail; }
        [[nodiscard]] double getAmount() const { return amount; }
        [[nodiscard]] const std::string& getPrimaryCallbackUrl() const { return primaryCallbackUrl; }
        [[nodiscard]] const std::string& getDescription() const { return description; }
        [[nodiscard]] const std::string& getProvider() const { return provider; }
        [[nodiscard]] const std::string& getClientReference() const { return clientReference; }

        // Setters
        void setRecipientName(const std::string& val) { recipientName = val; }
        void setRecipientMsisdn(const std::string& val) { recipientMsisdn = val; }
        void setCustomerEmail(const std::string& val) { customerEmail = val; }
        void setAmount(double val) { amount = val; }
        void setPrimaryCallbackUrl(const std::string& val) { primaryCallbackUrl = val; }
        void setDescription(const std::string& val) { description = val; }
        void setProvider(const std::string& val) { provider = val; }
        void setClientReference(const std::string& val) { clientReference = val; }
    };

    class DirectSendToBankDto {
        std::string bankName;
        std::string bankBranch;
        std::string bankBranchCode;
        double amount {0};
        std::string bankAccountNumber;
        std::string bankAccountName;
        std::string primaryCallbackUrl;
        std::string description;
        std::string recipientPhoneNumber;
        std::string senderMsisdn;
        std::string clientReference;

    public:

        // Getters
        [[nodiscard]] const std::string& getBankName() const { return bankName; }
        [[nodiscard]] const std::string& getBankBranch() const { return bankBranch; }
        [[nodiscard]] const std::string& getBankBranchCode() const { return bankBranchCode; }
        [[nodiscard]] double getAmount() const { return amount; }
        [[nodiscard]] const std::string& getBankAccountNumber() const { return bankAccountNumber; }
        [[nodiscard]] const std::string& getBankAccountName() const { return bankAccountName; }
        [[nodiscard]] const std::string& getPrimaryCallbackUrl() const { return primaryCallbackUrl; }
        [[nodiscard]] const std::string& getDescription() const { return description; }
        [[nodiscard]] const std::string& getRecipientPhoneNumber() const { return recipientPhoneNumber; }
        [[nodiscard]] const std::string& getSenderMsisdn() const { return senderMsisdn; }
        [[nodiscard]] const std::string& getClientReference() const { return clientReference; }

        // Setters
        void setBankName(const std::string& val) { bankName = val; }
        void setBankBranch(const std::string& val) { bankBranch = val; }
        void setBankBranchCode(const std::string& val) { bankBranchCode = val; }
        void setAmount(double val) { amount = val; }
        void setBankAccountNumber(const std::string& val) { bankAccountNumber = val; }
        void setBankAccountName(const std::string& val) { bankAccountName = val; }
        void setPrimaryCallbackUrl(const std::string& val) { primaryCallbackUrl = val; }
        void setDescription(const std::string& val) { description = val; }
        void setRecipientPhoneNumber(const std::string& val) { recipientPhoneNumber = val; }
        void setSenderMsisdn(const std::string& val) { senderMsisdn = val; }
        void setClientReference(const std::string& val) { clientReference = val; }
    };

    class TransactionResponse {
    public:
        std::string accountType;
        std::string accountBalance;
        std::string message;
        std::string clientReference;
        int code {0};
        std::string checkoutUrl;
    };

    class HubtelWebCheckoutDto {
    public:
        std::string phoneNo;
        double totalAmount {0};
        std::string description;
        std::string callbackUrl;
        std::string returnUrl;
        std::string cancellationUrl;
        std::string clientReference;

        // Getters
        [[nodiscard]] const std::string& getPhoneNo() const { return phoneNo; }
        [[nodiscard]] double getTotalAmount() const { return totalAmount; }
        [[nodiscard]] const std::string& getDescription() const { return description; }
        [[nodiscard]] const std::string& getCallbackUrl() const { return callbackUrl; }
        [[nodiscard]] const std::string& getReturnUrl() const { return returnUrl; }
        [[nodiscard]] const std::string& getCancellationUrl() const { return cancellationUrl; }
        [[nodiscard]] const std::string& getClientReference() const { return clientReference; }
    };

    class PaystackWebCheckoutDto {
    public:
        std::string email;
        double amount {0};
        std::string callbackUrl;
        std::string reference;

        // Getters
        [[nodiscard]] const std::string& getEmail() const { return email; }
        [[nodiscard]] double getTotalAmount() const { return amount; }
        [[nodiscard]] const std::string& getCallbackUrl() const { return callbackUrl; }
        [[nodiscard]] const std::string& getClientReference() const { return reference; }
    };

    class HubtelWebCheckoutResponse {
    public:
        struct Data {
            std::string checkoutUrl;
            std::string checkoutId;
            std::string clientReference;
            std::string message;
            std::string checkoutDirectUrl;
        };

        std::string responseCode;
        std::string status;
        Data data;

        static HubtelWebCheckoutResponse fromJson(const Json::Value &json) {
            HubtelWebCheckoutResponse res;
            // API returns camelCase: responseCode, status, data
            if (json.isMember("responseCode")) res.responseCode = json["responseCode"].asString();
            else if (json.isMember("ResponseCode")) res.responseCode = json["ResponseCode"].asString();

            if (json.isMember("status")) res.status = json["status"].asString();
            else if (json.isMember("Status")) res.status = json["Status"].asString();

            const Json::Value *dataNode = nullptr;
            if (json.isMember("data") && json["data"].isObject()) dataNode = &json["data"];
            else if (json.isMember("Data") && json["Data"].isObject()) dataNode = &json["Data"];

            if (dataNode) {
                const auto &d = *dataNode;
                if (d.isMember("checkoutUrl")) res.data.checkoutUrl = d["checkoutUrl"].asString();
                else if (d.isMember("CheckoutUrl")) res.data.checkoutUrl = d["CheckoutUrl"].asString();

                if (d.isMember("checkoutId")) res.data.checkoutId = d["checkoutId"].asString();
                else if (d.isMember("CheckoutId")) res.data.checkoutId = d["CheckoutId"].asString();

                if (d.isMember("clientReference")) res.data.clientReference = d["clientReference"].asString();
                else if (d.isMember("ClientReference")) res.data.clientReference = d["ClientReference"].asString();

                if (d.isMember("message")) res.data.message = d["message"].asString();
                else if (d.isMember("Message")) res.data.message = d["Message"].asString();

                if (d.isMember("checkoutDirectUrl")) res.data.checkoutDirectUrl = d["checkoutDirectUrl"].asString();
                else if (d.isMember("CheckoutDirectUrl")) res.data.checkoutDirectUrl = d["CheckoutDirectUrl"].asString();
            }
            return res;
        }

        [[nodiscard]] Json::Value toJson() const {
            Json::Value json;
            json["ResponseCode"] = responseCode;
            json["Status"] = status;
            Json::Value d;
            d["CheckoutUrl"] = data.checkoutUrl;
            d["CheckoutId"] = data.checkoutId;
            d["ClientReference"] = data.clientReference;
            d["Message"] = data.message;
            d["CheckoutDirectUrl"] = data.checkoutDirectUrl;
            json["Data"] = d;
            return json;
        }
    };

    class PaystackWebCheckoutResponse {
    public:
        struct Data {
            std::string authorizationUrl;
            std::string accessCode;
            std::string reference;
        };

        std::string message;
        bool status {false};
        Data data;

        static PaystackWebCheckoutResponse fromJson(const Json::Value &json) {
            PaystackWebCheckoutResponse res;
            // API returns camelCase: responseCode, status, data
            if (json.isMember("message")) res.message = json["message"].asString();
            else if (json.isMember("message")) res.message = json["message"].asString();

            if (json.isMember("status")) res.status = json["status"].asBool();
            else if (json.isMember("Status")) res.status = json["Status"].asBool();

            const Json::Value *dataNode = nullptr;
            if (json.isMember("data") && json["data"].isObject()) dataNode = &json["data"];
            else if (json.isMember("Data") && json["Data"].isObject()) dataNode = &json["Data"];

            if (dataNode) {

                const auto &d = *dataNode;

                if (d.isMember("authorization_url")) res.data.authorizationUrl = d["authorization_url"].asString();

                if (d.isMember("access_code")) res.data.accessCode = d["access_code"].asString();

                if (d.isMember("reference")) res.data.reference = d["reference"].asString();

            }
            return res;
        }
    };

    class PaystackTransactionStatusResponse {
        public:
        struct Data {
            std::string gatewayResponse;
            std::string channel;
            std::string reference;
            std::string status;
            std::string transactionDate;
        };

        std::string message;
        bool status {false};
        Data data;

        static PaystackTransactionStatusResponse fromJson(const Json::Value &json) {
            PaystackTransactionStatusResponse res;
            // API returns camelCase: responseCode, status, data
            if (json.isMember("message")) res.message = json["message"].asString();
            else if (json.isMember("message")) res.message = json["message"].asString();

            if (json.isMember("status")) res.status = json["status"].asBool();
            else if (json.isMember("Status")) res.status = json["Status"].asBool();

            const Json::Value *dataNode = nullptr;
            if (json.isMember("data") && json["data"].isObject()) dataNode = &json["data"];
            else if (json.isMember("Data") && json["Data"].isObject()) dataNode = &json["Data"];

            if (dataNode) {

                const auto &d = *dataNode;

                if (d.isMember("gateway_response")) res.data.gatewayResponse = d["gateway_response"].asString();

                if (d.isMember("transaction_date")) res.data.transactionDate = d["transaction_date"].asString();

                if (d.isMember("reference")) res.data.reference = d["reference"].asString();

                if (d.isMember("status")) res.data.status = d["status"].asString();

                if (d.isMember("channel")) res.data.channel = d["channel"].asString();

            }
            return res;
        }

    };

    class AccountBalanceResponse {
    public:
        struct Data {
            double amount {0};
        };

        std::string responseCode;
        std::string message;
        Data data;

        static AccountBalanceResponse fromJson(const Json::Value &json) {
            AccountBalanceResponse res;

            if (json.isMember("responseCode")) res.responseCode = json["responseCode"].asString();
            if (json.isMember("message")) res.message = json["message"].asString();

            if (json.isMember("Data") && json["Data"].isObject()) {
                const auto &d = json["Data"];

                if (d.isMember("amount")) res.data.amount = d["amount"].asDouble();

            }
            return res;
        }
    };

    class TransactionStatusResponse {
    public:
        struct Data {
            std::string transactionId;
            std::string networkTransactionId;
            double amount {0};
            double fees {0};
            std::string clientReference;
            std::string channel;
            std::string customerNumber;
            std::string transactionStatus;
            std::string createdAt;
        };

        std::string responseCode;
        std::string status;
        Data data;

        static TransactionStatusResponse fromJson(const Json::Value &json) {
            TransactionStatusResponse res;

            if (json.isMember("ResponseCode")) res.responseCode = json["ResponseCode"].asString();
            if (json.isMember("Status")) res.status = json["Status"].asString();

            if (json.isMember("Data") && json["Data"].isObject()) {
                const auto &d = json["Data"];
                if (d.isMember("TransactionId")) res.data.transactionId = d["TransactionId"].asString();
                if (d.isMember("NetworkTransactionId")) res.data.networkTransactionId = d["NetworkTransactionId"].asString();
                if (d.isMember("Amount")) res.data.amount = d["Amount"].asDouble();
                if (d.isMember("Fees")) res.data.fees = d["Fees"].asDouble();
                if (d.isMember("ClientReference")) res.data.clientReference = d["ClientReference"].asString();
                if (d.isMember("Channel")) res.data.channel = d["Channel"].asString();
                if (d.isMember("CustomerNumber")) res.data.customerNumber = d["CustomerNumber"].asString();
                if (d.isMember("TransactionStatus")) res.data.transactionStatus = d["TransactionStatus"].asString();
                if (d.isMember("CreatedAt")) res.data.createdAt = d["CreatedAt"].asString();
            }
            return res;
        }

        [[nodiscard]] Json::Value toJson() const {
            Json::Value json;
            json["ResponseCode"] = responseCode;
            json["Status"] = status;
            Json::Value d;
            d["TransactionId"] = data.transactionId;
            d["NetworkTransactionId"] = data.networkTransactionId;
            d["Amount"] = data.amount;
            d["Fees"] = data.fees;
            d["ClientReference"] = data.clientReference;
            d["Channel"] = data.channel;
            d["CustomerNumber"] = data.customerNumber;
            d["TransactionStatus"] = data.transactionStatus;
            d["CreatedAt"] = data.createdAt;
            json["Data"] = d;
            return json;
        }
    };

}

#endif //DATASUPPORTAPI_DIRECTRECEIVEMONEYDTO_H
