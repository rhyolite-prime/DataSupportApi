//
// Created by Emmanuel Addo-Odame on 26/07/2026.
//

#ifndef DATASUPPORTAPI_KIDDYWITHDRAWALREQUESTDTO_H
#define DATASUPPORTAPI_KIDDYWITHDRAWALREQUESTDTO_H
#include <json/json.h>

namespace data_support::dto::smart_suite {

    class KiddyWithdrawalRequestDto {

    public:
        KiddyWithdrawalRequestDto() = default;
        explicit KiddyWithdrawalRequestDto(const Json::Value& json);

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const std::string& getPhoneNo() const { return phone_no_; }
        [[nodiscard]] const std::string& getPin() const { return pin_; }
        [[nodiscard]] double getAmount() const { return amount_; }
        [[nodiscard]] const std::string& getSerialNo() const { return serial_no_; }

        // Setters
        void setPhoneNo(const std::string& value) { phone_no_ = value; }
        void setPin(const std::string& value) { pin_ = value; }
        void setAmount(const double value) { amount_ = value; }
        void setSerialNo(const std::string& value) { serial_no_ = value; }

    private:
        std::string phone_no_;
        std::string pin_;
        double amount_ {0};
        std::string serial_no_;

    };

    inline void KiddyWithdrawalRequestDto::fromJson(const Json::Value& json) {

        if (json.isMember("phoneNo") && !json["phoneNo"].isNull()) {
            setPhoneNo(json["phoneNo"].asString());
        }

        if (json.isMember("pin") && !json["pin"].isNull()) {
            setPin(json["pin"].asString());
        }

        if (json.isMember("amount") && !json["amount"].isNull()) {
            setAmount(json["amount"].asDouble());
        }

        if (json.isMember("serialNo") && !json["serialNo"].isNull()) {
            setSerialNo(json["serialNo"].asString());
        }

    }

}


#endif //DATASUPPORTAPI_KIDDYWITHDRAWALREQUESTDTO_H