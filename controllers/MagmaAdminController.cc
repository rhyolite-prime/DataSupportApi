//
// MagmaNet — tenant administration API.
//

#include "MagmaAdminController.h"

#include "MagmaControllerSupport.h"
#include "dto/magma_net/DataPlanDto.h"
#include "dto/magma_net/OpsDto.h"
#include "dto/magma_net/SubscriberDto.h"
#include "dto/magma_net/ZoneDto.h"
#include "plugins/DataSupportServicePlugin.h"

using namespace drogon;
using namespace data_support::magma;

namespace {
    data_support::services::MagmaNetService &magmaService() {
        return app().getPlugin<data_support::plugins::DataSupportServicePlugin>()->getMagmaNetService();
    }
}

// ---------------------------------------------------------------- overview ---
Task<HttpResponsePtr> MagmaAdminController::overview(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().getAdminOverview(auth.businessId));
}

Task<HttpResponsePtr> MagmaAdminController::capacity(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().zones().getCapacityReport(auth.businessId));
}

Task<HttpResponsePtr> MagmaAdminController::networkOverview(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().zones().getNetworkOverview(auth.businessId));
}

// ------------------------------------------------------------------- zones ---
Task<HttpResponsePtr> MagmaAdminController::listZones(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().zones().listZones(
        auth.businessId, queryInt(req, "pageNo", 1), queryInt(req, "pageSize", 50),
        queryValue(req, "status")));
}

Task<HttpResponsePtr> MagmaAdminController::createZone(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    const auto json = req->getJsonObject();
    if (!json) co_return jsonGuard();
    auto result = co_await magmaService().zones().createZone(
        auth.businessId, data_support::dto::magma_net::ZoneDto(*json));
    co_return toResponse(result, result.success ? k201Created : k400BadRequest);
}

Task<HttpResponsePtr> MagmaAdminController::getZone(HttpRequestPtr req, std::string zoneId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    auto result = co_await magmaService().zones().getZone(auth.businessId, zoneId);
    co_return toResponse(result, result.success ? k200OK : k404NotFound);
}

Task<HttpResponsePtr> MagmaAdminController::updateZone(HttpRequestPtr req, std::string zoneId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    const auto json = req->getJsonObject();
    if (!json) co_return jsonGuard();
    auto result = co_await magmaService().zones().updateZone(
        auth.businessId, zoneId, data_support::dto::magma_net::ZoneDto(*json));
    co_return toResponse(result, result.success ? k200OK : k400BadRequest);
}

Task<HttpResponsePtr> MagmaAdminController::zoneHealth(HttpRequestPtr req, std::string zoneId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().zones().getZoneHealth(auth.businessId, zoneId));
}

Task<HttpResponsePtr> MagmaAdminController::listZoneCosts(HttpRequestPtr req, std::string zoneId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().zones().listZoneCosts(
        auth.businessId, zoneId, queryValue(req, "from"), queryValue(req, "to")));
}

Task<HttpResponsePtr> MagmaAdminController::recordZoneCost(HttpRequestPtr req, std::string zoneId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    const auto json = req->getJsonObject();
    if (!json) co_return jsonGuard();
    auto dto = data_support::dto::magma_net::ZoneCostDto::fromJson(*json);
    dto.zoneId = zoneId;
    auto result = co_await magmaService().zones().recordZoneCost(auth.businessId, dto);
    co_return toResponse(result, result.success ? k201Created : k400BadRequest);
}

Task<HttpResponsePtr> MagmaAdminController::zonePnl(HttpRequestPtr req, std::string zoneId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().zones().getZonePnl(
        auth.businessId, zoneId, queryValue(req, "from"), queryValue(req, "to")));
}

// ------------------------------------------------------------------- plans ---
Task<HttpResponsePtr> MagmaAdminController::listPlans(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().plans().listDataPlans(
        auth.businessId, queryValue(req, "includeInactive") == "true"));
}

Task<HttpResponsePtr> MagmaAdminController::createPlan(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    const auto json = req->getJsonObject();
    if (!json) co_return jsonGuard();
    auto result = co_await magmaService().plans().createDataPlan(
        auth.businessId, data_support::dto::magma_net::DataPlanDto(*json));
    co_return toResponse(result, result.success ? k201Created : k400BadRequest);
}

