//
// MagmaNet — shared helpers for the MagmaNet HTTP controllers.
//

#ifndef DATASUPPORTAPI_MAGMACONTROLLERSUPPORT_H
#define DATASUPPORTAPI_MAGMACONTROLLERSUPPORT_H

#include <drogon/drogon.h>
#include <json/json.h>

#include <string>

#include "dto/BaseApiResponse.h"
#include "plugins/DataSupportServicePlugin.h"
#include "utils/DbUtils.h"

namespace data_support::magma {

    struct AdminAuth {
        bool ok = false;
        std::string businessId;
        drogon::HttpResponsePtr error;
    };

    /** ClientId/ClientSecret authentication, matching ThirdPartyController. */
    inline drogon::Task<AdminAuth> authenticateAdmin(drogon::HttpRequestPtr req) {
        AdminAuth auth;
        const auto clientId = req->getHeader("ClientId");
        const auto clientSecret = req->getHeader("ClientSecret");
        if (clientId.empty() || clientSecret.empty()) {
            auth.error = drogon::HttpResponse::newHttpJsonResponse(
                utils::errorResponse("ClientId and ClientSecret headers are required").toJson());
            auth.error->setStatusCode(drogon::k400BadRequest);
            co_return auth;
        }

        auto plugin = drogon::app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
        if (!plugin) {
            auth.error = drogon::HttpResponse::newHttpJsonResponse(
                utils::errorResponse("Service plugin is not running").toJson());
            auth.error->setStatusCode(drogon::k500InternalServerError);
            co_return auth;
        }
        auto identity = co_await plugin->getBusinessApiKeyService()
                            .validateApiCredentials(clientId, clientSecret);
        if (!identity.isValid) {
            auth.error = drogon::HttpResponse::newHttpJsonResponse(
                utils::errorResponse(identity.errorMessage.empty() ? "Invalid API credentials"
                                                                   : identity.errorMessage).toJson());
            auth.error->setStatusCode(drogon::k401Unauthorized);
            co_return auth;
        }

        auth.ok = true;
        auth.businessId = identity.businessId;
        co_return auth;
    }

    inline drogon::HttpResponsePtr toResponse(const dto::BaseApiResponse &response,
                                              drogon::HttpStatusCode status = drogon::k200OK) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(status);
        return resp;
    }

    inline drogon::HttpResponsePtr jsonGuard() {
        return toResponse(utils::errorResponse("Invalid JSON body"), drogon::k400BadRequest);
    }

    inline Json::Value bodyJson(const drogon::HttpRequestPtr &req) {
        const auto json = req->getJsonObject();
        return json ? *json : Json::Value(Json::objectValue);
    }

    inline std::string queryValue(const drogon::HttpRequestPtr &req, const std::string &name,
                                  const std::string &fallback = "") {
        const std::string value = req->getParameter(name);
        return value.empty() ? fallback : value;
    }

    inline int queryInt(const drogon::HttpRequestPtr &req, const std::string &name, int fallback) {
        const std::string value = req->getParameter(name);
        if (value.empty()) {
            return fallback;
        }
        try {
            return std::stoi(value);
        } catch (...) {
            return fallback;
        }
    }

} // namespace data_support::magma

#endif //DATASUPPORTAPI_MAGMACONTROLLERSUPPORT_H
