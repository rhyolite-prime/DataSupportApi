//
// Created by Emmanuel Addo-Odame on 23/07/2026.
//

#include "BusinessApiKeyService.h"
#include "models/BusinessApiKeys.h"
#include "models/BusinessAccounts.h"
#include "utils/JsonUtils.h"
#include "constants/ErrorCode.h"
#include <drogon/drogon.h>
#include <drogon/orm/CoroMapper.h>
#include <bcrypt.h>
#include "utils/IdGeneratorUtils.h"

using namespace drogon::orm;

namespace data_support::services {

    drogon::Task<dto::BaseApiResponse> BusinessApiKeyService::getAll(const std::string &businessId, int pageNo, int pageSize, const std::string &query) {
        dto::BaseApiResponse response;
        try {
            auto dbClient = drogon::app().getDbClient();
            drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessApiKeys> mapper(dbClient);

            auto apiKeys = co_await mapper.paginate(pageNo, pageSize).findBy(
                drogon::orm::Criteria(drogon_model::DataSupportDb::BusinessApiKeys::Cols::_business_id, drogon::orm::CompareOperator::EQ, businessId)
            );

            Json::Value result(Json::arrayValue);
            for (const auto &apiKey : apiKeys) {
                result.append(utils::JsonUtils::convertKeysToCamelCase(apiKey.toJson()));
            }

            response.success = true;
            response.message = "Business API keys retrieved successfully";
            response.result = result;
        } catch (const std::exception &e) {
            response.success = false;
            response.message = e.what();
        }
        co_return response;
    }

    drogon::Task<dto::BaseApiResponse> BusinessApiKeyService::create(const dto::BusinessApiKeyDto &dto) {
        dto::BaseApiResponse response;
        try {
            auto dbClient = drogon::app().getDbClient();
            drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessApiKeys> mapper(dbClient);

            // 1. Generate Client ID and Client Secret
            std::string clientId = utils::IdGeneratorUtils::generateAlphanumericId(10);
            std::string clientSecret = utils::IdGeneratorUtils::generateAlphanumericId(18);

            // 2. Hash Client Secret
            std::string clientSecretHash = bcrypt::generateHash(clientSecret,6);

            drogon_model::DataSupportDb::BusinessApiKeys apiKey;
            apiKey.setBusinessId(dto.getBusinessId());
            apiKey.setClientId(clientId);
            apiKey.setClientSecretHash(clientSecretHash);
            apiKey.setLabel(dto.getLabel());
            apiKey.setBusinessName(dto.getBusinessName());

            Json::Value scopesJson(Json::arrayValue);
            for (const auto &scope : dto.getScopes()) {
                scopesJson.append(scope);
            }
            Json::FastWriter writer;
            apiKey.setScopes(writer.write(scopesJson));

            Json::Value ipsJson(Json::arrayValue);
            for (const auto &ip : dto.getAllowedIps()) {
                ipsJson.append(ip);
            }
            apiKey.setAllowedIps(writer.write(ipsJson));

            // Set basic default fields
            apiKey.setIsActive(true);

            auto newApiKey = co_await mapper.insert(apiKey);

            response.success = true;
            response.message = "Business API key created successfully";
            response.result = utils::JsonUtils::convertKeysToCamelCase(newApiKey.toJson());
        } catch (const std::exception &e) {
            response.success = false;
            response.message = e.what();
        }
        co_return response;
    }

    drogon::Task<dto::BaseApiResponse> BusinessApiKeyService::revoke(const std::string &businessId, const std::string &id) {
        dto::BaseApiResponse response;
        try {
            auto dbClient = drogon::app().getDbClient();
            drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessApiKeys> mapper(dbClient);

            auto keys = co_await mapper.findBy(
                drogon::orm::Criteria(drogon_model::DataSupportDb::BusinessApiKeys::Cols::_id, drogon::orm::CompareOperator::EQ, id) &&
                drogon::orm::Criteria(drogon_model::DataSupportDb::BusinessApiKeys::Cols::_business_id, drogon::orm::CompareOperator::EQ, businessId)
            );

            if (keys.empty()) {
                response.success = false;
                response.message = "API key not found or unauthorized";
                response.error["code"] = constants::ErrorCode::ERR_DB_NOT_FOUND;
                co_return response;
            }

            auto apiKey = keys.front();
            apiKey.setIsActive(false);

            co_await mapper.update(apiKey);

            response.success = true;
            response.message = "Business API key revoked successfully";
        } catch (const std::exception &e) {
            response.success = false;
            response.message = e.what();
            response.error["code"] = constants::ErrorCode::ERR_INTERNAL;
        }
        co_return response;
    }

