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