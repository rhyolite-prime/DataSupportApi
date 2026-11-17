//
// MagmaNet — alerts, traffic series, SLA and platform health.
//

#include "MagmaMonitoringService.h"

#include "MagmaSettings.h"
#include "MagmaSupport.h"
#include "utils/DbUtils.h"

using namespace drogon;
using namespace data_support::utils;

namespace data_support::services {

    Task<dto::BaseApiResponse> MagmaMonitoringService::listAlerts(
        const std::string &businessId, bool openOnly, const std::string &severity,
        int pageNo, int pageSize) {
        try {
            auto db = app().getDbClient();
            if (pageNo < 1) pageNo = 1;
            if (pageSize < 1 || pageSize > 200) pageSize = 50;
            auto rows = co_await db->execSqlCoro(
                "SELECT a.id, a.zone_id, z.zone_code, a.device_id, d.name AS device_name, "
                "a.alert_type, a.severity, a.message, a.acknowledged_at, a.resolved_at, a.created_at "
                "FROM magma_alerts a LEFT JOIN magma_zones z ON z.id=a.zone_id "
                "LEFT JOIN magma_devices d ON d.id=a.device_id "
                "WHERE a.business_id=$1 AND ($2=false OR a.resolved_at IS NULL) "
                "AND ($3='' OR a.severity=$3) "
                "ORDER BY a.created_at DESC LIMIT $4 OFFSET $5",
                businessId, openOnly, severity, pageSize, (pageNo - 1) * pageSize);
            co_return okResponse("Alerts", rowsToJson(rows));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] listAlerts error: " << e.what();
            co_return errorResponse("Unable to list alerts");
        }
    }

    Task<dto::BaseApiResponse> MagmaMonitoringService::getAlertSummary(
        const std::string &businessId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT COUNT(*) FILTER (WHERE resolved_at IS NULL) AS open_total, "
                "COUNT(*) FILTER (WHERE resolved_at IS NULL AND severity='CRITICAL') AS open_critical, "
                "COUNT(*) FILTER (WHERE resolved_at IS NULL AND severity='WARNING') AS open_warning, "
                "COUNT(*) FILTER (WHERE created_at >= now() - interval '24 hours') AS last_24h "
                "FROM magma_alerts WHERE business_id=$1", businessId);
            co_return okResponse("Alert summary",
                                 rows.empty() ? Json::Value(Json::objectValue) : jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getAlertSummary error: " << e.what();
            co_return errorResponse("Unable to summarise alerts");
        }
    }

    Task<dto::BaseApiResponse> MagmaMonitoringService::acknowledgeAlert(
        const std::string &businessId, const std::string &alertId, const std::string &staffId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "UPDATE magma_alerts SET acknowledged_by=NULLIF($3,'')::uuid, acknowledged_at=now() "
                "WHERE id=$1::bigint AND business_id=$2 RETURNING id, acknowledged_at",
                alertId, businessId, staffId);
            if (rows.empty()) {
                co_return errorResponse("Alert not found");
            }
            co_return okResponse("Alert acknowledged", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] acknowledgeAlert error: " << e.what();
            co_return errorResponse("Unable to acknowledge alert");
        }
    }

    Task<dto::BaseApiResponse> MagmaMonitoringService::resolveAlert(
        const std::string &businessId, const std::string &alertId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "UPDATE magma_alerts SET resolved_at=now() WHERE id=$1::bigint AND business_id=$2 "
                "AND resolved_at IS NULL RETURNING id, resolved_at", alertId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Alert not found or already resolved");
            }
            co_return okResponse("Alert resolved", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] resolveAlert error: " << e.what();
            co_return errorResponse("Unable to resolve alert");
        }
    }

    Task<dto::BaseApiResponse> MagmaMonitoringService::createAlert(
        const std::string &businessId, const std::string &zoneId, const std::string &deviceId,
        const std::string &alertType, const std::string &severity, const std::string &message) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "INSERT INTO magma_alerts (business_id, zone_id, device_id, alert_type, severity, message) "
                "VALUES ($1,NULLIF($2,'')::uuid,NULLIF($3,'')::uuid,$4,$5,$6) RETURNING id, created_at",
                businessId, zoneId, deviceId, alertType, severity, message);
            co_return okResponse("Alert created", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] createAlert error: " << e.what();
            co_return errorResponse("Unable to create alert");
        }
    }

    Task<dto::BaseApiResponse> MagmaMonitoringService::getBandwidthSeries(
        const std::string &businessId, const std::string &zoneId, int hours) {
        try {
            auto db = app().getDbClient();
            if (hours < 1 || hours > 720) hours = 24;
            const std::string schema = MagmaSettings::instance().radiusSchema;
            auto rows = co_await db->execSqlCoro(
                "SELECT date_trunc('hour', r.acctstarttime) AS bucket, "
                "COALESCE(SUM(r.acctinputoctets),0) AS up_bytes, "
                "COALESCE(SUM(r.acctoutputoctets),0) AS down_bytes, "
                "COUNT(DISTINCT r.username) AS users "
                "FROM " + schema + ".radacct r JOIN magma_subscriptions s "
                "ON s.radius_username=r.username "
                "WHERE s.business_id=$1 AND r.acctstarttime >= now() - ($2 * interval '1 hour') "
                "AND ($3='' OR s.zone_id::text=$3) "
                "GROUP BY 1 ORDER BY 1", businessId, hours, zoneId);

            Json::Value series(Json::arrayValue);
            for (const auto &row : rows) {
                Json::Value point;
                point["bucket"] = row["bucket"].as<std::string>();
                point["upBytes"] = row["up_bytes"].as<std::string>();
                point["downBytes"] = row["down_bytes"].as<std::string>();
                point["users"] = row["users"].as<std::string>();
                series.append(point);
            }

            Json::Value result;
            result["hours"] = hours;
            result["zoneId"] = zoneId;
            result["series"] = series;
            co_return okResponse("Bandwidth series", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getBandwidthSeries error: " << e.what();
            co_return errorResponse("Unable to build bandwidth series");
        }
    }

    Task<dto::BaseApiResponse> MagmaMonitoringService::getConcurrencySeries(
        const std::string &businessId, int hours) {
        try {
            auto db = app().getDbClient();
            if (hours < 1 || hours > 720) hours = 24;
            auto rows = co_await db->execSqlCoro(
                "SELECT gs AS bucket, COUNT(s.id) AS concurrent "
                "FROM generate_series(date_trunc('hour', now() - ($2 * interval '1 hour')), "
                "                     date_trunc('hour', now()), interval '1 hour') gs "
                "LEFT JOIN magma_sessions s ON s.business_id=$1 "
                "AND s.started_at <= gs + interval '1 hour' "
                "AND (s.ended_at IS NULL OR s.ended_at >= gs) "
                "GROUP BY gs ORDER BY gs", businessId, hours);

            Json::Value series(Json::arrayValue);
            for (const auto &row : rows) {
                Json::Value point;
                point["bucket"] = row["bucket"].as<std::string>();
                point["concurrent"] = row["concurrent"].as<std::string>();
                series.append(point);
            }
            co_return okResponse("Concurrency series", series);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getConcurrencySeries error: " << e.what();
            co_return errorResponse("Unable to build concurrency series");
        }
    }

    Task<dto::BaseApiResponse> MagmaMonitoringService::getSlaReport(
        const std::string &businessId, int days) {
        try {
            auto db = app().getDbClient();
            if (days < 1 || days > 365) days = 30;
            auto alerts = co_await db->execSqlCoro(
                "SELECT COUNT(*) FILTER (WHERE alert_type='DEVICE_OFFLINE') AS outages, "
                "COUNT(*) FILTER (WHERE alert_type='DEVICE_OFFLINE' AND resolved_at IS NOT NULL) AS resolved, "
                "COALESCE(AVG(EXTRACT(EPOCH FROM (resolved_at - created_at))/60) "
                "         FILTER (WHERE alert_type='DEVICE_OFFLINE' AND resolved_at IS NOT NULL),0) AS mttr_minutes, "
                "COALESCE(SUM(EXTRACT(EPOCH FROM (COALESCE(resolved_at, now()) - created_at))/60) "
                "         FILTER (WHERE alert_type='DEVICE_OFFLINE'),0) AS downtime_minutes "
                "FROM magma_alerts WHERE business_id=$1 AND created_at >= now() - ($2 * interval '1 day')",
                businessId, days);

            auto devices = co_await db->execSqlCoro(
                "SELECT COUNT(*) AS total, COUNT(*) FILTER (WHERE status='ONLINE') AS online "
                "FROM magma_devices WHERE business_id=$1 AND enrolment_status='ENROLLED'", businessId);

            Json::Value result;
            result["windowDays"] = days;
            result["alerts"] = alerts.empty() ? Json::Value(Json::objectValue) : jsonRow(alerts[0]);
            result["devices"] = devices.empty() ? Json::Value(Json::objectValue) : jsonRow(devices[0]);
            co_return okResponse("SLA report", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getSlaReport error: " << e.what();
            co_return errorResponse("Unable to build SLA report");
        }
    }

    Task<dto::BaseApiResponse> MagmaMonitoringService::getOutboxStatus(
        const std::string &businessId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT status, COUNT(*) AS count, MIN(created_at) AS oldest "
                "FROM magma_outbox GROUP BY status ORDER BY status");
            auto failed = co_await db->execSqlCoro(
                "SELECT id, event_type, attempts, last_error, created_at FROM magma_outbox "
                "WHERE status IN ('FAILED','DEAD') ORDER BY created_at DESC LIMIT 50");
            (void)businessId;
            Json::Value result;
            result["byStatus"] = rowsToJson(rows);
            result["failures"] = rowsToJson(failed);
            co_return okResponse("Outbox status", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getOutboxStatus error: " << e.what();
            co_return errorResponse("Unable to read outbox");
        }
    }

    Task<dto::BaseApiResponse> MagmaMonitoringService::getSystemHealth(
        const std::string &businessId) {
        try {
            auto db = app().getDbClient();
            auto devices = co_await db->execSqlCoro(
                "SELECT COUNT(*) AS total, COUNT(*) FILTER (WHERE status='ONLINE') AS online, "
                "COUNT(*) FILTER (WHERE status='OFFLINE') AS offline "
                "FROM magma_devices WHERE business_id=$1", businessId);
            auto alerts = co_await db->execSqlCoro(
                "SELECT COUNT(*) FILTER (WHERE resolved_at IS NULL) AS open_alerts "
                "FROM magma_alerts WHERE business_id=$1", businessId);
            auto outbox = co_await db->execSqlCoro(
                "SELECT COUNT(*) FILTER (WHERE status='PENDING') AS pending, "
                "COUNT(*) FILTER (WHERE status IN ('FAILED','DEAD')) AS failed "
                "FROM magma_outbox");
            auto latency = co_await db->execSqlCoro(
                "SELECT EXTRACT(EPOCH FROM (now() - statement_timestamp())) * 1000 AS db_skew_ms");

            bool redisHealthy = false;
            auto redis = app().getRedisClient();
            if (redis) {
                auto pong = co_await redis->execCommandCoro("PING");
                redisHealthy = pong.asString() == "PONG";
            }

            Json::Value result;
            result["devices"] = devices.empty() ? Json::Value(Json::objectValue) : jsonRow(devices[0]);
            result["alerts"] = alerts.empty() ? Json::Value(Json::objectValue) : jsonRow(alerts[0]);
            result["outbox"] = outbox.empty() ? Json::Value(Json::objectValue) : jsonRow(outbox[0]);
            result["databaseMs"] = latency.empty() ? "0" : latency[0]["db_skew_ms"].as<std::string>();
            result["redis"] = redisHealthy ? "UP" : "DOWN";
            co_return okResponse("System health", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getSystemHealth error: " << e.what();
            co_return errorResponse("Unable to read platform health");
        }
    }

} // namespace data_support::services