    drogon::Task<dto::BaseApiResponse> BusinessApiKeyService::activate(const std::string &businessId, const std::string &id) {
        dto::BaseApiResponse response;
        try {
            auto dbClient = drogon::app().getDbClient();
            drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessApiKeys> mapper(dbClient);

            auto keys = co_await mapper.findBy(
                drogon::orm::Criteria(drogon_model::DataSupportDb::BusinessApiKeys::Cols::_id, drogon::orm::CompareOperator::EQ, id) &&
                drogon::orm::Criteria(drogon_model::DataSupportDb::BusinessApiKeys::Cols::_business_id, drogon::orm::CompareOperator::EQ, businessId)
            );

            if (keys.empty()) {
                response.success = false;
                response.message = "API key not found or unauthorized";
                response.error["code"] = constants::ErrorCode::ERR_DB_NOT_FOUND;
                co_return response;
            }

            auto apiKey = keys.front();
            apiKey.setIsActive(true);

            co_await mapper.update(apiKey);

            response.success = true;
            response.message = "Business API key activated successfully";
        } catch (const std::exception &e) {
            response.success = false;
            response.message = e.what();
            response.error["code"] = constants::ErrorCode::ERR_INTERNAL;
        }
        co_return response;
    }

    drogon::Task<dto::BaseApiResponse> BusinessApiKeyService::deleteApiKey(const std::string &businessId, const std::string &id) {
        dto::BaseApiResponse response;
        try {
            auto dbClient = drogon::app().getDbClient();
            CoroMapper<drogon_model::DataSupportDb::BusinessApiKeys> mapper(dbClient);

            auto keys = co_await mapper.findBy(
                Criteria(drogon_model::DataSupportDb::BusinessApiKeys::Cols::_id, CompareOperator::EQ, id) &&
                Criteria(drogon_model::DataSupportDb::BusinessApiKeys::Cols::_business_id, CompareOperator::EQ, businessId)
            );

            if (keys.empty()) {
                response.success = false;
                response.message = "API key not found or unauthorized";
                response.error["code"] = constants::ErrorCode::ERR_DB_NOT_FOUND;
                co_return response;
            }

            co_await mapper.deleteByPrimaryKey(id);

            response.success = true;
            response.message = "Business API key deleted successfully";
        } catch (const std::exception &e) {
            response.success = false;
            response.message = e.what();
            response.error["code"] = constants::ErrorCode::ERR_INTERNAL;
        }
        co_return response;
    }

    drogon::Task<ApiCredentialsValidationResult> BusinessApiKeyService::validateApiCredentials(const std::string &clientId, const std::string &clientSecret) {
        ApiCredentialsValidationResult result;
        try {
            auto dbClient = drogon::app().getDbClient();
            CoroMapper<drogon_model::DataSupportDb::BusinessApiKeys> apiKeyMapper(dbClient);

            auto keys = co_await apiKeyMapper.findBy(
                Criteria(drogon_model::DataSupportDb::BusinessApiKeys::Cols::_client_id, CompareOperator::EQ, clientId) && Criteria(drogon_model::DataSupportDb::BusinessApiKeys::Cols::_is_active, CompareOperator::EQ, true)
            );

            if (keys.empty()) {
                result.isValid = false;
                result.errorMessage = "Invalid API credentials";
                result.errorCode = constants::ErrorCode::ERR_DB_NOT_FOUND;
                co_return result;
            }

            auto apiKey = keys.front();

            if (!bcrypt::validatePassword(clientSecret, apiKey.getValueOfClientSecretHash())) {
                result.isValid = false;
                result.errorMessage = "Invalid API credentials";
                result.errorCode = constants::ErrorCode::ERR_AUTH_INVALID_CREDENTIALS;
                co_return result;
            }

            const std::string& businessId = apiKey.getValueOfBusinessId();

            CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> accountMapper(dbClient);

            drogon_model::DataSupportDb::BusinessAccounts account;

            try {
                account = co_await accountMapper.findByPrimaryKey(businessId);
            } catch (const UnexpectedRows &) {
                result.isValid = false;
                result.errorCode = constants::ERR_RESOURCE_NOT_FOUND;
                result.errorMessage = "Business account not found.";
                co_return result;
            }

            result.isValid = true;
            result.accountId = account.getValueOfAccountId();
            result.businessId = businessId;
        } catch (const std::exception &e) {
            result.isValid = false;
            result.errorMessage = e.what();
            result.errorCode = constants::ErrorCode::ERR_INTERNAL;
        }
        co_return result;
    }
}
