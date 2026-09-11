//
// Created by Emmanuel Addo-Odame on 30/08/2026.
//

#ifndef DATASUPPORTAPI_SYNCAPICREDENTIALDTO_H
#define DATASUPPORTAPI_SYNCAPICREDENTIALDTO_H
#include <json/json.h>
#include <string>

namespace data_support::dto::erp_api {

    class SyncApiCredentialDto {

    public:

        SyncApiCredentialDto() = default;

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const std::string& getAccountId() const { return account_id_; }
        [[nodiscard]] const std::string& getAccountSecret() const { return account_secret_; }
        [[nodiscard]] const std::string& getBusinessAccountId() const { return business_account_id_; }

        // Setters
        void setAccountId(const std::string& value) { account_id_ = value; }
        void setAccountSecret(const std::string& value) { account_secret_ = value; }
        void setBusinessAccountId(const std::string& value) { business_account_id_ = value; }

    private:

        std::string account_id_;
        std::string account_secret_;
        std::string business_account_id_;

    };


    inline void SyncApiCredentialDto::fromJson(const Json::Value& json) {

        if (json.isMember("accountId") && !json["accountId"].isNull()) {
            setAccountId(json["accountId"].asString());
        }

        if (json.isMember("accountSecret") && !json["accountSecret"].isNull()) {
            setAccountSecret(json["accountSecret"].asString());
        }

        if (json.isMember("businessAccountId") && !json["businessAccountId"].isNull()) {
            setAccountSecret(json["businessAccountId"].asString());
        }

    }

}
#endif //DATASUPPORTAPI_SYNCAPICREDENTIALDTO_H
