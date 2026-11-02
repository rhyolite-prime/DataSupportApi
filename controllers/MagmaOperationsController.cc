//
// MagmaNet — vouchers, agents, monitoring, reports and back-office operations.
//

#include "MagmaOperationsController.h"

#include "MagmaControllerSupport.h"
#include "dto/magma_net/AgentDto.h"
#include "dto/magma_net/VoucherDto.h"
#include "plugins/DataSupportServicePlugin.h"

using namespace drogon;
using namespace data_support::magma;

namespace {
    data_support::services::MagmaNetService &magmaService() {
        return app().getPlugin<data_support::plugins::DataSupportServicePlugin>()->getMagmaNetService();
    }
}

// ------------------------------------------------------------------ vouchers ---
Task<HttpResponsePtr> MagmaOperationsController::listBatches(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().vouchers().listVoucherBatches(
        auth.businessId, queryInt(req, "pageNo", 1), queryInt(req, "pageSize", 50)));
}

Task<HttpResponsePtr> MagmaOperationsController::createBatch(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    const auto json = req->getJsonObject();
    if (!json) co_return jsonGuard();
    auto result = co_await magmaService().vouchers().createVoucherBatch(
        auth.businessId, data_support::dto::magma_net::VoucherBatchDto::fromJson(*json));
    co_return toResponse(result, result.success ? k201Created : k400BadRequest);
}

Task<HttpResponsePtr> MagmaOperationsController::getBatch(HttpRequestPtr req, std::string batchId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    auto result = co_await magmaService().vouchers().getVoucherBatch(auth.businessId, batchId);
    co_return toResponse(result, result.success ? k200OK : k404NotFound);
}

Task<HttpResponsePtr> MagmaOperationsController::downloadBatch(HttpRequestPtr req, std::string batchId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    auto result = co_await magmaService().vouchers().downloadVoucherBatch(auth.businessId, batchId);
    co_return toResponse(result, result.success ? k200OK : k404NotFound);
}

Task<HttpResponsePtr> MagmaOperationsController::assignBatch(HttpRequestPtr req, std::string batchId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().vouchers().assignBatchToAgent(
        auth.businessId, batchId, bodyJson(req).get("agentId", "").asString()));
}

Task<HttpResponsePtr> MagmaOperationsController::voidBatch(HttpRequestPtr req, std::string batchId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    std::vector<std::string> vouchers;
    const auto json = bodyJson(req);
    if (json.isMember("voucherIds") && json["voucherIds"].isArray()) {
        for (const auto &id : json["voucherIds"]) {
            vouchers.push_back(id.asString());
        }
    }
    co_return toResponse(co_await magmaService().vouchers().voidVouchers(
        auth.businessId, batchId, vouchers));
}

Task<HttpResponsePtr> MagmaOperationsController::voucherInventory(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().vouchers().getVoucherInventory(
        auth.businessId, queryValue(req, "zoneId")));
}

// -------------------------------------------------------------------- agents ---
Task<HttpResponsePtr> MagmaOperationsController::listAgents(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().agents().listAgents(
        auth.businessId, queryInt(req, "pageNo", 1), queryInt(req, "pageSize", 50),
        queryValue(req, "status")));
}

Task<HttpResponsePtr> MagmaOperationsController::createAgent(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    const auto json = req->getJsonObject();
    if (!json) co_return jsonGuard();
    auto result = co_await magmaService().agents().createAgent(
        auth.businessId, data_support::dto::magma_net::AgentDto(*json));
    co_return toResponse(result, result.success ? k201Created : k400BadRequest);
}

Task<HttpResponsePtr> MagmaOperationsController::updateAgent(HttpRequestPtr req, std::string agentId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    const auto json = req->getJsonObject();
    if (!json) co_return jsonGuard();
    auto result = co_await magmaService().agents().updateAgent(
        auth.businessId, agentId, data_support::dto::magma_net::AgentDto(*json));
    co_return toResponse(result, result.success ? k200OK : k404NotFound);
}

Task<HttpResponsePtr> MagmaOperationsController::agentDashboard(HttpRequestPtr req, std::string agentId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().agents().getAgentDashboard(auth.businessId, agentId));
}

