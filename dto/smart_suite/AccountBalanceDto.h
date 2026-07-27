//
// Created by Emmanuel Addo-Odame on 25/07/2026.
//

#ifndef DATASUPPORTAPI_ACCOUNTBALANCEDTO_H
#define DATASUPPORTAPI_ACCOUNTBALANCEDTO_H

#include <json/json.h>

namespace data_support::dto::smart_suite {

    class AccountBalanceDto {

    public:
        AccountBalanceDto() = default;
        explicit AccountBalanceDto(const Json::Value& json);

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const std::string& getPhoneNo() const { return phone_no_; }
        [[nodiscard]] const std::string& getPin() const { return pin_; }
        [[nodiscard]] const std::string& getAccountType() const { return account_type_; }

        // Setters
        void setPhoneNo(const std::string& value) { phone_no_ = value; }
        void setPin(const std::string& value) { pin_ = value; }
        void setAccountType(const std::string& value) { account_type_ = value; }

    private:
        std::string phone_no_;
        std::string pin_;
        std::string account_type_;
    };

    inline void AccountBalanceDto::fromJson(const Json::Value& json) {

        if (json.isMember("phoneNo") && !json["phoneNo"].isNull()) {
            setPhoneNo(json["phoneNo"].asString());
        }

        if (json.isMember("pin") && !json["pin"].isNull()) {
            setPin(json["pin"].asString());
        }

        if (json.isMember("accountType") && !json["accountType"].isNull()) {
            setAccountType(json["accountType"].asString());
        }

    }

}


#endif //DATASUPPORTAPI_ACCOUNTBALANCEDTO_H