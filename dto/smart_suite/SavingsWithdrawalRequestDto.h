//
// Created by Emmanuel Addo-Odame on 26/07/2026.
//

#ifndef DATASUPPORTAPI_SAVINGSWITHDRAWALREQUESTDTO_H
#define DATASUPPORTAPI_SAVINGSWITHDRAWALREQUESTDTO_H
#include <json/json.h>

namespace data_support::dto::smart_suite {

    class SavingsWithdrawalRequestDto {

    public:
        SavingsWithdrawalRequestDto() = default;
        explicit SavingsWithdrawalRequestDto(const Json::Value& json);

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const std::string& getPhoneNo() const { return phone_no_; }
        [[nodiscard]] const std::string& getPin() const { return pin_; }
        [[nodiscard]] double getAmount() const { return amount_; }

        // Setters
        void setPhoneNo(const std::string& value) { phone_no_ = value; }
        void setPin(const std::string& value) { pin_ = value; }
        void setAmount(const double value) { amount_ = value; }


    private:
        std::string phone_no_;
        std::string pin_;
        double amount_ {0};
    };

    inline void SavingsWithdrawalRequestDto::fromJson(const Json::Value& json) {

        if (json.isMember("phoneNo") && !json["phoneNo"].isNull()) {
            setPhoneNo(json["phoneNo"].asString());
        }

        if (json.isMember("pin") && !json["pin"].isNull()) {
            setPin(json["pin"].asString());
        }

        if (json.isMember("amount") && !json["amount"].isNull()) {
            setAmount(json["amount"].asDouble());
        }

    }

}


#endif //DATASUPPORTAPI_SAVINGSWITHDRAWALREQUESTDTO_H