Task<HttpResponsePtr> MagmaOperationsController::agentTransactions(HttpRequestPtr req, std::string agentId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().agents().getAgentTransactions(
        auth.businessId, agentId, queryInt(req, "pageNo", 1), queryInt(req, "pageSize", 50)));
}

Task<HttpResponsePtr> MagmaOperationsController::agentStatement(HttpRequestPtr req, std::string agentId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().agents().getAgentStatement(
        auth.businessId, agentId, queryValue(req, "from"), queryValue(req, "to")));
}

Task<HttpResponsePtr> MagmaOperationsController::agentStock(HttpRequestPtr req, std::string agentId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    auto dto = data_support::dto::magma_net::AgentStockPurchaseDto::fromJson(bodyJson(req));
    dto.agentId = agentId;
    co_return toResponse(co_await magmaService().agents().purchaseBatch(auth.businessId, dto));
}

Task<HttpResponsePtr> MagmaOperationsController::agentSettlement(HttpRequestPtr req, std::string agentId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    auto dto = data_support::dto::magma_net::AgentSettlementDto::fromJson(bodyJson(req));
    dto.agentId = agentId;
    co_return toResponse(co_await magmaService().agents().initializeSettlement(auth.businessId, dto));
}

// ---------------------------------------------------------------- monitoring ---
Task<HttpResponsePtr> MagmaOperationsController::listAlerts(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().monitoring().listAlerts(
        auth.businessId, queryValue(req, "openOnly") != "false",
        queryValue(req, "severity"), queryInt(req, "pageNo", 1), queryInt(req, "pageSize", 50)));
}

Task<HttpResponsePtr> MagmaOperationsController::createAlert(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    const auto json = bodyJson(req);
    auto result = co_await magmaService().monitoring().createAlert(
        auth.businessId, json.get("zoneId", "").asString(), json.get("deviceId", "").asString(),
        json.get("alertType", "MANUAL").asString(), json.get("severity", "INFO").asString(),
        json.get("message", "").asString());
    co_return toResponse(result, result.success ? k201Created : k400BadRequest);
}

Task<HttpResponsePtr> MagmaOperationsController::alertSummary(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().monitoring().getAlertSummary(auth.businessId));
}

Task<HttpResponsePtr> MagmaOperationsController::ackAlert(HttpRequestPtr req, std::string alertId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().monitoring().acknowledgeAlert(
        auth.businessId, alertId, bodyJson(req).get("staffId", "").asString()));
}

Task<HttpResponsePtr> MagmaOperationsController::resolveAlert(HttpRequestPtr req, std::string alertId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().monitoring().resolveAlert(auth.businessId, alertId));
}

Task<HttpResponsePtr> MagmaOperationsController::bandwidth(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().monitoring().getBandwidthSeries(
        auth.businessId, queryValue(req, "zoneId"), queryInt(req, "hours", 24)));
}

Task<HttpResponsePtr> MagmaOperationsController::concurrency(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().monitoring().getConcurrencySeries(
        auth.businessId, queryInt(req, "hours", 24)));
}

Task<HttpResponsePtr> MagmaOperationsController::sla(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().monitoring().getSlaReport(
        auth.businessId, queryInt(req, "days", 30)));
}

Task<HttpResponsePtr> MagmaOperationsController::systemHealth(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().monitoring().getSystemHealth(auth.businessId));
}

Task<HttpResponsePtr> MagmaOperationsController::outboxStatus(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().monitoring().getOutboxStatus(auth.businessId));
}

// ------------------------------------------------------------------- reports ---
Task<HttpResponsePtr> MagmaOperationsController::salesReport(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().reports().getSalesReport(
        auth.businessId, queryValue(req, "from"), queryValue(req, "to"),
        queryValue(req, "groupBy", "day")));
}

Task<HttpResponsePtr> MagmaOperationsController::revenueReport(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().reports().getRevenueSummary(
        auth.businessId, queryValue(req, "from"), queryValue(req, "to")));
}

Task<HttpResponsePtr> MagmaOperationsController::subscriberReport(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().reports().getSubscriberReport(
        auth.businessId, queryInt(req, "days", 30)));
}

Task<HttpResponsePtr> MagmaOperationsController::voucherReport(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().reports().getVoucherSalesReport(
        auth.businessId, queryValue(req, "from"), queryValue(req, "to")));
}

