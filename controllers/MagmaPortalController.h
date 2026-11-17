//
// MagmaNet — public captive-portal API (no API key, rate limited in services).
//

#ifndef DATASUPPORTAPI_MAGMAPORTALCONTROLLER_H
#define DATASUPPORTAPI_MAGMAPORTALCONTROLLER_H

#include <drogon/HttpController.h>

using namespace drogon;

class MagmaPortalController : public HttpController<MagmaPortalController> {
public:
    static constexpr const char *PREFIX = "/api/v1/magma/portal/";

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(MagmaPortalController::bootstrap, std::string(PREFIX) + "bootstrap", Get, Options);
    ADD_METHOD_TO(MagmaPortalController::plans, std::string(PREFIX) + "plans", Get, Options);
    ADD_METHOD_TO(MagmaPortalController::zoneStatus, std::string(PREFIX) + "status", Get, Options);
    ADD_METHOD_TO(MagmaPortalController::checkout, std::string(PREFIX) + "checkout", Post, Options);
    ADD_METHOD_TO(MagmaPortalController::checkoutStatus, std::string(PREFIX) + "checkout/{1}", Get, Options);
    ADD_METHOD_TO(MagmaPortalController::login, std::string(PREFIX) + "checkout/{1}/login", Post, Options);
    ADD_METHOD_TO(MagmaPortalController::redeemVoucher, std::string(PREFIX) + "vouchers/redeem", Post, Options);
    ADD_METHOD_TO(MagmaPortalController::createTicket, std::string(PREFIX) + "tickets", Post, Options);
    ADD_METHOD_TO(MagmaPortalController::agentLogin, std::string(PREFIX) + "agents/login", Post, Options);
    ADD_METHOD_TO(MagmaPortalController::agentPinResetRequest, std::string(PREFIX) + "agents/pin/reset/request", Post, Options);
    ADD_METHOD_TO(MagmaPortalController::agentPinReset, std::string(PREFIX) + "agents/pin/reset", Post, Options);
    METHOD_LIST_END

    Task<HttpResponsePtr> bootstrap(HttpRequestPtr req);
    Task<HttpResponsePtr> plans(HttpRequestPtr req);
    Task<HttpResponsePtr> zoneStatus(HttpRequestPtr req);
    Task<HttpResponsePtr> checkout(HttpRequestPtr req);
    Task<HttpResponsePtr> checkoutStatus(HttpRequestPtr req, std::string intentId);
    Task<HttpResponsePtr> login(HttpRequestPtr req, std::string intentId);
    Task<HttpResponsePtr> redeemVoucher(HttpRequestPtr req);
    Task<HttpResponsePtr> createTicket(HttpRequestPtr req);
    Task<HttpResponsePtr> agentLogin(HttpRequestPtr req);
    Task<HttpResponsePtr> agentPinResetRequest(HttpRequestPtr req);
    Task<HttpResponsePtr> agentPinReset(HttpRequestPtr req);
};

#endif //DATASUPPORTAPI_MAGMAPORTALCONTROLLER_H
