//
// MagmaNet — vouchers, agents, monitoring, reports and back-office operations.
//

#ifndef DATASUPPORTAPI_MAGMAOPERATIONSCONTROLLER_H
#define DATASUPPORTAPI_MAGMAOPERATIONSCONTROLLER_H

#include <drogon/HttpController.h>

using namespace drogon;

class MagmaOperationsController : public HttpController<MagmaOperationsController> {
public:
    static constexpr const char *PREFIX = "/api/v1/magma/admin/";

    METHOD_LIST_BEGIN
    // vouchers
    ADD_METHOD_TO(MagmaOperationsController::listBatches, std::string(PREFIX) + "vouchers/batches", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::createBatch, std::string(PREFIX) + "vouchers/batches", Post, Options);
    ADD_METHOD_TO(MagmaOperationsController::getBatch, std::string(PREFIX) + "vouchers/batches/{1}", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::downloadBatch, std::string(PREFIX) + "vouchers/batches/{1}/download", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::assignBatch, std::string(PREFIX) + "vouchers/batches/{1}/assign", Post, Options);
    ADD_METHOD_TO(MagmaOperationsController::voidBatch, std::string(PREFIX) + "vouchers/batches/{1}/void", Post, Options);
    ADD_METHOD_TO(MagmaOperationsController::voucherInventory, std::string(PREFIX) + "vouchers/inventory", Get, Options);

    // agents
    ADD_METHOD_TO(MagmaOperationsController::listAgents, std::string(PREFIX) + "agents", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::createAgent, std::string(PREFIX) + "agents", Post, Options);
    ADD_METHOD_TO(MagmaOperationsController::updateAgent, std::string(PREFIX) + "agents/{1}", Put, Options);
    ADD_METHOD_TO(MagmaOperationsController::agentDashboard, std::string(PREFIX) + "agents/{1}/dashboard", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::agentTransactions, std::string(PREFIX) + "agents/{1}/transactions", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::agentStatement, std::string(PREFIX) + "agents/{1}/statement", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::agentStock, std::string(PREFIX) + "agents/{1}/stock", Post, Options);
    ADD_METHOD_TO(MagmaOperationsController::agentSettlement, std::string(PREFIX) + "agents/{1}/settlement", Post, Options);

    // monitoring
    ADD_METHOD_TO(MagmaOperationsController::listAlerts, std::string(PREFIX) + "alerts", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::createAlert, std::string(PREFIX) + "alerts", Post, Options);
    ADD_METHOD_TO(MagmaOperationsController::alertSummary, std::string(PREFIX) + "alerts/summary", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::ackAlert, std::string(PREFIX) + "alerts/{1}/ack", Post, Options);
    ADD_METHOD_TO(MagmaOperationsController::resolveAlert, std::string(PREFIX) + "alerts/{1}/resolve", Post, Options);
    ADD_METHOD_TO(MagmaOperationsController::bandwidth, std::string(PREFIX) + "monitoring/bandwidth", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::concurrency, std::string(PREFIX) + "monitoring/concurrency", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::sla, std::string(PREFIX) + "monitoring/sla", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::systemHealth, std::string(PREFIX) + "monitoring/health", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::outboxStatus, std::string(PREFIX) + "monitoring/outbox", Get, Options);

    // reports
    ADD_METHOD_TO(MagmaOperationsController::salesReport, std::string(PREFIX) + "reports/sales", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::revenueReport, std::string(PREFIX) + "reports/revenue", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::subscriberReport, std::string(PREFIX) + "reports/subscribers", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::voucherReport, std::string(PREFIX) + "reports/vouchers", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::agentReport, std::string(PREFIX) + "reports/agents", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::taxReport, std::string(PREFIX) + "reports/tax", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::exportReport, std::string(PREFIX) + "reports/export", Get, Options);

    // support desk
    ADD_METHOD_TO(MagmaOperationsController::listTickets, std::string(PREFIX) + "tickets", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::createTicket, std::string(PREFIX) + "tickets", Post, Options);
    ADD_METHOD_TO(MagmaOperationsController::getTicket, std::string(PREFIX) + "tickets/{1}", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::updateTicket, std::string(PREFIX) + "tickets/{1}", Put, Options);
    ADD_METHOD_TO(MagmaOperationsController::ticketMessage, std::string(PREFIX) + "tickets/{1}/messages", Post, Options);
    ADD_METHOD_TO(MagmaOperationsController::listWorkOrders, std::string(PREFIX) + "work-orders", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::createWorkOrder, std::string(PREFIX) + "work-orders", Post, Options);
    ADD_METHOD_TO(MagmaOperationsController::updateWorkOrder, std::string(PREFIX) + "work-orders/{1}", Put, Options);
    ADD_METHOD_TO(MagmaOperationsController::listStaff, std::string(PREFIX) + "staff", Get, Options);
    ADD_METHOD_TO(MagmaOperationsController::createStaff, std::string(PREFIX) + "staff", Post, Options);
    ADD_METHOD_TO(MagmaOperationsController::staffLogin, std::string(PREFIX) + "staff/login", Post, Options);
    ADD_METHOD_TO(MagmaOperationsController::auditLog, std::string(PREFIX) + "audit", Get, Options);

