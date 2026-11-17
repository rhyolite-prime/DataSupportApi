//
// MagmaNet — tenant administration API (API-key protected).
//

#ifndef DATASUPPORTAPI_MAGMAADMINCONTROLLER_H
#define DATASUPPORTAPI_MAGMAADMINCONTROLLER_H

#include <drogon/HttpController.h>

using namespace drogon;

class MagmaAdminController : public HttpController<MagmaAdminController> {
public:
    static constexpr const char *PREFIX = "/api/v1/magma/admin/";

    METHOD_LIST_BEGIN
    // overview
    ADD_METHOD_TO(MagmaAdminController::overview, std::string(PREFIX) + "overview", Get, Options);
    ADD_METHOD_TO(MagmaAdminController::capacity, std::string(PREFIX) + "capacity", Get, Options);
    ADD_METHOD_TO(MagmaAdminController::networkOverview, std::string(PREFIX) + "network-overview", Get, Options);

    // zones
    ADD_METHOD_TO(MagmaAdminController::listZones, std::string(PREFIX) + "zones", Get, Options);
    ADD_METHOD_TO(MagmaAdminController::createZone, std::string(PREFIX) + "zones", Post, Options);
    ADD_METHOD_TO(MagmaAdminController::getZone, std::string(PREFIX) + "zones/{1}", Get, Options);
    ADD_METHOD_TO(MagmaAdminController::updateZone, std::string(PREFIX) + "zones/{1}", Put, Options);
    ADD_METHOD_TO(MagmaAdminController::zoneHealth, std::string(PREFIX) + "zones/{1}/health", Get, Options);
    ADD_METHOD_TO(MagmaAdminController::listZoneCosts, std::string(PREFIX) + "zones/{1}/costs", Get, Options);
    ADD_METHOD_TO(MagmaAdminController::recordZoneCost, std::string(PREFIX) + "zones/{1}/costs", Post, Options);
    ADD_METHOD_TO(MagmaAdminController::zonePnl, std::string(PREFIX) + "zones/{1}/pnl", Get, Options);

    // plans
    ADD_METHOD_TO(MagmaAdminController::listPlans, std::string(PREFIX) + "plans", Get, Options);
    // Compatibility alias for clients that still call the older name.
    ADD_METHOD_TO(MagmaAdminController::listPlans, std::string(PREFIX) + "get-internet-plans", Get, Options);
    ADD_METHOD_TO(MagmaAdminController::createPlan, std::string(PREFIX) + "plans", Post, Options);
    ADD_METHOD_TO(MagmaAdminController::updatePlan, std::string(PREFIX) + "plans/{1}", Put, Options);
    ADD_METHOD_TO(MagmaAdminController::deactivatePlan, std::string(PREFIX) + "plans/{1}", Delete, Options);
    ADD_METHOD_TO(MagmaAdminController::planAvailability, std::string(PREFIX) + "plans/{1}/availability", Get, Options);
    ADD_METHOD_TO(MagmaAdminController::simulatePlan, std::string(PREFIX) + "plans/{1}/simulate", Post, Options);

    // subscribers
    ADD_METHOD_TO(MagmaAdminController::listSubscribers, std::string(PREFIX) + "subscribers", Get, Options);
    ADD_METHOD_TO(MagmaAdminController::createSubscriber, std::string(PREFIX) + "subscribers", Post, Options);
    ADD_METHOD_TO(MagmaAdminController::getSubscriber, std::string(PREFIX) + "subscribers/{1}", Get, Options);
    ADD_METHOD_TO(MagmaAdminController::updateSubscriber, std::string(PREFIX) + "subscribers/{1}", Put, Options);
    ADD_METHOD_TO(MagmaAdminController::subscriberUsage, std::string(PREFIX) + "subscribers/{1}/usage", Get, Options);
    ADD_METHOD_TO(MagmaAdminController::subscriberSessions, std::string(PREFIX) + "subscribers/{1}/sessions", Get, Options);
    ADD_METHOD_TO(MagmaAdminController::subscriberDevices, std::string(PREFIX) + "subscribers/{1}/devices", Get, Options);
    ADD_METHOD_TO(MagmaAdminController::suspendSubscriber, std::string(PREFIX) + "subscribers/{1}/suspend", Post, Options);
    ADD_METHOD_TO(MagmaAdminController::resumeSubscriber, std::string(PREFIX) + "subscribers/{1}/resume", Post, Options);
    ADD_METHOD_TO(MagmaAdminController::churnSubscriber, std::string(PREFIX) + "subscribers/{1}/churn", Post, Options);
    ADD_METHOD_TO(MagmaAdminController::blockDevice, std::string(PREFIX) + "devices/block", Post, Options);
    ADD_METHOD_TO(MagmaAdminController::unblockDevice, std::string(PREFIX) + "devices/unblock", Post, Options);