Task<HttpResponsePtr> MagmaOperationsController::agentReport(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().reports().getAgentPerformanceReport(
        auth.businessId, queryValue(req, "from"), queryValue(req, "to")));
}

Task<HttpResponsePtr> MagmaOperationsController::taxReport(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().reports().getTaxSummary(
        auth.businessId, queryValue(req, "from"), queryValue(req, "to")));
}

Task<HttpResponsePtr> MagmaOperationsController::exportReport(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().reports().exportCsv(
        auth.businessId, queryValue(req, "type", "transactions"),
        queryValue(req, "from"), queryValue(req, "to")));
}

// --------------------------------------------------------------- support desk ---
Task<HttpResponsePtr> MagmaOperationsController::listTickets(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().ops().listTickets(
        auth.businessId, queryValue(req, "status"), queryInt(req, "pageNo", 1),
        queryInt(req, "pageSize", 50)));
}

Task<HttpResponsePtr> MagmaOperationsController::createTicket(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    auto result = co_await magmaService().ops().createTicket(auth.businessId, bodyJson(req));
    co_return toResponse(result, result.success ? k201Created : k400BadRequest);
}

Task<HttpResponsePtr> MagmaOperationsController::getTicket(HttpRequestPtr req, std::string ticketId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    auto result = co_await magmaService().ops().getTicket(auth.businessId, ticketId);
    co_return toResponse(result, result.success ? k200OK : k404NotFound);
}

Task<HttpResponsePtr> MagmaOperationsController::updateTicket(HttpRequestPtr req, std::string ticketId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    auto result = co_await magmaService().ops().updateTicket(auth.businessId, ticketId, bodyJson(req));
    co_return toResponse(result, result.success ? k200OK : k404NotFound);
}

Task<HttpResponsePtr> MagmaOperationsController::ticketMessage(HttpRequestPtr req, std::string ticketId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    auto result = co_await magmaService().ops().addTicketMessage(auth.businessId, ticketId, bodyJson(req));
    co_return toResponse(result, result.success ? k201Created : k400BadRequest);
}

Task<HttpResponsePtr> MagmaOperationsController::listWorkOrders(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().ops().listWorkOrders(
        auth.businessId, queryValue(req, "status"), queryInt(req, "pageNo", 1),
        queryInt(req, "pageSize", 50)));
}

Task<HttpResponsePtr> MagmaOperationsController::createWorkOrder(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    auto result = co_await magmaService().ops().createWorkOrder(auth.businessId, bodyJson(req));
    co_return toResponse(result, result.success ? k201Created : k400BadRequest);
}

Task<HttpResponsePtr> MagmaOperationsController::updateWorkOrder(HttpRequestPtr req, std::string workOrderId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    auto result = co_await magmaService().ops().updateWorkOrder(auth.businessId, workOrderId, bodyJson(req));
    co_return toResponse(result, result.success ? k200OK : k404NotFound);
}

Task<HttpResponsePtr> MagmaOperationsController::listStaff(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().ops().listStaff(auth.businessId));
}

Task<HttpResponsePtr> MagmaOperationsController::createStaff(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    auto result = co_await magmaService().ops().createStaff(auth.businessId, bodyJson(req));
    co_return toResponse(result, result.success ? k201Created : k400BadRequest);
}

Task<HttpResponsePtr> MagmaOperationsController::staffLogin(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    auto result = co_await magmaService().ops().staffLogin(auth.businessId, bodyJson(req));
    co_return toResponse(result, result.success ? k200OK : k401Unauthorized);
}

Task<HttpResponsePtr> MagmaOperationsController::auditLog(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().ops().listAuditLog(
        auth.businessId, queryInt(req, "pageNo", 1), queryInt(req, "pageSize", 50)));
}

Task<HttpResponsePtr> MagmaOperationsController::drainOutbox(HttpRequestPtr req) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().ops().drainOutbox(queryInt(req, "limit", 100)));
}

Task<HttpResponsePtr> MagmaOperationsController::retryOutbox(HttpRequestPtr req, std::string outboxId) {
    const auto auth = co_await authenticateAdmin(req);
    if (!auth.ok) co_return auth.error;
    co_return toResponse(co_await magmaService().ops().retryOutbox(outboxId));
}
