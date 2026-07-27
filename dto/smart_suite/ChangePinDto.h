//
// Created by Emmanuel Addo-Odame on 24/07/2026.
//

#ifndef DATASUPPORTAPI_CHANGEPINDTO_H
#define DATASUPPORTAPI_CHANGEPINDTO_H
#include <json/json.h>

namespace data_support::dto::smart_suite {

    class ChangePinDto {

    public:
        ChangePinDto() = default;
        explicit ChangePinDto(const Json::Value& json);

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const std::string& getPhoneNo() const { return phone_no_; }
        [[nodiscard]] const std::string& getCurrentPin() const { return current_pin_; }
        [[nodiscard]] const std::string& getNewPin() const { return new_pin_; }

        // Setters
        void setPhoneNo(const std::string& value) { phone_no_ = value; }
        void setCurrentPin(const std::string& value) { current_pin_ = value; }
        void setNewPin(const std::string& value) { new_pin_ = value; }

    private:
        std::string phone_no_;
        std::string current_pin_;
        std::string new_pin_;
    };

    inline void ChangePinDto::fromJson(const Json::Value& json) {

        if (json.isMember("phoneNo") && !json["phoneNo"].isNull()) {
            setPhoneNo(json["phoneNo"].asString());
        }

        if (json.isMember("currentPin") && !json["currentPin"].isNull()) {
            setCurrentPin(json["currentPin"].asString());
        }

        if (json.isMember("newPin") && !json["newPin"].isNull()) {
            setNewPin(json["newPin"].asString());
        }

    }

}


#endif //DATASUPPORTAPI_CHANGEPINDTO_H