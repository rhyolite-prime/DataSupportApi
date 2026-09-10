#include "SyncController.h"

#include "dto/BaseApiResponse.h"
#include "plugins/DataSupportServicePlugin.h"


Task<HttpResponsePtr> SyncController::syncPosBranches(HttpRequestPtr req) {

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::erp_api::SyncPosBranchDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.syncPosBranches(dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}


Task<HttpResponsePtr> SyncController::syncPosCustomers(HttpRequestPtr req) {

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::erp_api::SyncPosBranchDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.syncPosBranches(dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}


Task<HttpResponsePtr> SyncController::syncPosInventoryCategories(HttpRequestPtr req) {

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::erp_api::SyncInventoryCategoryDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.syncPosInventoryCategories(dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}


Task<HttpResponsePtr> SyncController::syncPosCoupons(HttpRequestPtr req) {

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::erp_api::SyncPosCouponDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.syncPosCoupons(dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}


Task<HttpResponsePtr> SyncController::syncErpCurrencies(HttpRequestPtr req) {

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::erp_api::SyncErpCurrencyDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.syncErpCurrencies(dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}


Task<HttpResponsePtr> SyncController::syncPosDeliveryChannels(HttpRequestPtr req) {

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::erp_api::SyncPosDeliveryChannelDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.syncPosDeliveryChannels(dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}


Task<HttpResponsePtr> SyncController::syncPosPickupPoints(HttpRequestPtr req) {

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::erp_api::SyncPosPickupPointDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.syncPosPickupPoints(dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}


Task<HttpResponsePtr> SyncController::syncPosInventoryItems(HttpRequestPtr req) {

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::erp_api::SyncPosInventoryItemDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.syncPosInventoryItems(dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}


Task<HttpResponsePtr> SyncController::syncErpApiKeys(HttpRequestPtr req) {

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::erp_api::SyncApiCredentialDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.syncApiCredentials(dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}