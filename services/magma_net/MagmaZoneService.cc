//
// MagmaNet — zones, backhaul records, capacity and zone P&L.
//

#include "MagmaZoneService.h"

#include "MagmaSupport.h"
#include "utils/DbUtils.h"

using namespace drogon;
using namespace data_support::utils;

namespace data_support::services {

    Task<dto::BaseApiResponse> MagmaZoneService::createZone(
        const std::string &businessId, const dto::magma_net::ZoneDto &dto) {
        if (dto.zoneCode.empty() || dto.name.empty() || dto.district.empty()) {
            co_return errorResponse("zoneCode, name and district are required");
        }

        try {
            auto db = app().getDbClient();
            const std::string id = newId();
            auto rows = co_await db->execSqlCoro(
                "INSERT INTO magma_zones (id, business_id, zone_code, name, district, region, "
                "gps_lat, gps_lng, backhaul_provider, backhaul_circuit_ref, upstream_down_mbps, "
                "upstream_up_mbps, upstream_monthly_cost, upstream_is_cgnat, public_ip, tunnel_ip, "
                "status, notes) VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,"
                "NULLIF($15,'')::inet, NULLIF($16,'')::inet, $17, $18) RETURNING *",
                id, businessId, dto.zoneCode, dto.name, dto.district,
                dto.region.empty() ? std::string("Greater Accra") : dto.region,
                dto.gpsLat, dto.gpsLng, dto.backhaulProvider, dto.backhaulCircuitRef,
                dto.upstreamDownMbps, dto.upstreamUpMbps, dto.upstreamMonthlyCost,
                dto.cgnat, dto.publicIp, dto.tunnelIp, dto.status, dto.notes);

            co_await magma::auditLog(businessId, "STAFF", "", "ZONE_CREATED", "magma_zones", id,
                                     jsonRow(rows[0]));
            co_return okResponse("Zone created", jsonRow(rows[0]));
        } catch (const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << "[MagmaNet] createZone db error: " << e.base().what();
            co_return errorResponse("Unable to create zone (code must be unique)");
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] createZone error: " << e.what();
            co_return errorResponse("Unable to create zone");
        }
    }

    Task<dto::BaseApiResponse> MagmaZoneService::updateZone(
        const std::string &businessId, const std::string &zoneId,
        const dto::magma_net::ZoneDto &dto) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "UPDATE magma_zones SET "
                "name = COALESCE(NULLIF($3,''), name), "
                "district = COALESCE(NULLIF($4,''), district), "
                "region = COALESCE(NULLIF($5,''), region), "
                "backhaul_provider = COALESCE(NULLIF($6,''), backhaul_provider), "
                "backhaul_circuit_ref = COALESCE(NULLIF($7,''), backhaul_circuit_ref), "
                "upstream_down_mbps = CASE WHEN $8 > 0 THEN $8 ELSE upstream_down_mbps END, "
                "upstream_up_mbps = CASE WHEN $9 > 0 THEN $9 ELSE upstream_up_mbps END, "
                "upstream_monthly_cost = CASE WHEN $10 > 0 THEN $10 ELSE upstream_monthly_cost END, "
                "upstream_is_cgnat = $11, "
                "tunnel_ip = COALESCE(NULLIF($12,'')::inet, tunnel_ip), "
                "status = COALESCE(NULLIF($13,''), status), "
                "notes = COALESCE(NULLIF($14,''), notes), "
                "updated_at = now() "
                "WHERE id = $1 AND business_id = $2 RETURNING *",
                zoneId, businessId, dto.name, dto.district, dto.region,
                dto.backhaulProvider, dto.backhaulCircuitRef,
                dto.upstreamDownMbps, dto.upstreamUpMbps, dto.upstreamMonthlyCost,
                dto.cgnat, dto.tunnelIp, dto.status, dto.notes);

            if (rows.empty()) {
                co_return errorResponse("Zone not found");
            }
            co_await magma::auditLog(businessId, "STAFF", "", "ZONE_UPDATED", "magma_zones",
                                     zoneId, jsonRow(rows[0]));
            co_return okResponse("Zone updated", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] updateZone error: " << e.what();
            co_return errorResponse("Unable to update zone");
        }
    }

    Task<dto::BaseApiResponse> MagmaZoneService::getZone(const std::string &businessId,
                                                         const std::string &zoneId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT * FROM magma_zones WHERE id = $1 AND business_id = $2", zoneId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Zone not found");
            }
            auto zone = jsonRow(rows[0]);

            auto devices = co_await db->execSqlCoro(
                "SELECT id, name, role, model, serial_number, status, last_seen_at "
                "FROM magma_devices WHERE zone_id = $1 ORDER BY role, name", zoneId);
            zone["devices"] = rowsToJson(devices);

            auto counts = co_await db->execSqlCoro(
                "SELECT "
                "(SELECT COUNT(*) FROM magma_subscriptions WHERE zone_id=$1 AND status='ACTIVE') AS active_subscriptions, "
                "(SELECT COUNT(*) FROM magma_sessions WHERE zone_id=$1 AND ended_at IS NULL) AS active_sessions, "
                "(SELECT COUNT(*) FROM magma_subscribers WHERE home_zone_id=$1) AS subscribers",
                zoneId);
            zone["counts"] = rowsToJson(counts);
            co_return okResponse("Zone", zone);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getZone error: " << e.what();
            co_return errorResponse("Unable to load zone");
        }
    }

    Task<dto::BaseApiResponse> MagmaZoneService::listZones(const std::string &businessId,
                                                           int pageNo, int pageSize,
                                                           const std::string &status) {
        try {
            auto db = app().getDbClient();
            if (pageNo < 1) pageNo = 1;
            if (pageSize < 1 || pageSize > 200) pageSize = 50;
            const int offset = (pageNo - 1) * pageSize;

            auto rows = co_await db->execSqlCoro(
                "SELECT z.*, "
                "(SELECT COUNT(*) FROM magma_devices d WHERE d.zone_id=z.id AND d.status='ONLINE') AS devices_online, "
                "(SELECT COUNT(*) FROM magma_devices d WHERE d.zone_id=z.id) AS devices_total, "
                "(SELECT COUNT(*) FROM magma_subscriptions s WHERE s.zone_id=z.id AND s.status='ACTIVE') AS active_subscriptions "
                "FROM magma_zones z WHERE z.business_id=$1 "
                "AND ($2 = '' OR z.status = $2) "
                "ORDER BY z.created_at DESC LIMIT $3 OFFSET $4",
                businessId, status, pageSize, offset);

            auto total = co_await db->execSqlCoro(
                "SELECT COUNT(*) AS count FROM magma_zones WHERE business_id=$1 "
                "AND ($2 = '' OR status = $2)", businessId, status);

            Json::Value result;
            result["zones"] = rowsToJson(rows);
            result["pageNo"] = pageNo;
            result["pageSize"] = pageSize;
            result["total"] = total.empty() ? 0 : total[0]["count"].as<int64_t>();
            co_return okResponse("Zones", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] listZones error: " << e.what();
            co_return errorResponse("Unable to list zones");
        }
    }

    Task<dto::BaseApiResponse> MagmaZoneService::getZoneHealth(const std::string &businessId,
                                                               const std::string &zoneId) {
        try {
            auto db = app().getDbClient();
            auto zoneRows = co_await db->execSqlCoro(
                "SELECT * FROM magma_zones WHERE id=$1 AND business_id=$2", zoneId, businessId);
            if (zoneRows.empty()) {
                co_return errorResponse("Zone not found");
            }

            Json::Value health = jsonRow(zoneRows[0]);

            auto devices = co_await db->execSqlCoro(
                "SELECT id, name, role, status, last_seen_at, cpu_load, free_memory_mb, temperature_c "
                "FROM magma_devices WHERE zone_id=$1 ORDER BY role", zoneId);
            health["devices"] = rowsToJson(devices);

            auto counters = co_await db->execSqlCoro(
                "SELECT "
                "(SELECT COUNT(*) FROM magma_sessions WHERE zone_id=$1 AND ended_at IS NULL) AS online_sessions, "
                "(SELECT COUNT(*) FROM magma_subscriptions WHERE zone_id=$1 AND status='ACTIVE' AND expires_at > now()) AS active_plans, "
                "(SELECT COALESCE(SUM(amount),0) FROM magma_transactions t JOIN magma_subscriptions s ON s.id=t.subscription_id "
                " WHERE s.zone_id=$1 AND t.status='SUCCESS' AND t.created_at >= date_trunc('day', now())) AS revenue_today, "
                "(SELECT COUNT(*) FROM magma_alerts WHERE zone_id=$1 AND resolved_at IS NULL) AS open_alerts",
                zoneId);
            health["counters"] = rowsToJson(counters);

            auto alerts = co_await db->execSqlCoro(
                "SELECT id, alert_type, severity, message, created_at FROM magma_alerts "
                "WHERE zone_id=$1 AND resolved_at IS NULL ORDER BY created_at DESC LIMIT 20", zoneId);
            health["alerts"] = rowsToJson(alerts);

            co_return okResponse("Zone health", health);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getZoneHealth error: " << e.what();
            co_return errorResponse("Unable to load zone health");
        }
    }

    Task<dto::BaseApiResponse> MagmaZoneService::recordZoneCost(
        const std::string &businessId, const dto::magma_net::ZoneCostDto &dto) {
        if (dto.zoneId.empty() || dto.amount <= 0) {
            co_return errorResponse("zoneId and a positive amount are required");
        }
        try {
            auto db = app().getDbClient();
            const std::string id = newId();
            auto rows = co_await db->execSqlCoro(
                "INSERT INTO magma_zone_costs (id, business_id, zone_id, category, amount, "
                "period_start, period_end, vendor, reference, notes) VALUES ($1,$2,$3,$4,$5,"
                "COALESCE(NULLIF($6,'')::date, date_trunc('month', now())::date),"
                "COALESCE(NULLIF($7,'')::date, (date_trunc('month', now()) + interval '1 month - 1 day')::date),"
                "$8,$9,$10) RETURNING *",
                id, businessId, dto.zoneId, dto.category, dto.amount,
                dto.periodStart, dto.periodEnd, dto.vendor, dto.reference, dto.notes);
            co_return okResponse("Cost recorded", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] recordZoneCost error: " << e.what();
            co_return errorResponse("Unable to record cost");
        }
    }

    Task<dto::BaseApiResponse> MagmaZoneService::listZoneCosts(const std::string &businessId,
                                                               const std::string &zoneId,
                                                               const std::string &from,
                                                               const std::string &to) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT * FROM magma_zone_costs WHERE business_id=$1 AND ($2='' OR zone_id::text=$2) "
                "AND period_start >= COALESCE(NULLIF($3,'')::date, date_trunc('year', now())::date) "
                "AND period_end <= COALESCE(NULLIF($4,'')::date, now()::date) "
                "ORDER BY period_start DESC",
                businessId, zoneId, from, to);
            co_return okResponse("Zone costs", rowsToJson(rows));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] listZoneCosts error: " << e.what();
            co_return errorResponse("Unable to list costs");
        }
    }

    Task<dto::BaseApiResponse> MagmaZoneService::getZonePnl(const std::string &businessId,
                                                            const std::string &zoneId,
                                                            const std::string &from,
                                                            const std::string &to) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "WITH period AS ("
                "  SELECT COALESCE(NULLIF($3,'')::date, date_trunc('month', now())::date) AS from_date,"
                "         COALESCE(NULLIF($4,'')::date, now()::date) AS to_date"
                "), revenue AS ("
                "  SELECT COALESCE(SUM(t.amount),0) AS gross, COUNT(*) AS tx_count"
                "  FROM magma_transactions t JOIN magma_subscriptions s ON s.id=t.subscription_id"
                "  WHERE s.zone_id=$2 AND t.status='SUCCESS' AND t.purpose IN ('BUNDLE','TOPUP')"
                "    AND t.created_at::date BETWEEN (SELECT from_date FROM period) AND (SELECT to_date FROM period)"
                "), costs AS ("
                "  SELECT COALESCE(SUM(amount),0) AS total FROM magma_zone_costs"
                "  WHERE zone_id=$2 AND period_start >= (SELECT from_date FROM period)"
                "    AND period_end <= (SELECT to_date FROM period)"
                "), subs AS ("
                "  SELECT COUNT(*) AS active FROM magma_subscriptions WHERE zone_id=$2 AND status='ACTIVE'"
                ") "
                "SELECT revenue.gross, revenue.tx_count, costs.total AS cost_total, subs.active, "
                "CASE WHEN revenue.gross > 0 THEN ROUND((revenue.gross - costs.total) / revenue.gross * 100, 2) ELSE 0 END AS gross_margin_pct "
                "FROM revenue, costs, subs",
                businessId, zoneId, from, to);

            auto byCategory = co_await db->execSqlCoro(
                "SELECT category, COALESCE(SUM(amount),0) AS total FROM magma_zone_costs "
                "WHERE zone_id=$1 GROUP BY category ORDER BY total DESC", zoneId);

            Json::Value result;
            if (!rows.empty()) {
                result = jsonRow(rows[0]);
            }
            result["costs_by_category"] = rowsToJson(byCategory);
            co_return okResponse("Zone P&L", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getZonePnl error: " << e.what();
            co_return errorResponse("Unable to compute zone P&L");
        }
    }

    Task<dto::BaseApiResponse> MagmaZoneService::getCapacityReport(const std::string &businessId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT z.id, z.zone_code, z.name, z.upstream_down_mbps, "
                "(SELECT COUNT(*) FROM magma_subscriptions s WHERE s.zone_id=z.id AND s.status='ACTIVE') AS active_subs, "
                "(SELECT COUNT(*) FROM magma_sessions s WHERE s.zone_id=z.id AND s.ended_at IS NULL) AS online_sessions, "
                "(SELECT COUNT(*) FROM magma_devices d WHERE d.zone_id=z.id AND d.status='ONLINE') AS devices_online, "
                "CASE WHEN z.upstream_down_mbps > 0 THEN "
                "ROUND((SELECT COUNT(*) FROM magma_sessions s WHERE s.zone_id=z.id AND s.ended_at IS NULL) "
                "/ z.upstream_down_mbps, 2) ELSE 0 END AS sessions_per_mbps "
                "FROM magma_zones z WHERE z.business_id=$1 ORDER BY z.zone_code", businessId);
            co_return okResponse("Capacity report", rowsToJson(rows));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getCapacityReport error: " << e.what();
            co_return errorResponse("Unable to build capacity report");
        }
    }

    Task<dto::BaseApiResponse> MagmaZoneService::getNetworkOverview(const std::string &businessId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT "
                "(SELECT COUNT(*) FROM magma_zones WHERE business_id=$1 AND status='LIVE') AS live_zones, "
                "(SELECT COUNT(*) FROM magma_zones WHERE business_id=$1) AS total_zones, "
                "(SELECT COUNT(*) FROM magma_devices WHERE business_id=$1 AND status='ONLINE') AS devices_online, "
                "(SELECT COUNT(*) FROM magma_devices WHERE business_id=$1) AS devices_total, "
                "(SELECT COUNT(*) FROM magma_sessions WHERE business_id=$1 AND ended_at IS NULL) AS online_sessions, "
                "(SELECT COUNT(*) FROM magma_subscriptions WHERE business_id=$1 AND status='ACTIVE') AS active_subscriptions, "
                "(SELECT COALESCE(SUM(amount),0) FROM magma_transactions WHERE business_id=$1 AND status='SUCCESS' "
                " AND created_at >= date_trunc('day', now())) AS revenue_today, "
                "(SELECT COALESCE(SUM(amount),0) FROM magma_transactions WHERE business_id=$1 AND status='SUCCESS' "
                " AND created_at >= date_trunc('month', now())) AS revenue_month, "
                "(SELECT COUNT(*) FROM magma_alerts WHERE business_id=$1 AND resolved_at IS NULL) AS open_alerts, "
                "(SELECT COUNT(*) FROM magma_tickets WHERE business_id=$1 AND status IN ('OPEN','IN_PROGRESS')) AS open_tickets",
                businessId);
            Json::Value result = rows.empty() ? Json::Value(Json::objectValue) : jsonRow(rows[0]);
            co_return okResponse("Network overview", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getNetworkOverview error: " << e.what();
            co_return errorResponse("Unable to load overview");
        }
    }

} // namespace data_support::services