    // outbox
    ADD_METHOD_TO(MagmaOperationsController::drainOutbox, std::string(PREFIX) + "outbox/drain", Post, Options);
    ADD_METHOD_TO(MagmaOperationsController::retryOutbox, std::string(PREFIX) + "outbox/{1}/retry", Post, Options);
    METHOD_LIST_END

    Task<HttpResponsePtr> listBatches(HttpRequestPtr req);
    Task<HttpResponsePtr> createBatch(HttpRequestPtr req);
    Task<HttpResponsePtr> getBatch(HttpRequestPtr req, std::string batchId);
    Task<HttpResponsePtr> downloadBatch(HttpRequestPtr req, std::string batchId);
    Task<HttpResponsePtr> assignBatch(HttpRequestPtr req, std::string batchId);
    Task<HttpResponsePtr> voidBatch(HttpRequestPtr req, std::string batchId);
    Task<HttpResponsePtr> voucherInventory(HttpRequestPtr req);

    Task<HttpResponsePtr> listAgents(HttpRequestPtr req);
    Task<HttpResponsePtr> createAgent(HttpRequestPtr req);
    Task<HttpResponsePtr> updateAgent(HttpRequestPtr req, std::string agentId);
    Task<HttpResponsePtr> agentDashboard(HttpRequestPtr req, std::string agentId);
    Task<HttpResponsePtr> agentTransactions(HttpRequestPtr req, std::string agentId);
    Task<HttpResponsePtr> agentStatement(HttpRequestPtr req, std::string agentId);
    Task<HttpResponsePtr> agentStock(HttpRequestPtr req, std::string agentId);
    Task<HttpResponsePtr> agentSettlement(HttpRequestPtr req, std::string agentId);

    Task<HttpResponsePtr> listAlerts(HttpRequestPtr req);
    Task<HttpResponsePtr> createAlert(HttpRequestPtr req);
    Task<HttpResponsePtr> alertSummary(HttpRequestPtr req);
    Task<HttpResponsePtr> ackAlert(HttpRequestPtr req, std::string alertId);
    Task<HttpResponsePtr> resolveAlert(HttpRequestPtr req, std::string alertId);
    Task<HttpResponsePtr> bandwidth(HttpRequestPtr req);
    Task<HttpResponsePtr> concurrency(HttpRequestPtr req);
    Task<HttpResponsePtr> sla(HttpRequestPtr req);
    Task<HttpResponsePtr> systemHealth(HttpRequestPtr req);
    Task<HttpResponsePtr> outboxStatus(HttpRequestPtr req);

    Task<HttpResponsePtr> salesReport(HttpRequestPtr req);
    Task<HttpResponsePtr> revenueReport(HttpRequestPtr req);
    Task<HttpResponsePtr> subscriberReport(HttpRequestPtr req);
    Task<HttpResponsePtr> voucherReport(HttpRequestPtr req);
    Task<HttpResponsePtr> agentReport(HttpRequestPtr req);
    Task<HttpResponsePtr> taxReport(HttpRequestPtr req);
    Task<HttpResponsePtr> exportReport(HttpRequestPtr req);

    Task<HttpResponsePtr> listTickets(HttpRequestPtr req);
    Task<HttpResponsePtr> createTicket(HttpRequestPtr req);
    Task<HttpResponsePtr> getTicket(HttpRequestPtr req, std::string ticketId);
    Task<HttpResponsePtr> updateTicket(HttpRequestPtr req, std::string ticketId);
    Task<HttpResponsePtr> ticketMessage(HttpRequestPtr req, std::string ticketId);
    Task<HttpResponsePtr> listWorkOrders(HttpRequestPtr req);
    Task<HttpResponsePtr> createWorkOrder(HttpRequestPtr req);
    Task<HttpResponsePtr> updateWorkOrder(HttpRequestPtr req, std::string workOrderId);
    Task<HttpResponsePtr> listStaff(HttpRequestPtr req);
    Task<HttpResponsePtr> createStaff(HttpRequestPtr req);
    Task<HttpResponsePtr> staffLogin(HttpRequestPtr req);
    Task<HttpResponsePtr> auditLog(HttpRequestPtr req);

    Task<HttpResponsePtr> drainOutbox(HttpRequestPtr req);
    Task<HttpResponsePtr> retryOutbox(HttpRequestPtr req, std::string outboxId);
};

#endif //DATASUPPORTAPI_MAGMAOPERATIONSCONTROLLER_H