Task<HttpResponsePtr> MagmaAdminController::updatePlan(HttpRequestPtr req, std::string planId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    const auto json = req->getJsonObject();
    if (!json) co_return jsonGuard();
    auto result = co_await magmaService().plans().updateDataPlan(
        auth.businessId, planId, data_support::dto::magma_net::DataPlanDto(*json));
    co_return toResponse(result, result.success ? k200OK : k400BadRequest);
}

Task<HttpResponsePtr> MagmaAdminController::deactivatePlan(HttpRequestPtr req, std::string planId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    auto result = co_await magmaService().plans().deactivateDataPlan(auth.businessId, planId);
    co_return toResponse(result, result.success ? k200OK : k404NotFound);
}

Task<HttpResponsePtr> MagmaAdminController::planAvailability(HttpRequestPtr req, std::string planId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().plans().getPlanAvailability(auth.businessId, planId));
}

Task<HttpResponsePtr> MagmaAdminController::simulatePlan(HttpRequestPtr req, std::string planId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    const auto json = req->getJsonObject();
    if (!json) co_return jsonGuard();
    co_return toResponse(co_await magmaService().plans().simulatePlanChange(
        auth.businessId, planId, data_support::dto::magma_net::DataPlanDto(*json)));
}

// ------------------------------------------------------------- subscribers ---
Task<HttpResponsePtr> MagmaAdminController::listSubscribers(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().subscribers().listSubscribers(
        auth.businessId, queryInt(req, "pageNo", 1), queryInt(req, "pageSize", 50),
        queryValue(req, "q"), queryValue(req, "status")));
}

Task<HttpResponsePtr> MagmaAdminController::createSubscriber(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    const auto json = req->getJsonObject();
    if (!json) co_return jsonGuard();
    auto result = co_await magmaService().subscribers().createSubscriber(
        auth.businessId, data_support::dto::magma_net::SubscriberDto(*json));
    co_return toResponse(result, result.success ? k201Created : k400BadRequest);
}

Task<HttpResponsePtr> MagmaAdminController::getSubscriber(HttpRequestPtr req, std::string subscriberId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    auto result = co_await magmaService().subscribers().getSubscriber(auth.businessId, subscriberId);
    co_return toResponse(result, result.success ? k200OK : k404NotFound);
}

Task<HttpResponsePtr> MagmaAdminController::updateSubscriber(HttpRequestPtr req, std::string subscriberId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    const auto json = req->getJsonObject();
    if (!json) co_return jsonGuard();
    auto result = co_await magmaService().subscribers().updateSubscriber(
        auth.businessId, subscriberId, data_support::dto::magma_net::SubscriberDto(*json));
    co_return toResponse(result, result.success ? k200OK : k400BadRequest);
}

Task<HttpResponsePtr> MagmaAdminController::subscriberUsage(HttpRequestPtr req, std::string subscriberId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().subscribers().getSubscriberUsage(auth.businessId, subscriberId));
}

Task<HttpResponsePtr> MagmaAdminController::subscriberSessions(HttpRequestPtr req, std::string subscriberId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().subscribers().getSubscriberSessions(
        auth.businessId, subscriberId, queryInt(req, "pageNo", 1), queryInt(req, "pageSize", 50)));
}

Task<HttpResponsePtr> MagmaAdminController::subscriberDevices(HttpRequestPtr req, std::string subscriberId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().subscribers().listClientDevices(auth.businessId, subscriberId));
}

Task<HttpResponsePtr> MagmaAdminController::suspendSubscriber(HttpRequestPtr req, std::string subscriberId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().subscribers().suspendSubscriber(
        auth.businessId, subscriberId, queryValue(req, "reason", "ADMIN")));
}

Task<HttpResponsePtr> MagmaAdminController::resumeSubscriber(HttpRequestPtr req, std::string subscriberId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().subscribers().resumeSubscriber(auth.businessId, subscriberId));
}

Task<HttpResponsePtr> MagmaAdminController::churnSubscriber(HttpRequestPtr req, std::string subscriberId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().subscribers().churnSubscriber(auth.businessId, subscriberId));
}

Task<HttpResponsePtr> MagmaAdminController::blockDevice(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    const auto json = bodyJson(req);
    co_return toResponse(co_await magmaService().subscribers().blockClientDevice(
        auth.businessId, queryValue(req, "mac", json.get("mac", "").asString()),
        json.get("reason", "ADMIN").asString()));
}

