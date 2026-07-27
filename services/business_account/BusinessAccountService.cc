//
// Created by Emmanuel Addo-Odame on 22/07/2026.
//

#include "BusinessAccountService.h"
#include "models/BusinessAccounts.h"
#include <drogon/drogon.h>
#include <drogon/orm/CoroMapper.h>
#include "utils/JsonUtils.h"

namespace data_support::services {

drogon::Task<dto::BaseApiResponse> BusinessAccountService::getAll(int pageNo, int pageSize, const std::string &query) {
    dto::BaseApiResponse response;
    try {
        auto dbClient = drogon::app().getDbClient();
        drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> mapper(dbClient);

        // Simple pagination
        auto accounts = co_await mapper.paginate(pageNo, pageSize).findAll();

        Json::Value result(Json::arrayValue);
        for (const auto &account : accounts) {
            result.append(utils::JsonUtils::convertKeysToCamelCase(account.toJson()));
        }

        response.success = true;
        response.message = "Business accounts retrieved successfully";
        response.result = result;
    } catch (const std::exception &e) {
        response.success = false;
        response.message = e.what();
    }
    co_return response;
}

drogon::Task<dto::BaseApiResponse> BusinessAccountService::create(const dto::BusinessAccountDto &dto) {
    dto::BaseApiResponse response;
    try {
        auto dbClient = drogon::app().getDbClient();
        drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> mapper(dbClient);

        drogon_model::DataSupportDb::BusinessAccounts account;
        account.setBusinessName(dto.getBusinessName());
        account.setAccountId(dto.getAccountId());
        account.setContactPersonName(dto.getContactPersonName());
        account.setContactPersonEmail(dto.getContactPersonEmail());
        account.setContactPersonPhoneNo(dto.getContactPersonPhoneNo());
        account.setBusinessEmail(dto.getBusinessEmail());
        account.setBusinessPhoneNo(dto.getBusinessPhoneNo());
        account.setStatus(dto.getStatus());
        account.setSubAccountEnabled(dto.isSubAccountEnabled());
        account.setRequireTwoFactorAuth(dto.isRequireTwoFactorAuth());
        account.setCountry(dto.getCountry());
        account.setCurrency(dto.getCurrency());
        
        if (!dto.getSubscriptionModel().isNull()) {
            Json::FastWriter writer;
            account.setSubscriptionModel(writer.write(dto.getSubscriptionModel()));
        }

        auto newAccount = co_await mapper.insert(account);

        response.success = true;
        response.message = "Business account created successfully";
        response.result = newAccount.toJson();
    } catch (const std::exception &e) {
        response.success = false;
        response.message = e.what();
    }
    co_return response;
}

drogon::Task<dto::BaseApiResponse> BusinessAccountService::update(const dto::BusinessAccountDto &dto, const std::string &id) {
    dto::BaseApiResponse response;
    try {
        auto dbClient = drogon::app().getDbClient();
        drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> mapper(dbClient);

        auto account = co_await mapper.findByPrimaryKey(id);
        
        account.setBusinessName(dto.getBusinessName());
        account.setContactPersonName(dto.getContactPersonName());
        account.setContactPersonEmail(dto.getContactPersonEmail());
        account.setContactPersonPhoneNo(dto.getContactPersonPhoneNo());
        account.setBusinessEmail(dto.getBusinessEmail());
        account.setBusinessPhoneNo(dto.getBusinessPhoneNo());
        account.setStatus(dto.getStatus());
        account.setSubAccountEnabled(dto.isSubAccountEnabled());
        account.setRequireTwoFactorAuth(dto.isRequireTwoFactorAuth());
        account.setCountry(dto.getCountry());
        account.setCurrency(dto.getCurrency());

        if (!dto.getSubscriptionModel().isNull()) {
            Json::FastWriter writer;
            account.setSubscriptionModel(writer.write(dto.getSubscriptionModel()));
        }

        auto updatedAccount = co_await mapper.update(account);

        response.success = true;
        response.message = "Business account updated successfully";
        response.result = account.toJson();
    } catch (const std::exception &e) {
        response.success = false;
        response.message = e.what();
    }
    co_return response;
}

drogon::Task<dto::BaseApiResponse> BusinessAccountService::updateAccountStatus(const std::string &id, constants::StatusType status) {
    dto::BaseApiResponse response;
    try {
        auto dbClient = drogon::app().getDbClient();
        drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> mapper(dbClient);

        auto account = co_await mapper.findByPrimaryKey(id);
        account.setStatus(static_cast<int32_t>(status));

        co_await mapper.update(account);

        response.success = true;
        response.message = "Business account status updated successfully";
    } catch (const std::exception &e) {
        response.success = false;
        response.message = e.what();
    }
    co_return response;
}

drogon::Task<dto::BaseApiResponse> BusinessAccountService::deleteAccount(const std::string &id) {
    dto::BaseApiResponse response;
    try {
        auto dbClient = drogon::app().getDbClient();
        drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> mapper(dbClient);

        co_await mapper.deleteByPrimaryKey(id);

        response.success = true;
        response.message = "Business account deleted successfully";
    } catch (const std::exception &e) {
        response.success = false;
        response.message = e.what();
    }
    co_return response;
}

} // namespace data_support::services


