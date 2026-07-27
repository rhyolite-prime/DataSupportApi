//
// Created by Emmanuel Addo-Odame on 22/07/2026.
//

#ifndef DATASUPPORTAPI_BUSINESSACCOUNTDTO_H
#define DATASUPPORTAPI_BUSINESSACCOUNTDTO_H
#include <json/json.h>

namespace data_support::dto {

    class BusinessAccountDto {

    public:

        BusinessAccountDto() = default;

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const std::string& getBusinessName() const { return business_name_; }
        [[nodiscard]] const std::string& getAccountId() const { return account_id_; }
        [[nodiscard]] const std::string& getContactPersonName() const { return contact_person_name_; }
        [[nodiscard]] const std::string& getContactPersonEmail() const { return contact_person_email_; }
        [[nodiscard]] const std::string& getContactPersonPhoneNo() const { return contact_person_phone_no_; }
        [[nodiscard]] const std::string& getBusinessEmail() const { return business_email_; }
        [[nodiscard]] const std::string& getBusinessPhoneNo() const { return business_phone_no_; }
        [[nodiscard]] int32_t getStatus() const { return status; }
        [[nodiscard]] bool isSubAccountEnabled() const { return sub_account_enabled_; }
        [[nodiscard]] bool isRequireTwoFactorAuth() const { return require_two_factor_auth_; }
        [[nodiscard]] const Json::Value& getSubscriptionModel() const { return subscription_model_; }
        [[nodiscard]] const std::string& getBusinessLogo() const { return business_logo_; }
        [[nodiscard]] const std::string& getSubscriptionStartDate() const { return subscription_start_date_; }
        [[nodiscard]] const std::string& getSubscriptionEndDate() const { return subscription_end_date_; }
        [[nodiscard]] const std::string& getCountry() const { return country_; }
        [[nodiscard]] const std::string& getCurrency() const { return currency_; }

        // Setters
        void setBusinessName(const std::string& value) { business_name_ = value; }
        void setAccountId(const std::string& value) { account_id_ = value; }
        void setContactPersonName(const std::string& value) { contact_person_name_ = value; }
        void setContactPersonEmail(const std::string& value) { contact_person_email_ = value; }
        void setContactPersonPhoneNo(const std::string& value) { contact_person_phone_no_ = value; }
        void setBusinessEmail(const std::string& value) { business_email_ = value; }
        void setBusinessPhoneNo(const std::string& value) { business_phone_no_ = value; }
        void setStatus(int32_t value) { status = value; }
        void setSubAccountEnabled(bool value) { sub_account_enabled_ = value; }
        void setRequireTwoFactorAuth(bool value) { require_two_factor_auth_ = value; }
        void setSubscriptionModel(const Json::Value& value) { subscription_model_ = value; }
        void setBusinessLogo(const std::string& value) { business_logo_ = value; }
        void setSubscriptionStartDate(const std::string& value) { subscription_start_date_ = value; }
        void setSubscriptionEndDate(const std::string& value) { subscription_end_date_ = value; }
        void setCountry(const std::string& value) { country_ = value; }
        void setCurrency(const std::string& value) { currency_ = value; }


    private:
        std::string business_name_;
        std::string account_id_;
        std::string contact_person_name_;
        std::string contact_person_email_;
        std::string contact_person_phone_no_;
        std::string business_email_;
        std::string business_phone_no_;
        int32_t status {0};
        bool sub_account_enabled_ {false};
        bool require_two_factor_auth_ {false};
        Json::Value subscription_model_;
        std::string business_logo_;
        std::string subscription_start_date_;
        std::string subscription_end_date_;
        std::string country_;
        std::string currency_;
    };

    inline void BusinessAccountDto::fromJson(const Json::Value& json) {

        if (json.isMember("businessName") && !json["businessName"].isNull()) {
            setBusinessName(json["businessName"].asString());
        }

        if (json.isMember("accountId") && !json["accountId"].isNull()) {
            setAccountId(json["accountId"].asString());
        }

        if (json.isMember("contactPersonName") && !json["contactPersonName"].isNull()) {
            setContactPersonName(json["contactPersonName"].asString());
        }

        if (json.isMember("contactPersonEmail") && !json["contactPersonEmail"].isNull()) {
            setContactPersonEmail(json["contactPersonEmail"].asString());
        }

        if (json.isMember("contactPersonPhoneNo") && !json["contactPersonPhoneNo"].isNull()) {
            setContactPersonPhoneNo(json["contactPersonPhoneNo"].asString());
        }

        if (json.isMember("businessEmail") && !json["businessEmail"].isNull()) {
             setBusinessEmail(json["businessEmail"].asString());
        }

        if (json.isMember("businessPhoneNo") && !json["businessPhoneNo"].isNull()) {
            setBusinessPhoneNo(json["businessPhoneNo"].asString());
        }

        if (json.isMember("status") && !json["status"].isNull()) {
            setStatus(json["status"].asInt());
        }

        if (json.isMember("subAccountEnabled") && !json["subAccountEnabled"].isNull()) {
            setSubAccountEnabled(json["subAccountEnabled"].asBool());
        }

        if (json.isMember("requireTwoFactorAuth") && !json["requireTwoFactorAuth"].isNull()) {
            setRequireTwoFactorAuth(json["requireTwoFactorAuth"].asBool());
        }

        if (json.isMember("subscriptionModel") && !json["subscriptionModel"].isNull()) {
            setSubscriptionModel(json["subscriptionModel"]);
        }

        if (json.isMember("businessLogo") && !json["businessLogo"].isNull()) {
            setBusinessLogo(json["businessLogo"].asString());
        }

        if (json.isMember("subscriptionStartDate") && !json["subscriptionStartDate"].isNull()) {
            setSubscriptionStartDate(json["subscriptionStartDate"].asString());
        }

        if (json.isMember("subscriptionEndDate") && !json["subscriptionEndDate"].isNull()) {
            setSubscriptionEndDate(json["subscriptionEndDate"].asString());
        }

        if (json.isMember("country") && !json["country"].isNull()) {
            setCountry(json["country"].asString());
        }

        if (json.isMember("currency") && !json["currency"].isNull()) {
            setCurrency(json["currency"].asString());
        }

    }

}
#endif //DATASUPPORTAPI_BUSINESSACCOUNTDTO_H