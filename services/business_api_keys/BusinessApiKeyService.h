//
// Created by Emmanuel Addo-Odame on 23/07/2026.
//

#ifndef DATASUPPORTAPI_BUSINESSAPIKEYSERVICE_H
#define DATASUPPORTAPI_BUSINESSAPIKEYSERVICE_H
#include <drogon/drogon.h>
#include "dto/BaseApiResponse.h"
#include "dto/BusinessApiKeyDto.h"

namespace data_support::services {

    struct ApiCredentialsValidationResult {
        bool isValid = false;
        std::string accountId;
        std::string businessId;
        std::string errorMessage;
        int errorCode = 0;
    };

    class BusinessApiKeyService {

    public:
        drogon::Task<dto::BaseApiResponse> getAll(const std::string &businessId, int pageNo, int pageSize, const std::string &query);

        drogon::Task<dto::BaseApiResponse> create(const dto::BusinessApiKeyDto &dto); //generates an API Key for a business

        drogon::Task<dto::BaseApiResponse> revoke(const std::string &businessId, const std::string &id);

        drogon::Task<dto::BaseApiResponse> activate(const std::string &businessId, const std::string &id);

        drogon::Task<dto::BaseApiResponse> deleteApiKey(const std::string &businessId, const std::string &id);

        drogon::Task<ApiCredentialsValidationResult> validateApiCredentials(const std::string &clientId, const std::string &clientSecret);
    };

}
#endif //DATASUPPORTAPI_BUSINESSAPIKEYSERVICE_H