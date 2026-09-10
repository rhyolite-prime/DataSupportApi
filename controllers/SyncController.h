#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class SyncController : public drogon::HttpController<SyncController>
{
  public:
    static constexpr const char *PREFIX = "/api/v1/sync/";

    METHOD_LIST_BEGIN

     ADD_METHOD_TO(SyncController::syncPosBranches,std::string(PREFIX) + "/pos/branches", Get, Options);
     ADD_METHOD_TO(SyncController::syncPosBranches,std::string(PREFIX) + "/pos/customers", Get, Options);
     ADD_METHOD_TO(SyncController::syncPosBranches,std::string(PREFIX) + "/pos/inventory-categories", Get, Options);
     ADD_METHOD_TO(SyncController::syncPosBranches,std::string(PREFIX) + "/pos/coupons", Get, Options);
     ADD_METHOD_TO(SyncController::syncPosBranches,std::string(PREFIX) + "/erp/currencies", Get, Options);
     ADD_METHOD_TO(SyncController::syncPosBranches,std::string(PREFIX) + "/pos/delivery-channels", Get, Options);
     ADD_METHOD_TO(SyncController::syncPosBranches,std::string(PREFIX) + "/pos/pickup-points", Get, Options);
     ADD_METHOD_TO(SyncController::syncPosBranches,std::string(PREFIX) + "/pos/inventory-items", Get, Options);
     ADD_METHOD_TO(SyncController::syncErpApiKeys,std::string(PREFIX) + "/erp/api-keys", Post, Options);

    METHOD_LIST_END

    Task<HttpResponsePtr> syncPosBranches(HttpRequestPtr req);
    Task<HttpResponsePtr> syncErpApiKeys(HttpRequestPtr req);
};
