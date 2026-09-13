//
// Created by Emmanuel Addo-Odame on 12/09/2026.
//

#ifndef DATASUPPORTAPI_BASEPOSDTO_H
#define DATASUPPORTAPI_BASEPOSDTO_H
#include <json/json.h>
#include <string>

namespace data_support::dto::erp_api {

    class ErpOperationResponse {
    public:
        std::string message;
        int code {1};
        bool success {false};

        [[nodiscard]]
        Json::Value toJson() const {
            Json::Value json;
            json["code"] = code;
            json["message"] = message;
            json["success"] = success;
            return json;
        }

    };

    class PosCustomerSignupDto {

    public:

        PosCustomerSignupDto() = default;

        void fromJson(const Json::Value& json);

        // Getters for wrapper fields
        [[nodiscard]] const std::string& getFirstName() const { return first_name_; }
        [[nodiscard]] const std::string& getLastName() const { return last_name_; }
        [[nodiscard]] const std::string& getUsername() const { return user_name_; }
        [[nodiscard]] const std::string& getPhoneNo() const { return phone_no_; }
        [[nodiscard]] const std::string& getEmail() const { return email_; }
        [[nodiscard]] const std::string& getPassword() const { return password_; }


        // Setters
        void setFirstName(const std::string& value) { first_name_ = value; }
        void setLastName(const std::string& value) { last_name_ = value; }
        void setUsername(const std::string& value) { user_name_ = value; }
        void setPhoneNo(const std::string& value) { phone_no_ = value; }
        void setEmail(const std::string& value) { email_ = value; }
        void setPassword(const std::string& value) { password_ = value; }


    private:

        std::string user_name_;
        std::string first_name_;
        std::string last_name_;
        std::string phone_no_;
        std::string email_;
        std::string password_;

    };

    inline void PosCustomerSignupDto::fromJson(const Json::Value& json) {

        // Parse wrapper fields (top-level)
        if (json.isMember("firstName") && !json["firstName"].isNull()) {
            setFirstName(json["firstName"].asString());
        }

        if (json.isMember("lastName") && !json["lastName"].isNull()) {
            setLastName(json["lastName"].asString());
        }

        if (json.isMember("email") && !json["email"].isNull()) {
            setEmail(json["email"].asString());
        }

        if (json.isMember("phoneNo") && !json["phoneNo"].isNull()) {
            setPhoneNo(json["phoneNo"].asString());
        }

        if (json.isMember("username") && !json["username"].isNull()) {
            setUsername(json["username"].asString());
        }

        if (json.isMember("password") && !json["password"].isNull()) {
            setUsername(json["password"].asString());
        }



    }


    class PosCustomerLoginDto {

    public:

        PosCustomerLoginDto() = default;

        void fromJson(const Json::Value& json);

        // Getters for wrapper fields
        [[nodiscard]] const std::string& getCustomerIdentifier() const { return customer_identifier_; }
        [[nodiscard]] const std::string& getPassword() const { return password_; }


        // Setters
        void setCustomerIdentifier(const std::string& value) { customer_identifier_ = value; }
        void setPassword(const std::string& value) { password_ = value; }


    private:

        std::string customer_identifier_;
        std::string password_;

    };

    inline void PosCustomerLoginDto::fromJson(const Json::Value& json) {

        // Parse wrapper fields (top-level)
        if (json.isMember("customerIdentifier") && !json["customerIdentifier"].isNull()) {
            setCustomerIdentifier(json["customerIdentifier"].asString());
        }
        if (json.isMember("password") && !json["password"].isNull()) {
            setPassword(json["password"].asString());
        }

    }


}


#endif //DATASUPPORTAPI_BASEPOSDTO_H