    // subscriptions and money
    ADD_METHOD_TO(MagmaAdminController::getSubscription, std::string(PREFIX) + "subscriptions/{1}", Get, Options);
    ADD_METHOD_TO(MagmaAdminController::extendSubscription, std::string(PREFIX) + "subscriptions/{1}/extend", Post, Options);
    ADD_METHOD_TO(MagmaAdminController::terminateSubscription, std::string(PREFIX) + "subscriptions/{1}/terminate", Post, Options);
    ADD_METHOD_TO(MagmaAdminController::listTransactions, std::string(PREFIX) + "transactions", Get, Options);
    ADD_METHOD_TO(MagmaAdminController::verifyTransaction, std::string(PREFIX) + "transactions/{1}/verify", Post, Options);
    ADD_METHOD_TO(MagmaAdminController::refundTransaction, std::string(PREFIX) + "transactions/{1}/refund", Post, Options);
    ADD_METHOD_TO(MagmaAdminController::revenue, std::string(PREFIX) + "revenue", Get, Options);
    ADD_METHOD_TO(MagmaAdminController::runInvoices, std::string(PREFIX) + "invoices/run", Post, Options);
    ADD_METHOD_TO(MagmaAdminController::suspendOverdue, std::string(PREFIX) + "suspend-overdue", Post, Options);
    ADD_METHOD_TO(MagmaAdminController::closeTill, std::string(PREFIX) + "till/close", Post, Options);
    ADD_METHOD_TO(MagmaAdminController::reconcile, std::string(PREFIX) + "reconcile", Post, Options);

    // devices
    ADD_METHOD_TO(MagmaAdminController::listDevices, std::string(PREFIX) + "devices", Get, Options);
    ADD_METHOD_TO(MagmaAdminController::registerDevice, std::string(PREFIX) + "devices", Post, Options);
    ADD_METHOD_TO(MagmaAdminController::getDevice, std::string(PREFIX) + "devices/{1}", Get, Options);
    ADD_METHOD_TO(MagmaAdminController::decommissionDevice, std::string(PREFIX) + "devices/{1}", Delete, Options);
    ADD_METHOD_TO(MagmaAdminController::pushConfig, std::string(PREFIX) + "devices/{1}/config", Post, Options);
    ADD_METHOD_TO(MagmaAdminController::rebootDevice, std::string(PREFIX) + "devices/{1}/reboot", Post, Options);
    ADD_METHOD_TO(MagmaAdminController::deviceDiagnostics, std::string(PREFIX) + "devices/{1}/diagnostics", Get, Options);
    ADD_METHOD_TO(MagmaAdminController::pollDevices, std::string(PREFIX) + "devices/poll", Post, Options);
    METHOD_LIST_END

    Task<HttpResponsePtr> overview(HttpRequestPtr req);
    Task<HttpResponsePtr> capacity(HttpRequestPtr req);
    Task<HttpResponsePtr> networkOverview(HttpRequestPtr req);

