/**
 *
 *  DataSupportServicePlugin.h
 *
 */

#pragma once

#include <drogon/plugins/Plugin.h>

#include "services/business_account/BusinessAccountService.h"
#include "services/business_api_keys/BusinessApiKeyService.h"
#include "services/hubtel_api/HubtelApi.h"
#include "services/quartz/QuartzApi.h"
#include "services/redis/RedisCacheManager.h"
#include "services/smart_suite_banking/SmartSuiteBankingService.h"

namespace data_support::plugins {

    class DataSupportServicePlugin : public drogon::Plugin<DataSupportServicePlugin>
    {
    public:

        DataSupportServicePlugin() = default;
        ~DataSupportServicePlugin() override = default;
        /// This method must be called by drogon to initialize and start the plugin.
        /// It must be implemented by the user.
        void initAndStart(const Json::Value &config) override;

        /// This method must be called by drogon to shutdown the plugin.
        /// It must be implemented by the user.
        void shutdown() override;

        services::QuartzApi &getQuartzApi() { return quartzApi_; }
        services::HubtelApi &getHubtelApi() { return hubtelApi_; }
        services::RedisCacheManager &getRedisCacheManager() { return redisCacheService_; }
        services::BusinessAccountService &getBusinessAccountService() { return businessAccountService_; }
        services::BusinessApiKeyService &getBusinessApiKeyService() { return businessApiKeyService_; }
        services::SmartSuiteBankingService &getSmartSuiteBankingService() { return smartSuiteBankingService_; }

    private:
        services::QuartzApi quartzApi_;
        services::HubtelApi hubtelApi_;
        services::RedisCacheManager redisCacheService_;
        services::BusinessAccountService businessAccountService_;
        services::BusinessApiKeyService businessApiKeyService_;
        services::SmartSuiteBankingService smartSuiteBankingService_;
    };

}


