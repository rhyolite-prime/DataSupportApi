//
// Created by Emmanuel Addo-Odame on 23/08/2026.
//
#include "ErpApi.h"
#include "dto/BaseApiResponse.h"
#include "models/BusinessAccounts.h"
#include <drogon/drogon.h>
#include <drogon/orm/CoroMapper.h>

#include "PosBranches.h"

namespace data_support::services {

    drogon::Task<dto::BaseApiResponse> ErpApi::getBranches() {

        //rely on synced data
        dto::BaseApiResponse res;
        co_return res;

    }

    drogon::Task<dto::BaseApiResponse> ErpApi::getDeliveryChannels() {

        //rely on synced data
        dto::BaseApiResponse res;
        co_return res;

    }

    drogon::Task<dto::BaseApiResponse> ErpApi::getCoupons() {

        //rely on synced data
        dto::BaseApiResponse res;
        co_return res;


    }

    drogon::Task<dto::BaseApiResponse> ErpApi::getPickupPoints()
    {
        dto::BaseApiResponse res;
        co_return res;

    }

    drogon::Task<dto::BaseApiResponse> ErpApi::getCategories() {

        //rely on synced data
        dto::BaseApiResponse res;
        co_return res;

    }

    drogon::Task<dto::BaseApiResponse> ErpApi::getCurrencies() {

        //rely on synced data
        dto::BaseApiResponse res;
        co_return res;

    }

    //auth

    drogon::Task<dto::BaseApiResponse> ErpApi::verifyCustomer() {

        // direct call
        dto::BaseApiResponse res;
        co_return res;

    }

    drogon::Task<dto::BaseApiResponse> ErpApi::signup() {

        // direct call
        dto::BaseApiResponse res;
        co_return res;
    }


    drogon::Task<dto::BaseApiResponse> ErpApi::login() {

    // direct call
        dto::BaseApiResponse res;
        co_return res;
    }

    drogon::Task<dto::BaseApiResponse> ErpApi::resetPassword() {

        // direct call
        dto::BaseApiResponse res;
        co_return res;
    }

    drogon::Task<dto::BaseApiResponse> ErpApi::verifyOtp() {

        // direct call
        dto::BaseApiResponse res;
        co_return res;
    }

    drogon::Task<dto::BaseApiResponse> ErpApi::addItemToCart() {

        // direct call
        dto::BaseApiResponse res;
        co_return res;

    }

    drogon::Task<dto::BaseApiResponse> ErpApi::addMultipleItemsToCart() {

        // direct call
        dto::BaseApiResponse res;
        co_return res;

    }


    drogon::Task<dto::BaseApiResponse> ErpApi::removeItemFromCart() {

        // direct call
        dto::BaseApiResponse res;
        co_return res;

    }

    drogon::Task<dto::BaseApiResponse> ErpApi::removeMultipleItemsFromCart() {

        // direct call
        dto::BaseApiResponse res;
        co_return res;

    }

    drogon::Task<dto::BaseApiResponse> ErpApi::clearCart() {

        // direct call
        dto::BaseApiResponse res;
        co_return res;

    }


    //checkout

    drogon::Task<dto::BaseApiResponse> ErpApi::initializeWebCheckout() {

        // direct call
        dto::BaseApiResponse res;
        co_return res;

    }

    drogon::Task<dto::BaseApiResponse> ErpApi::initializeUssdCheckout() {

        // direct call
        dto::BaseApiResponse res;
        co_return res;

    }

    drogon::Task<dto::BaseApiResponse> ErpApi::processPaymentCallback() {

        // direct call
        dto::BaseApiResponse res;
        co_return res;

    }

    drogon::Task<dto::BaseApiResponse> ErpApi::getInventoryItems() {

        //rely on synced data
        dto::BaseApiResponse res;
        co_return res;

    }

    drogon::Task<dto::BaseApiResponse> ErpApi::getOrderHistory() {

        // direct call
        dto::BaseApiResponse res;
        co_return res;

    }

    drogon::Task<dto::BaseApiResponse> ErpApi::getUserProfile() {

        // direct call
        dto::BaseApiResponse res;
        co_return res;

    }

    drogon::Task<dto::BaseApiResponse> ErpApi::changePassword() {

        // direct call
        dto::BaseApiResponse res;
        co_return res;

    }

    drogon::Task<dto::BaseApiResponse> ErpApi::updateUserProfile() {

        // direct call

        dto::BaseApiResponse res;
        co_return res;
    }

    drogon::Task<dto::BaseApiResponse> ErpApi::getNotifications() {

        //rely on synced data
        dto::BaseApiResponse res;
        co_return res;

    }


    //------- SYNC API CREDENTIALS ----------

