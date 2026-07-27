//
// Created by Emmanuel Addo-Odame on 24/07/2026.
//

#ifndef DATASUPPORTAPI_MINISTATEMENTDTO_H
#define DATASUPPORTAPI_MINISTATEMENTDTO_H
#include <json/json.h>

namespace data_support::dto::smart_suite {

    class MiniStatementDto {

    public:
        MiniStatementDto() = default;
        explicit MiniStatementDto(const Json::Value& json);

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const std::string& getPhoneNo() const { return phone_no_; }
        [[nodiscard]] const std::string& getPin() const { return pin_; }
        [[nodiscard]] const std::string& getAccountType() const { return account_type_; }
        [[nodiscard]] const std::string& getSerialNo() const { return serial_no_; }

        // Setters
        void setPhoneNo(const std::string& value) { phone_no_ = value; }
        void setPin(const std::string& value) { pin_ = value; }
        void setAccountType(const std::string& value) { account_type_ = value; }
        void setSerialNo(const std::string& value) { serial_no_ = value; }

    private:
        std::string phone_no_;
        std::string pin_;
        std::string account_type_;
        std::string serial_no_;
    };

    inline void MiniStatementDto::fromJson(const Json::Value& json) {

        if (json.isMember("phoneNo") && !json["phoneNo"].isNull()) {
            setPhoneNo(json["phoneNo"].asString());
        }

        if (json.isMember("pin") && !json["pin"].isNull()) {
            setPin(json["pin"].asString());
        }

        if (json.isMember("accountType") && !json["accountType"].isNull()) {
            setAccountType(json["accountType"].asString());
        }

        if (json.isMember("serialNo") && !json["serialNo"].isNull()) {
            setSerialNo(json["serialNo"].asString());
        }

    }

}


#endif //DATASUPPORTAPI_MINISTATEMENTDTO_H