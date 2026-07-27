//
// Created by Emmanuel Addo-Odame on 25/07/2026.
//

#ifndef DATASUPPORTAPI_WEBCHECKOUTDEPOSITDTO_H
#define DATASUPPORTAPI_WEBCHECKOUTDEPOSITDTO_H
#include <json/json.h>

namespace data_support::dto::smart_suite {

    class WebCheckoutDepositDto {

    public:
        WebCheckoutDepositDto() = default;
        explicit WebCheckoutDepositDto(const Json::Value& json);

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const std::string& getPhoneNo() const { return phone_no_; }
        [[nodiscard]] double getAmount() const { return amount_; }
        [[nodiscard]] const std::string& getPrimaryCallbackUrl() const { return primary_callback_url_; }
        [[nodiscard]] const std::string& getCancellationUrl() const { return cancellation_url_; }
        [[nodiscard]] const std::string& getReturnUrl() const { return return_url_; }
        [[nodiscard]] const std::string& getDescription() const { return description_; }
        [[nodiscard]] const std::string& getTransactionType() const { return transaction_type_; }
        [[nodiscard]] const std::string& getAccountType() const { return account_type_; }
        [[nodiscard]] const std::string& getIntegrationChannel() const { return integration_channel_; }
        [[nodiscard]] const std::string& getIntegrationService() const { return integrating_service_; }
        [[nodiscard]] const std::string& getKiddyAccountSerialNo() const { return kiddy_account_serial_no_; }
        [[nodiscard]] const std::string& getPin() const { return pin_; }

        // Setters
        void setPhoneNo(const std::string& value) { phone_no_ = value; }
        void setAmount(const double value) { amount_ = value; }
        void setPrimaryCallbackUrl(const std::string& value) { primary_callback_url_ = value; }
        void setCancellationUrl(const std::string& value) { cancellation_url_ = value; }
        void setReturnUrl(const std::string& value) { return_url_ = value; }
        void setDescription(const std::string& value) { description_ = value; }
        void setTransactionType(const std::string& value) { transaction_type_ = value; }
        void setAccountType(const std::string& value) { account_type_ = value; }
        void setIntegrationChannel(const std::string& value) { integration_channel_ = value; }
        void setIntegrationService(const std::string& value) { integrating_service_ = value; }
        void setKiddyAccountSerialNo(const std::string& value) { kiddy_account_serial_no_ = value; }
        void setPin(const std::string& value) { pin_ = value; }

    private:
        std::string phone_no_;
        double amount_ {0};
        std::string primary_callback_url_;
        std::string cancellation_url_;
        std::string return_url_;
        std::string description_;
        std::string transaction_type_;
        std::string account_type_;
        std::string integration_channel_;
        std::string integrating_service_;
        std::string kiddy_account_serial_no_;
        std::string pin_;

    };

    inline void WebCheckoutDepositDto::fromJson(const Json::Value& json) {

        if (json.isMember("phoneNo") && !json["phoneNo"].isNull()) {
            setPhoneNo(json["phoneNo"].asString());
        }

        if (json.isMember("amount") && !json["amount"].isNull()) {
            setAmount(json["amount"].asDouble());
        }

        if (json.isMember("primaryCallbackUrl") && !json["primaryCallbackUrl"].isNull()) {
            setPrimaryCallbackUrl(json["primaryCallbackUrl"].asString());
        }

        if (json.isMember("cancellationUrl") && !json["cancellationUrl"].isNull()) {
            setCancellationUrl(json["cancellationUrl"].asString());
        }

        if (json.isMember("accountType") && !json["accountType"].isNull()) {
            setAccountType(json["accountType"].asString());
        }

        if (json.isMember("integrationChannel") && !json["integrationChannel"].isNull()) {
            setIntegrationChannel(json["integrationChannel"].asString());
        }

        if (json.isMember("integratingService") && !json["integratingService"].isNull()) {
            setIntegrationService(json["integratingService"].asString());
        }

        if (json.isMember("kiddyAccountSerialNo") && !json["kiddyAccountSerialNo"].isNull()) {
            setKiddyAccountSerialNo(json["kiddyAccountSerialNo"].asString());
        }

        if (json.isMember("pin") && !json["pin"].isNull()) {
            setPin(json["pin"].asString());
        }

    }

}


#endif //DATASUPPORTAPI_WEBCHECKOUTDEPOSITDTO_H