    Task<HttpResponsePtr> listZones(HttpRequestPtr req);
    Task<HttpResponsePtr> createZone(HttpRequestPtr req);
    Task<HttpResponsePtr> getZone(HttpRequestPtr req, std::string zoneId);
    Task<HttpResponsePtr> updateZone(HttpRequestPtr req, std::string zoneId);
    Task<HttpResponsePtr> zoneHealth(HttpRequestPtr req, std::string zoneId);
    Task<HttpResponsePtr> listZoneCosts(HttpRequestPtr req, std::string zoneId);
    Task<HttpResponsePtr> recordZoneCost(HttpRequestPtr req, std::string zoneId);
    Task<HttpResponsePtr> zonePnl(HttpRequestPtr req, std::string zoneId);

    Task<HttpResponsePtr> listPlans(HttpRequestPtr req);
    Task<HttpResponsePtr> createPlan(HttpRequestPtr req);
    Task<HttpResponsePtr> updatePlan(HttpRequestPtr req, std::string planId);
    Task<HttpResponsePtr> deactivatePlan(HttpRequestPtr req, std::string planId);
    Task<HttpResponsePtr> planAvailability(HttpRequestPtr req, std::string planId);
    Task<HttpResponsePtr> simulatePlan(HttpRequestPtr req, std::string planId);

    Task<HttpResponsePtr> listSubscribers(HttpRequestPtr req);
    Task<HttpResponsePtr> createSubscriber(HttpRequestPtr req);
    Task<HttpResponsePtr> getSubscriber(HttpRequestPtr req, std::string subscriberId);
    Task<HttpResponsePtr> updateSubscriber(HttpRequestPtr req, std::string subscriberId);
    Task<HttpResponsePtr> subscriberUsage(HttpRequestPtr req, std::string subscriberId);
    Task<HttpResponsePtr> subscriberSessions(HttpRequestPtr req, std::string subscriberId);
    Task<HttpResponsePtr> subscriberDevices(HttpRequestPtr req, std::string subscriberId);
    Task<HttpResponsePtr> suspendSubscriber(HttpRequestPtr req, std::string subscriberId);
    Task<HttpResponsePtr> resumeSubscriber(HttpRequestPtr req, std::string subscriberId);
    Task<HttpResponsePtr> churnSubscriber(HttpRequestPtr req, std::string subscriberId);
    Task<HttpResponsePtr> blockDevice(HttpRequestPtr req);
    Task<HttpResponsePtr> unblockDevice(HttpRequestPtr req);

    Task<HttpResponsePtr> getSubscription(HttpRequestPtr req, std::string subscriptionId);
    Task<HttpResponsePtr> extendSubscription(HttpRequestPtr req, std::string subscriptionId);
    Task<HttpResponsePtr> terminateSubscription(HttpRequestPtr req, std::string subscriptionId);
    Task<HttpResponsePtr> listTransactions(HttpRequestPtr req);
    Task<HttpResponsePtr> verifyTransaction(HttpRequestPtr req, std::string transactionId);
    Task<HttpResponsePtr> refundTransaction(HttpRequestPtr req, std::string transactionId);
    Task<HttpResponsePtr> revenue(HttpRequestPtr req);
    Task<HttpResponsePtr> runInvoices(HttpRequestPtr req);
    Task<HttpResponsePtr> suspendOverdue(HttpRequestPtr req);
    Task<HttpResponsePtr> closeTill(HttpRequestPtr req);
    Task<HttpResponsePtr> reconcile(HttpRequestPtr req);

    Task<HttpResponsePtr> listDevices(HttpRequestPtr req);
    Task<HttpResponsePtr> registerDevice(HttpRequestPtr req);
    Task<HttpResponsePtr> getDevice(HttpRequestPtr req, std::string deviceId);
    Task<HttpResponsePtr> decommissionDevice(HttpRequestPtr req, std::string deviceId);
    Task<HttpResponsePtr> pushConfig(HttpRequestPtr req, std::string deviceId);
    Task<HttpResponsePtr> rebootDevice(HttpRequestPtr req, std::string deviceId);
    Task<HttpResponsePtr> deviceDiagnostics(HttpRequestPtr req, std::string deviceId);
    Task<HttpResponsePtr> pollDevices(HttpRequestPtr req);
};

#endif //DATASUPPORTAPI_MAGMAADMINCONTROLLER_H