Task<HttpResponsePtr> MagmaAdminController::unblockDevice(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    const auto json = bodyJson(req);
    co_return toResponse(co_await magmaService().subscribers().unblockClientDevice(
        auth.businessId, queryValue(req, "mac", json.get("mac", "").asString())));
}

// -------------------------------------------------- subscriptions and money ---
Task<HttpResponsePtr> MagmaAdminController::getSubscription(HttpRequestPtr req, std::string subscriptionId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    auto result = co_await magmaService().billing().getSubscription(auth.businessId, subscriptionId);
    co_return toResponse(result, result.success ? k200OK : k404NotFound);
}

Task<HttpResponsePtr> MagmaAdminController::extendSubscription(HttpRequestPtr req, std::string subscriptionId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().billing().extendSubscription(
        auth.businessId, subscriptionId, bodyJson(req).get("planId", "").asString()));
}

Task<HttpResponsePtr> MagmaAdminController::terminateSubscription(HttpRequestPtr req, std::string subscriptionId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().billing().terminateSubscription(
        auth.businessId, subscriptionId, bodyJson(req).get("reason", "ADMIN").asString()));
}

Task<HttpResponsePtr> MagmaAdminController::listTransactions(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().billing().listTransactions(
        auth.businessId, queryInt(req, "pageNo", 1), queryInt(req, "pageSize", 50),
        queryValue(req, "status"), queryValue(req, "from"), queryValue(req, "to")));
}

Task<HttpResponsePtr> MagmaAdminController::verifyTransaction(HttpRequestPtr req, std::string transactionId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().billing().verifyPayment(auth.businessId, transactionId));
}

Task<HttpResponsePtr> MagmaAdminController::refundTransaction(HttpRequestPtr req, std::string transactionId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().billing().refundPayment(
        auth.businessId, transactionId, bodyJson(req).get("reason", "").asString()));
}

Task<HttpResponsePtr> MagmaAdminController::revenue(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().reports().getRevenueSummary(
        auth.businessId, queryValue(req, "from"), queryValue(req, "to")));
}

Task<HttpResponsePtr> MagmaAdminController::runInvoices(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().billing().issueInvoices(auth.businessId));
}

Task<HttpResponsePtr> MagmaAdminController::suspendOverdue(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().billing().suspendOverdue(auth.businessId));
}

Task<HttpResponsePtr> MagmaAdminController::closeTill(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().billing().closeDailyTill(
        auth.businessId, bodyJson(req).get("date", "").asString()));
}

Task<HttpResponsePtr> MagmaAdminController::reconcile(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().billing().reconcilePendingPayments(auth.businessId));
}

// ----------------------------------------------------------------- devices ---
Task<HttpResponsePtr> MagmaAdminController::listDevices(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().devices().listDevices(
        auth.businessId, queryValue(req, "zoneId")));
}

Task<HttpResponsePtr> MagmaAdminController::registerDevice(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    const auto json = req->getJsonObject();
    if (!json) co_return jsonGuard();
    auto result = co_await magmaService().devices().registerDevice(auth.businessId, *json);
    co_return toResponse(result, result.success ? k201Created : k400BadRequest);
}

Task<HttpResponsePtr> MagmaAdminController::getDevice(HttpRequestPtr req, std::string deviceId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    auto result = co_await magmaService().devices().getDevice(auth.businessId, deviceId);
    co_return toResponse(result, result.success ? k200OK : k404NotFound);
}

Task<HttpResponsePtr> MagmaAdminController::decommissionDevice(HttpRequestPtr req, std::string deviceId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().devices().decommissionDevice(auth.businessId, deviceId));
}

Task<HttpResponsePtr> MagmaAdminController::pushConfig(HttpRequestPtr req, std::string deviceId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().devices().pushDeviceConfig(auth.businessId, deviceId));
}

Task<HttpResponsePtr> MagmaAdminController::rebootDevice(HttpRequestPtr req, std::string deviceId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().devices().rebootDevice(auth.businessId, deviceId));
}

Task<HttpResponsePtr> MagmaAdminController::deviceDiagnostics(HttpRequestPtr req, std::string deviceId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().devices().runDeviceDiagnostics(auth.businessId, deviceId));
}

Task<HttpResponsePtr> MagmaAdminController::pollDevices(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().devices().pollHealth(auth.businessId));
}