    drogon::Task<dto::BaseApiResponse> ErpApi::syncApiCredentials(const dto::erp_api::SyncApiCredentialDto &dto) {

        dto::BaseApiResponse response;

        try {
            auto dbClient = drogon::app().getDbClient();
            drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> mapper(dbClient);

            auto account = co_await mapper.findByPrimaryKey(dto.getBusinessAccountId());
            
            std::string configStr = account.getValueOfConfigurationData();
            Json::Value configData;
            Json::CharReaderBuilder readerBuilder;
            std::string errs;

            if (!configStr.empty()) {
                std::istringstream iss(configStr);
                if (!Json::parseFromStream(readerBuilder, iss, &configData, &errs)) {
                    LOG_ERROR << "Failed to parse ConfigurationData: " << errs;
                    response.message = "Configuration error.";
                    response.success = false;
                    co_return response;
                }
            } else {
                configData = Json::Value(Json::objectValue);
            }

            if (!configData.isMember("ErpApi") || !configData["ErpApi"].isObject()) {
                configData["ErpApi"] = Json::Value(Json::objectValue);
            }

            configData["ErpApi"]["AccountId"] = dto.getAccountId();
            configData["ErpApi"]["AccountSecret"] = dto.getAccountSecret();

            Json::StreamWriterBuilder writer;
            writer["indentation"] = "";
            account.setConfigurationData(Json::writeString(writer, configData));

            co_await mapper.update(account);

            response.message = "API credentials synced successfully";
            response.success = true;

        } catch (const std::exception &e) {
            LOG_ERROR << "Error syncing API credentials: " << e.what();

            response.message = "Failed to sync API credentials. Please try again.";
            response.success = false;
        }

        co_return response;
    }


    drogon::Task<dto::BaseApiResponse> ErpApi::syncPosBranches(const dto::erp_api::SyncPosBranchDto &dto) {
    dto::BaseApiResponse response;

    try {
        auto dbClient = drogon::app().getDbClient();
        drogon::orm::CoroMapper<drogon_model::DataSupportDb::PosBranches> mapper(dbClient);

        // Get data from DTO
        std::string businessAccountId = dto.getBusinessAccountId();
        const std::string& operation = dto.getOperation();

        // Create model object
        drogon_model::DataSupportDb::PosBranches branch;

        // Set all fields
        branch.setId(dto.getId());
        branch.setBusinessId(dto.getBusinessAccountId());
        branch.setName(dto.getName());
        branch.setLocation(dto.getLocation());
        branch.setCountryName(dto.getCountryName());
        branch.setCountryCode(dto.getCountryCode());
        branch.setRegionOrState(dto.getRegionOrState());
        branch.setCity(dto.getCity());
        branch.setAddress(dto.getAddress());
        branch.setPhoneNo(dto.getPhoneNo());
        branch.setBranchManager(dto.getBranchManager());
        branch.setRevenueAccountId(dto.getRevenueAccountId());
        branch.setRevenueAccountNo(dto.getRevenueAccountNo());
        branch.setGeoCoordinates(dto.getGeoCoordinates());
        branch.setIsDefault(dto.getIsDefault());
        branch.setTenantId(dto.getTenantId());

        if (operation == "create") {

            co_await mapper.insert(branch);

            response.message = "Branch created successfully";
            response.success = true;
            LOG_INFO << "Created branch: " << dto.getId() << " for business: " << businessAccountId;

        } else if (operation == "update") {
            // Update existing branch
            branch.setUpdatedAt(trantor::Date::now());
            co_await mapper.update(branch);

            response.message = "Branch updated successfully";
            response.success = true;
            LOG_INFO << "Updated branch: " << dto.getId() << " for business: " << businessAccountId;

        } else if (operation == "delete") {

            co_await mapper.deleteByPrimaryKey(dto.getId());

            response.message = "Branch deleted successfully";
            response.success = true;
            LOG_INFO << "Deleted branch: " << dto.getId() << " for business: " << businessAccountId;

        } else {
            response.message = "Unsupported operation: " + operation;
            response.success = false;
            LOG_WARN << "Unsupported operation: " << operation << " for branch: " << dto.getId();
        }


    } catch (const drogon::orm::UnexpectedRows &e) {
        // This is expected when findByPrimaryKey doesn't find a record
        // We handle this in the create/update logic
        LOG_DEBUG << "Branch not found: " << e.what();
    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "Database error in syncPosBranches: " << e.base().what();
        response.message = "Database error occurred while syncing branch";
        response.success = false;
    } catch (const std::exception &e) {
        LOG_ERROR << "Error in syncPosBranches: " << e.what();
        response.message = "Failed to sync branch. Please try again.";
        response.success = false;
    }

    co_return response;
}




}
