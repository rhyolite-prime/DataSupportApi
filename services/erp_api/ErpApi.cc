//
// Created by Emmanuel Addo-Odame on 23/08/2026.
//
#include "ErpApi.h"
#include "dto/BaseApiResponse.h"
#include "models/BusinessAccounts.h"
#include <drogon/drogon.h>
#include <drogon/orm/CoroMapper.h>

#include "PosBranches.h"
#include "PosInventoryCategories.h"

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

    LOG_INFO << "[syncPosBranches] Entering method. Operation: '" << dto.getOperation()
             << "', BusinessAccountId: '" << dto.getBusinessAccountId()
             << "', BranchId: '" << dto.getId() << "'";

        try {
            auto dbClient = drogon::app().getDbClient();
            if (!dbClient) {
                LOG_ERROR << "[syncPosBranches] Failed to get database client!";
                response.message = "Database client unavailable.";
                response.success = false;
                co_return response;
            }
            LOG_DEBUG << "[syncPosBranches] Database client obtained.";

            drogon::orm::CoroMapper<drogon_model::DataSupportDb::PosBranches> mapper(dbClient);
            LOG_DEBUG << "[syncPosBranches] CoroMapper created.";

            // Get data from DTO
            std::string businessAccountId = dto.getBusinessAccountId();
            const std::string& operation = dto.getOperation();

            LOG_DEBUG << "[syncPosBranches] Populating branch model with DTO data...";

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

            LOG_DEBUG << "[syncPosBranches] Branch model populated. ID: " << branch.getValueOfId()
                      << ", Name: " << branch.getValueOfName();

            if (operation == "create") {
                LOG_INFO << "[syncPosBranches] Attempting to INSERT branch: " << dto.getId();

                co_await mapper.insert(branch);

                LOG_INFO << "[syncPosBranches] INSERT successful for branch: " << dto.getId();
                response.message = "Branch created successfully";
                response.success = true;

            } else if (operation == "update") {
                LOG_INFO << "[syncPosBranches] Attempting to UPDATE branch: " << dto.getId();

                branch.setUpdatedAt(trantor::Date::now());
                co_await mapper.update(branch);

                LOG_INFO << "[syncPosBranches] UPDATE successful for branch: " << dto.getId();
                response.message = "Branch updated successfully";
                response.success = true;

            } else if (operation == "delete") {
                LOG_INFO << "[syncPosBranches] Attempting to DELETE branch: " << dto.getId();

                co_await mapper.deleteByPrimaryKey(dto.getId());

                LOG_INFO << "[syncPosBranches] DELETE successful for branch: " << dto.getId();
                response.message = "Branch deleted successfully";
                response.success = true;

            } else {
                LOG_WARN << "[syncPosBranches] Unsupported operation: '" << operation
                         << "' for branch: " << dto.getId();
                response.message = "Unsupported operation: " + operation;
                response.success = false;
            }

        } catch (const drogon::orm::UnexpectedRows &e) {
            // This exception is thrown by findByPrimaryKey when no record is found.
            // It is NOT expected for insert/update/delete. If it occurs here, something is wrong.
            LOG_ERROR << "[syncPosBranches] UnexpectedRows exception caught: " << e.what()
                      << ". This may indicate a missing record for an update/delete operation.";
            response.message = "Record not found or database inconsistency: " + std::string(e.what());
            response.success = false;

        } catch (const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << "[syncPosBranches] Database error: " << e.base().what();
            response.message = "Database error occurred while syncing branch: " + std::string(e.base().what());
            response.success = false;

        } catch (const std::exception &e) {
            LOG_ERROR << "[syncPosBranches] General exception: " << e.what();
            response.message = "Failed to sync branch. Please try again. Error: " + std::string(e.what());
            response.success = false;
        }

        LOG_INFO << "[syncPosBranches] Exiting method. Success: " << response.success
             << ", Message: '" << response.message << "'";

        co_return response;
    }

    drogon::Task<dto::BaseApiResponse> ErpApi::syncPosInventoryCategories(const dto::erp_api::SyncInventoryCategoryDto &dto) {

    dto::BaseApiResponse response;

    LOG_INFO << "[syncPosInventoryCategories] Entering method. Operation: '" << dto.getOperation()
             << "', BusinessAccountId: '" << dto.getBusinessAccountId()
             << "', CategoryId: '" << dto.getId() << "'";

    try {
        auto dbClient = drogon::app().getDbClient();
        if (!dbClient) {
            LOG_ERROR << "[syncPosInventoryCategories] Failed to get database client!";
            response.message = "Database client unavailable.";
            response.success = false;
            co_return response;
        }
        LOG_DEBUG << "[syncPosInventoryCategories] Database client obtained.";

        drogon::orm::CoroMapper<drogon_model::DataSupportDb::PosInventoryCategories> mapper(dbClient);
        LOG_DEBUG << "[syncPosInventoryCategories] CoroMapper created.";

        // Get envelope fields
        const std::string& businessAccountId = dto.getBusinessAccountId();
        const std::string& operation         = dto.getOperation();

        if (businessAccountId.empty()) {
            LOG_WARN << "[syncPosInventoryCategories] BusinessAccountId is empty. Aborting.";
            response.message = "BusinessAccountId is required.";
            response.success = false;
            co_return response;
        }

        if (dto.getId().empty()) {
            LOG_WARN << "[syncPosInventoryCategories] Category Id is empty. Aborting.";
            response.message = "Category Id is required.";
            response.success = false;
            co_return response;
        }

        // ---------------------------------------------------------------
        // DELETE — handled first because we only need the primary key
        // ---------------------------------------------------------------
        if (operation == "delete") {
            LOG_INFO << "[syncPosInventoryCategories] Attempting to DELETE category: "
                     << dto.getId();

            auto deletedCount = co_await mapper.deleteByPrimaryKey(dto.getId());

            LOG_INFO << "[syncPosInventoryCategories] DELETE successful for category: "
                     << dto.getId() << ", rows affected: " << deletedCount;

            response.message = "Category deleted successfully";
            response.success = true;
            co_return response;
        }

        // ---------------------------------------------------------------
        // BUILD MODEL for create / update
        // ---------------------------------------------------------------
        LOG_DEBUG << "[syncPosInventoryCategories] Populating category model with DTO data...";

        drogon_model::DataSupportDb::PosInventoryCategories category;

        category.setId(dto.getId());
        category.setBusinessId(businessAccountId);
        category.setName(dto.getName());
        category.setSlug(dto.getSlug());
        category.setDescription(dto.getDescription());
        category.setParentCategoryId(dto.getParentCategoryId());
        category.setIsService(dto.getIsService());
        category.setProductCount(dto.getProductCount());
        category.setTenantId(dto.getTenantId());

        LOG_DEBUG << "[syncPosInventoryCategories] Category model populated. ID: "
                  << category.getValueOfId()
                  << ", Name: " << category.getValueOfName();

        // ---------------------------------------------------------------
        // CREATE
        // ---------------------------------------------------------------
        if (operation == "create") {
            LOG_INFO << "[syncPosInventoryCategories] Attempting to INSERT category: "
                     << dto.getId();

            category.setCreatedAt(trantor::Date::now());
            co_await mapper.insert(category);

            LOG_INFO << "[syncPosInventoryCategories] INSERT successful for category: "
                     << dto.getId();

            response.message = "Category created successfully";
            response.success = true;
            co_return response;
        }

        // ---------------------------------------------------------------
        // UPDATE
        // ---------------------------------------------------------------
        if (operation == "update") {
            LOG_INFO << "[syncPosInventoryCategories] Attempting to UPDATE category: "
                     << dto.getId();

            category.setUpdatedAt(trantor::Date::now());

            // update() uses the primary key set on the model, so make sure it's there
            auto updatedCount = co_await mapper.update(category);

            LOG_INFO << "[syncPosInventoryCategories] UPDATE successful for category: "
                     << dto.getId() << ", rows affected: " << updatedCount;

            response.message = "Category updated successfully";
            response.success = true;
            co_return response;
        }

        // ---------------------------------------------------------------
        // UNKNOWN OPERATION
        // ---------------------------------------------------------------
        LOG_WARN << "[syncPosInventoryCategories] Unsupported operation: '" << operation
                 << "' for category: " << dto.getId();
        response.message = "Unsupported operation: " + operation;
        response.success = false;

    } catch (const drogon::orm::UnexpectedRows &e) {
        LOG_ERROR << "[syncPosInventoryCategories] UnexpectedRows exception: " << e.what()
                  << ". The category may not exist for update.";
        response.message = "Record not found or database inconsistency: " + std::string(e.what());
        response.success = false;

    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << "[syncPosInventoryCategories] Database error: " << e.base().what();
        response.message = "Database error occurred while syncing category: "
                           + std::string(e.base().what());
        response.success = false;

    } catch (const std::exception &e) {
        LOG_ERROR << "[syncPosInventoryCategories] General exception: " << e.what();
        response.message = "Failed to sync category. Please try again. Error: "
                           + std::string(e.what());
        response.success = false;
    }

    LOG_INFO << "[syncPosInventoryCategories] Exiting method. Success: " << response.success
             << ", Message: '" << response.message << "'";

    co_return response;
}




}
