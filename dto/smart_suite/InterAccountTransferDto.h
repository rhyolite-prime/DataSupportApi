//
// Created by Emmanuel Addo-Odame on 26/07/2026.
//

#ifndef DATASUPPORTAPI_INTERACCOUNTTRANSFERDTO_H
#define DATASUPPORTAPI_INTERACCOUNTTRANSFERDTO_H
#include <json/json.h>

namespace data_support::dto::smart_suite {

    class InterAccountTransferDto {

    public:
        InterAccountTransferDto() = default;
        explicit InterAccountTransferDto(const Json::Value& json);

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const std::string& getPhoneNo() const { return phone_no_; }
        [[nodiscard]] const std::string& getPin() const { return pin_; }
        [[nodiscard]] double getAmount() const { return amount_; }
        [[nodiscard]] const std::string& getDestinationAccount() const { return destination_account_; }

        // Setters
        void setPhoneNo(const std::string& value) { phone_no_ = value; }
        void setPin(const std::string& value) { pin_ = value; }
        void setAmount(const double value) { amount_ = value; }
        void setDestinationAccount(const std::string& value) { destination_account_ = value; }


    private:
        std::string phone_no_;
        std::string pin_;
        double amount_ {0};
        std::string destination_account_;
    };

    inline void InterAccountTransferDto::fromJson(const Json::Value& json) {

        if (json.isMember("phoneNo") && !json["phoneNo"].isNull()) {
            setPhoneNo(json["phoneNo"].asString());
        }

        if (json.isMember("pin") && !json["pin"].isNull()) {
            setPin(json["pin"].asString());
        }

        if (json.isMember("amount") && !json["amount"].isNull()) {
            setAmount(json["amount"].asDouble());
        }

        if (json.isMember("destinationAccount") && !json["destinationAccount"].isNull()) {
            setDestinationAccount(json["destinationAccount"].asString());
        }

    }

}

#endif //DATASUPPORTAPI_INTERACCOUNTTRANSFERDTO_H