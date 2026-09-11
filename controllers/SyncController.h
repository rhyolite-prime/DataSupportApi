#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class SyncController : public drogon::HttpController<SyncController>
{
  public:
    static constexpr const char *PREFIX = "/api/v1/sync/";

    METHOD_LIST_BEGIN

     ADD_METHOD_TO(SyncController::syncPosBranches,std::string(PREFIX) + "pos/branches", Post, Options);
     ADD_METHOD_TO(SyncController::syncPosCustomers,std::string(PREFIX) + "pos/customers", Post, Options);
     ADD_METHOD_TO(SyncController::syncPosInventoryCategories,std::string(PREFIX) + "pos/inventory-categories", Post, Options);
     ADD_METHOD_TO(SyncController::syncPosCoupons,std::string(PREFIX) + "pos/coupons", Post, Options);
     ADD_METHOD_TO(SyncController::syncErpCurrencies,std::string(PREFIX) + "erp/currencies", Post, Options);
     ADD_METHOD_TO(SyncController::syncPosDeliveryChannels,std::string(PREFIX) + "pos/delivery-channels", Post, Options);
     ADD_METHOD_TO(SyncController::syncPosPickupPoints,std::string(PREFIX) + "pos/pickup-points", Post, Options);
     ADD_METHOD_TO(SyncController::syncPosInventoryItems,std::string(PREFIX) + "pos/inventory-items", Post, Options);
     ADD_METHOD_TO(SyncController::syncErpApiKeys,std::string(PREFIX) + "erp/api-keys", Post, Options);

    METHOD_LIST_END

    Task<HttpResponsePtr> syncPosBranches(HttpRequestPtr req);
    Task<HttpResponsePtr> syncPosCustomers(HttpRequestPtr req);
    Task<HttpResponsePtr> syncPosInventoryCategories(HttpRequestPtr req);
    Task<HttpResponsePtr> syncPosCoupons(HttpRequestPtr req);
    Task<HttpResponsePtr> syncErpCurrencies(HttpRequestPtr req);
    Task<HttpResponsePtr> syncPosDeliveryChannels(HttpRequestPtr req);
    Task<HttpResponsePtr> syncPosPickupPoints(HttpRequestPtr req);
    Task<HttpResponsePtr> syncPosInventoryItems(HttpRequestPtr req);
    Task<HttpResponsePtr> syncErpApiKeys(HttpRequestPtr req);
};
