//
// MagmaNet — support tickets, work orders, staff and the outbox worker.
//

#include "MagmaOpsService.h"

#include <bcrypt.h>

#include <algorithm>

#include "MagmaRadiusService.h"
#include "MagmaSettings.h"
#include "MagmaSmsService.h"
#include "MagmaSupport.h"
#include "utils/DbUtils.h"
#include "utils/HmacUtils.h"
#include "utils/PhoneNumberFormatter.h"

using namespace drogon;
using namespace data_support::utils;

namespace data_support::services {

    MagmaOpsService::MagmaOpsService(MagmaRadiusService &radius, MagmaSmsService &sms)
        : radius_(radius), sms_(sms) {}

    Task<dto::BaseApiResponse> MagmaOpsService::listTickets(const std::string &businessId,
                                                            const std::string &status,
                                                            int pageNo, int pageSize) {
        try {
            auto db = app().getDbClient();
            if (pageNo < 1) pageNo = 1;
            if (pageSize < 1 || pageSize > 200) pageSize = 50;
            auto rows = co_await db->execSqlCoro(
                "SELECT t.id, t.zone_id, z.zone_code, t.subscriber_id, s.msisdn, s.full_name, "
                "t.category, t.priority, t.status, t.subject, t.assigned_to, t.sla_due_at, "
                "t.resolved_at, t.created_at "
                "FROM magma_tickets t LEFT JOIN magma_zones z ON z.id=t.zone_id "
                "LEFT JOIN magma_subscribers s ON s.id=t.subscriber_id "
                "WHERE t.business_id=$1 AND ($2='' OR t.status=$2) "
                "ORDER BY CASE t.priority WHEN 'URGENT' THEN 0 WHEN 'HIGH' THEN 1 ELSE 2 END, "
                "t.created_at DESC LIMIT $3 OFFSET $4",
                businessId, status, pageSize, (pageNo - 1) * pageSize);
            co_return okResponse("Tickets", rowsToJson(rows));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] listTickets error: " << e.what();
            co_return errorResponse("Unable to list tickets");
        }
    }

    Task<dto::BaseApiResponse> MagmaOpsService::getTicket(const std::string &businessId,
                                                          const std::string &ticketId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT t.*, z.zone_code, s.msisdn, s.full_name FROM magma_tickets t "
                "LEFT JOIN magma_zones z ON z.id=t.zone_id "
                "LEFT JOIN magma_subscribers s ON s.id=t.subscriber_id "
                "WHERE t.id=$1 AND t.business_id=$2", ticketId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Ticket not found");
            }
            auto messages = co_await db->execSqlCoro(
                "SELECT id, author_type, author_id, body, created_at FROM magma_ticket_messages "
                "WHERE ticket_id=$1 ORDER BY created_at", ticketId);
            Json::Value result = jsonRow(rows[0]);
            result["messages"] = rowsToJson(messages);
            co_return okResponse("Ticket", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getTicket error: " << e.what();
            co_return errorResponse("Unable to load ticket");
        }
    }

    Task<dto::BaseApiResponse> MagmaOpsService::createTicket(const std::string &businessId,
                                                             const Json::Value &body) {
        const std::string subject = jsonString(body, "subject");
        const std::string category = jsonString(body, "category", "OTHER");
        if (subject.empty()) {
            co_return errorResponse("subject is required");
        }
        try {
            auto db = app().getDbClient();
            const std::string id = newId();
            const std::string priority = jsonString(body, "priority", "NORMAL");
            const int slaHours = priority == "URGENT" ? 2 : (priority == "HIGH" ? 8 : 24);

            auto rows = co_await db->execSqlCoro(
                "INSERT INTO magma_tickets (id, business_id, zone_id, subscriber_id, category, priority, "
                "status, subject, description, sla_due_at) "
                "VALUES ($1,$2,NULLIF($3,'')::uuid,NULLIF($4,'')::uuid,$5,$6,'OPEN',$7,NULLIF($8,''), "
                "now() + ($9 * interval '1 hour')) RETURNING *",
                id, businessId, jsonString(body, "zoneId"), jsonString(body, "subscriberId"),
                category, priority, subject, jsonString(body, "description"), slaHours);

            const std::string description = jsonString(body, "description");
            if (!description.empty()) {
                co_await db->execSqlCoro(
                    "INSERT INTO magma_ticket_messages (id, ticket_id, author_type, body) "
                    "VALUES ($1,$2,$3,$4)",
                    newId(), id, jsonString(body, "authorType", "CUSTOMER"), description);
            }
            co_await magma::auditLog(businessId, "STAFF", "", "TICKET_CREATED", "magma_tickets", id,
                                     jsonRow(rows[0]));
            co_return okResponse("Ticket created", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] createTicket error: " << e.what();
            co_return errorResponse("Unable to create ticket");
        }
    }

    Task<dto::BaseApiResponse> MagmaOpsService::addTicketMessage(const std::string &businessId,
                                                                 const std::string &ticketId,
                                                                 const Json::Value &body) {
        const std::string message = jsonString(body, "body");
        if (message.empty()) {
            co_return errorResponse("body is required");
        }
        try {
            auto db = app().getDbClient();
            auto ticket = co_await db->execSqlCoro(
                "SELECT id, status FROM magma_tickets WHERE id=$1 AND business_id=$2",
                ticketId, businessId);
            if (ticket.empty()) {
                co_return errorResponse("Ticket not found");
            }
            co_await db->execSqlCoro(
                "INSERT INTO magma_ticket_messages (id, ticket_id, author_type, author_id, body) "
                "VALUES ($1,$2,$3,NULLIF($4,'')::uuid,$5)",
                newId(), ticketId, jsonString(body, "authorType", "STAFF"),
                jsonString(body, "authorId"), message);
            co_await db->execSqlCoro(
                "UPDATE magma_tickets SET status=CASE WHEN status='OPEN' THEN 'IN_PROGRESS' ELSE status END, "
                "updated_at=now() WHERE id=$1", ticketId);
            co_return okResponse("Message added", Json::Value(Json::objectValue));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] addTicketMessage error: " << e.what();
            co_return errorResponse("Unable to add message");
        }
    }

    Task<dto::BaseApiResponse> MagmaOpsService::updateTicket(const std::string &businessId,
                                                             const std::string &ticketId,
                                                             const Json::Value &body) {
        try {
            auto db = app().getDbClient();
            const std::string status = jsonString(body, "status");
            auto rows = co_await db->execSqlCoro(
                "UPDATE magma_tickets SET "
                "status=COALESCE(NULLIF($3,''), status), "
                "priority=COALESCE(NULLIF($4,''), priority), "
                "assigned_to=COALESCE(NULLIF($5,'')::uuid, assigned_to), "
                "resolved_at=CASE WHEN $3 IN ('RESOLVED','CLOSED') THEN COALESCE(resolved_at, now()) "
                "                 WHEN $3 = 'OPEN' THEN NULL ELSE resolved_at END, "
                "updated_at=now() "
                "WHERE id=$1 AND business_id=$2 RETURNING *",
                ticketId, businessId, status, jsonString(body, "priority"),
                jsonString(body, "assignedTo"));
            if (rows.empty()) {
                co_return errorResponse("Ticket not found");
            }
            co_return okResponse("Ticket updated", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] updateTicket error: " << e.what();
            co_return errorResponse("Unable to update ticket");
        }
    }

    Task<dto::BaseApiResponse> MagmaOpsService::listWorkOrders(const std::string &businessId,
                                                               const std::string &status,
                                                               int pageNo, int pageSize) {
        try {
            auto db = app().getDbClient();
            if (pageNo < 1) pageNo = 1;
            if (pageSize < 1 || pageSize > 200) pageSize = 50;
            auto rows = co_await db->execSqlCoro(
                "SELECT w.*, z.zone_code, s.msisdn FROM magma_work_orders w "
                "LEFT JOIN magma_zones z ON z.id=w.zone_id "
                "LEFT JOIN magma_subscribers s ON s.id=w.subscriber_id "
                "WHERE w.business_id=$1 AND ($2='' OR w.status=$2) "
                "ORDER BY w.scheduled_at NULLS LAST, w.created_at DESC LIMIT $3 OFFSET $4",
                businessId, status, pageSize, (pageNo - 1) * pageSize);
            co_return okResponse("Work orders", rowsToJson(rows));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] listWorkOrders error: " << e.what();
            co_return errorResponse("Unable to list work orders");
        }
    }

    Task<dto::BaseApiResponse> MagmaOpsService::createWorkOrder(const std::string &businessId,
                                                                const Json::Value &body) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "INSERT INTO magma_work_orders (id, business_id, zone_id, subscriber_id, order_type, "
                "status, assigned_to, scheduled_at, checklist, cost) "
                "VALUES ($1,$2,NULLIF($3,'')::uuid,NULLIF($4,'')::uuid,$5,'SCHEDULED',"
                "NULLIF($6,'')::uuid, NULLIF($7,'')::timestamptz, $8, $9) RETURNING *",
                newId(), businessId, jsonString(body, "zoneId"), jsonString(body, "subscriberId"),
                jsonString(body, "orderType", "INSTALL"), jsonString(body, "assignedTo"),
                jsonString(body, "scheduledAt"),
                jsonString(body, "checklist", "[]"), jsonDouble(body, "cost", 0));
            co_return okResponse("Work order created", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] createWorkOrder error: " << e.what();
            co_return errorResponse("Unable to create work order");
        }
    }

    Task<dto::BaseApiResponse> MagmaOpsService::updateWorkOrder(const std::string &businessId,
                                                                const std::string &workOrderId,
                                                                const Json::Value &body) {
        try {
            auto db = app().getDbClient();
            const std::string status = jsonString(body, "status");
            auto rows = co_await db->execSqlCoro(
                "UPDATE magma_work_orders SET "
                "status=COALESCE(NULLIF($3,''), status), "
                "assigned_to=COALESCE(NULLIF($4,'')::uuid, assigned_to), "
                "materials=COALESCE(NULLIF($5,'')::jsonb, materials), "
                "photos=COALESCE(NULLIF($6,'')::jsonb, photos), "
                "cost=CASE WHEN $7 >= 0 THEN $7 ELSE cost END, "
                "completed_at=CASE WHEN $3='DONE' THEN now() ELSE completed_at END "
                "WHERE id=$1 AND business_id=$2 RETURNING *",
                workOrderId, businessId, status, jsonString(body, "assignedTo"),
                jsonString(body, "materials"), jsonString(body, "photos"),
                jsonDouble(body, "cost", -1));
            if (rows.empty()) {
                co_return errorResponse("Work order not found");
            }
            co_return okResponse("Work order updated", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] updateWorkOrder error: " << e.what();
            co_return errorResponse("Unable to update work order");
        }
    }

    Task<dto::BaseApiResponse> MagmaOpsService::listStaff(const std::string &businessId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT id, full_name, msisdn, email, role, zone_ids, status, last_login_at, created_at "
                "FROM magma_staff WHERE business_id=$1 ORDER BY full_name", businessId);
            co_return okResponse("Staff", rowsToJson(rows));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] listStaff error: " << e.what();
            co_return errorResponse("Unable to list staff");
        }
    }

    Task<dto::BaseApiResponse> MagmaOpsService::createStaff(const std::string &businessId,
                                                            const Json::Value &body) {
        const std::string name = jsonString(body, "fullName");
        const std::string pin = jsonString(body, "pin");
        if (name.empty() || pin.size() < 4) {
            co_return errorResponse("fullName and a PIN of at least 4 characters are required");
        }
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "INSERT INTO magma_staff (id, business_id, full_name, msisdn, email, pin_hash, role, "
                "zone_ids, status) VALUES ($1,$2,$3,NULLIF($4,''),NULLIF($5,''),$6,$7,$8::uuid[],'ACTIVE') "
                "RETURNING id, full_name, msisdn, email, role, status, created_at",
                newId(), businessId, name, jsonString(body, "msisdn"), jsonString(body, "email"),
                bcrypt::generateHash(pin, 8), jsonString(body, "role", "SUPPORT"),
                jsonString(body, "zoneIds", "{}"));
            co_return okResponse("Staff created", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] createStaff error: " << e.what();
            co_return errorResponse("Unable to create staff member");
        }
    }

    Task<dto::BaseApiResponse> MagmaOpsService::staffLogin(const std::string &businessId,
                                                           const Json::Value &body) {
        const std::string identifier = jsonString(body, "msisdn", jsonString(body, "email"));
        const std::string pin = jsonString(body, "pin");
        if (identifier.empty() || pin.empty()) {
            co_return errorResponse("msisdn/email and pin are required");
        }
        const bool allowed = co_await magma::rateLimit("stafflogin:" + identifier, 8, 600);
        if (!allowed) {
            co_return errorResponse("Too many login attempts. Try again later.");
        }
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT id, full_name, pin_hash, role, status FROM magma_staff "
                "WHERE business_id=$1 AND (msisdn=$2 OR lower(COALESCE(email,''))=lower($2)) LIMIT 1",
                businessId, identifier);
            if (rows.empty() || rows[0]["status"].as<std::string>() != "ACTIVE" ||
                !bcrypt::validatePassword(pin, rows[0]["pin_hash"].as<std::string>())) {
                co_return errorResponse("Invalid credentials");
            }
            co_await db->execSqlCoro(
                "UPDATE magma_staff SET last_login_at=now() WHERE id=$1", rows[0]["id"].as<std::string>());

            Json::Value result;
            result["token"] = HmacUtils::signToken(MagmaSettings::instance().hmacKey, businessId,
                                                   rows[0]["id"].as<std::string>(),
                                                   rows[0]["role"].as<std::string>(), 28800);
            result["staffId"] = rows[0]["id"];
            result["name"] = rows[0]["full_name"];
            result["role"] = rows[0]["role"];
            co_return okResponse("Staff logged in", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] staffLogin error: " << e.what();
            co_return errorResponse("Unable to log in");
        }
    }

    Task<bool> MagmaOpsService::processOutboxEvent(const std::string &eventType,
                                                    const std::string &aggregateId,
                                                    const Json::Value &payload) {
        try {
            if (eventType == "RADIUS_PROVISION" || eventType == "RADIUS_DEPROVISION") {
                std::string businessId = jsonString(payload, "businessId");
                if (businessId.empty()) {
                    auto db = app().getDbClient();
                    auto owner = co_await db->execSqlCoro(
                        "SELECT business_id FROM magma_subscriptions WHERE id=$1", aggregateId);
                    if (owner.empty()) {
                        co_return true;
                    }
                    businessId = owner[0]["business_id"].as<std::string>();
                }
                if (eventType == "RADIUS_PROVISION") {
                    auto result = co_await radius_.projectSubscription(businessId, aggregateId);
                    co_return result.success;
                }
                auto result = co_await radius_.removeSubscription(businessId, aggregateId);
                co_return result.success;
            }
            if (eventType == "SUBSCRIPTION_ENDED" || eventType == "SMS_SEND") {
                auto db = app().getDbClient();
                auto rows = co_await db->execSqlCoro(
                    "SELECT s.business_id, s.expires_at, subs.msisdn, p.name AS plan_name "
                    "FROM magma_subscriptions s JOIN magma_subscribers subs ON subs.id=s.subscriber_id "
                    "JOIN magma_data_plans p ON p.id=s.plan_id WHERE s.id=$1", aggregateId);
                if (rows.empty() || rows[0]["msisdn"].isNull()) {
                    co_return true;
                }
                if (eventType == "SMS_SEND") {
                    co_return co_await sms_.sendReceipt(
                        rows[0]["business_id"].as<std::string>(),
                        rows[0]["msisdn"].as<std::string>(),
                        rows[0]["plan_name"].as<std::string>(), "active",
                        jsonDouble(payload, "amount", 0.0),
                        rows[0]["expires_at"].isNull() ? "" : rows[0]["expires_at"].as<std::string>());
                }
                co_return co_await sms_.send(
                    rows[0]["business_id"].as<std::string>(),
                    rows[0]["msisdn"].as<std::string>(),
                    "Your MagmaNet plan " + rows[0]["plan_name"].as<std::string>() +
                        " has ended. Dial *714# or visit the portal to top up.");
            }
            if (eventType == "SMS_RAW") {
                const std::string to = jsonString(payload, "msisdn");
                const std::string message = jsonString(payload, "message");
                if (to.empty() || message.empty()) {
                    co_return true;
                }
                auto db = app().getDbClient();
                auto rows = co_await db->execSqlCoro(
                    "SELECT business_id FROM magma_transactions WHERE id=$1 "
                    "UNION ALL SELECT business_id FROM magma_invoices WHERE id=$1 LIMIT 1",
                    aggregateId);
                if (rows.empty()) {
                    co_return true;
                }
                co_return co_await sms_.send(rows[0]["business_id"].as<std::string>(), to, message);
            }
            // Unknown events are acknowledged so they do not block the queue.
            co_return true;
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] outbox event " << eventType << " failed: " << e.what();
            co_return false;
        }
    }

    Task<dto::BaseApiResponse> MagmaOpsService::drainOutbox(int limit) {
        try {
            auto db = app().getDbClient();
            if (limit < 1 || limit > 500) limit = 50;
            auto rows = co_await db->execSqlCoro(
                "SELECT id, event_type, aggregate_id, payload, attempts FROM magma_outbox "
                "WHERE status='PENDING' AND next_attempt_at <= now() ORDER BY id LIMIT $1", limit);

            int processed = 0;
            int failed = 0;
            for (const auto &row : rows) {
                const int64_t id = std::stoll(row["id"].as<std::string>());
                const std::string eventType = row["event_type"].as<std::string>();
                const std::string aggregateId = row["aggregate_id"].isNull()
                                                    ? "" : row["aggregate_id"].as<std::string>();
                const auto payload = parseJson(row["payload"].as<std::string>());
                const int attempts = row["attempts"].as<int>();

                const bool ok = co_await processOutboxEvent(eventType, aggregateId, payload);
                if (ok) {
                    co_await db->execSqlCoro(
                        "UPDATE magma_outbox SET status='DONE', processed_at=now(), last_error=NULL "
                        "WHERE id=$1", id);
                    ++processed;
                } else {
                    const int nextAttempt = attempts + 1;
                    const int delayMinutes = std::min(60, 1 << std::min(nextAttempt, 6));
                    co_await db->execSqlCoro(
                        "UPDATE magma_outbox SET attempts=attempts+1, last_error=$2, "
                        "next_attempt_at=now() + ($3 * interval '1 minute'), "
                        "status=CASE WHEN attempts + 1 >= 8 THEN 'DEAD' ELSE 'PENDING' END "
                        "WHERE id=$1",
                        id, "Handler returned failure", delayMinutes);
                    ++failed;
                }
            }

            Json::Value result;
            result["picked"] = static_cast<int64_t>(rows.size());
            result["processed"] = processed;
            result["failed"] = failed;
            co_return okResponse("Outbox drained", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] drainOutbox error: " << e.what();
            co_return errorResponse("Outbox drain failed");
        }
    }

    Task<dto::BaseApiResponse> MagmaOpsService::retryOutbox(const std::string &outboxId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "UPDATE magma_outbox SET status='PENDING', attempts=0, next_attempt_at=now(), "
                "last_error=NULL WHERE id=$1::bigint AND status IN ('FAILED','DEAD') RETURNING id",
                outboxId);
            if (rows.empty()) {
                co_return errorResponse("Outbox event not found");
            }
            co_return okResponse("Outbox event queued for retry", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] retryOutbox error: " << e.what();
            co_return errorResponse("Unable to retry event");
        }
    }

    Task<dto::BaseApiResponse> MagmaOpsService::listAuditLog(const std::string &businessId,
                                                             int pageNo, int pageSize) {
        try {
            auto db = app().getDbClient();
            if (pageNo < 1) pageNo = 1;
            if (pageSize < 1 || pageSize > 200) pageSize = 50;
            auto rows = co_await db->execSqlCoro(
                "SELECT id, actor_type, actor_id, action, entity, entity_id, after, created_at "
                "FROM magma_audit_log WHERE business_id=$1 ORDER BY created_at DESC "
                "LIMIT $2 OFFSET $3", businessId, pageSize, (pageNo - 1) * pageSize);
            co_return okResponse("Audit log", rowsToJson(rows));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] listAuditLog error: " << e.what();
            co_return errorResponse("Unable to read audit log");
        }
    }

} // namespace data_support::services
