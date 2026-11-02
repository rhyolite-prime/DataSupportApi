//
// MagmaNet — control-plane facade.
//

#include "MagmaNetService.h"

#include <vector>

#include "MagmaSettings.h"
#include "dto/QuartzJobDto.h"
#include "services/quartz/QuartzApi.h"
#include "utils/DbUtils.h"

using namespace drogon;
using namespace data_support::utils;

namespace data_support::services {

    MagmaNetService::MagmaNetService()
        : radius_(),
          subscribers_(),
          billing_(radius_, subscribers_, payments_, sms_),
          zones_(),
          plans_(),
          vouchers_(subscribers_, billing_),
          agents_(vouchers_, payments_, sms_),
          devices_(radius_),
          monitoring_(),
          reports_(),
          ops_(radius_, sms_) {}

    Task<dto::BaseApiResponse> MagmaNetService::getPortalBootstrap(
        const std::string &zoneCode, const std::string &businessId) {
        try {
            auto db = app().getDbClient();
            auto zones = co_await db->execSqlCoro(
                "SELECT z.id, z.business_id, z.zone_code, z.name, z.description, z.status, "
                "z.district, z.region, b.business_name "
                "FROM magma_zones z JOIN business_accounts b ON b.id=z.business_id "
                "WHERE z.zone_code=$1 AND ($2='' OR z.business_id::text=$2) "
                "ORDER BY (z.status='LIVE') DESC LIMIT 1",
                zoneCode, businessId);
            if (zones.empty()) {
                co_return errorResponse("Zone not found");
            }
            const std::string resolvedBusinessId = zones[0]["business_id"].as<std::string>();

            auto plans = co_await plans_.getPublicPlansForZone(zoneCode, resolvedBusinessId);

            Json::Value result;
            result["zone"] = Json::Value(Json::objectValue);
            result["zone"]["code"] = zones[0]["zone_code"];
            result["zone"]["name"] = zones[0]["name"];
            result["zone"]["district"] = zones[0]["district"];
            result["zone"]["status"] = zones[0]["status"];
            result["business"] = Json::Value(Json::objectValue);
            result["business"]["id"] = resolvedBusinessId;
            result["business"]["name"] = zones[0]["business_name"];
            result["plans"] = plans.success ? plans.result : Json::Value(Json::arrayValue);
            result["paymentMethods"] = Json::Value(Json::arrayValue);
            result["paymentMethods"].append("MOMO_MTN");
            result["paymentMethods"].append("MOMO_TELECEL");
            result["paymentMethods"].append("MOMO_AIRTELTIGO");
            result["paymentMethods"].append("CARD");
            result["voucherEnabled"] = true;
            co_return okResponse("Portal bootstrap", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getPortalBootstrap error: " << e.what();
            co_return errorResponse("Unable to load portal configuration");
        }
    }

    Task<dto::BaseApiResponse> MagmaNetService::getAdminOverview(const std::string &businessId) {
        try {
            auto db = app().getDbClient();
            auto counts = co_await db->execSqlCoro(
                "SELECT "
                "(SELECT COUNT(*) FROM magma_zones WHERE business_id=$1) AS zones, "
                "(SELECT COUNT(*) FROM magma_subscribers WHERE business_id=$1 AND status='ACTIVE') AS active_subscribers, "
                "(SELECT COUNT(*) FROM magma_subscriptions WHERE business_id=$1 AND status='ACTIVE') AS active_subscriptions, "
                "(SELECT COUNT(*) FROM magma_sessions WHERE business_id=$1 AND ended_at IS NULL) AS online_sessions, "
                "(SELECT COUNT(*) FROM magma_devices WHERE business_id=$1 AND status='ONLINE') AS devices_online, "
                "(SELECT COUNT(*) FROM magma_devices WHERE business_id=$1) AS devices_total, "
                "(SELECT COALESCE(SUM(amount),0) FROM magma_transactions WHERE business_id=$1 "
                " AND status='SUCCESS' AND paid_at >= date_trunc('day', now())) AS revenue_today, "
                "(SELECT COALESCE(SUM(amount),0) FROM magma_transactions WHERE business_id=$1 "
                " AND status='SUCCESS' AND paid_at >= date_trunc('month', now())) AS revenue_month, "
                "(SELECT COUNT(*) FROM magma_alerts WHERE business_id=$1 AND resolved_at IS NULL) AS open_alerts, "
                "(SELECT COUNT(*) FROM magma_tickets WHERE business_id=$1 AND status='OPEN') AS open_tickets",
                businessId);

            auto zoneBreakdown = co_await db->execSqlCoro(
                "SELECT z.zone_code, z.name, z.status, "
                "(SELECT COUNT(*) FROM magma_subscriptions s WHERE s.zone_id=z.id AND s.status='ACTIVE') AS active_subscriptions, "
                "(SELECT COUNT(*) FROM magma_sessions s WHERE s.zone_id=z.id AND s.ended_at IS NULL) AS online_sessions, "
                "(SELECT COALESCE(SUM(t.amount),0) FROM magma_transactions t "
                " JOIN magma_subscriptions s ON s.id=t.subscription_id "
                " WHERE s.zone_id=z.id AND t.status='SUCCESS' AND t.paid_at >= date_trunc('day', now())) AS revenue_today "
                "FROM magma_zones z WHERE z.business_id=$1 ORDER BY z.name", businessId);

            Json::Value result;
            result["summary"] = counts.empty() ? Json::Value(Json::objectValue) : jsonRow(counts[0]);
            result["zones"] = rowsToJson(zoneBreakdown);
            co_return okResponse("Overview", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getAdminOverview error: " << e.what();
            co_return errorResponse("Unable to load overview");
        }
    }

    Task<dto::BaseApiResponse> MagmaNetService::runTenMinuteJobs() {
        Json::Value result;
        auto usage = co_await radius_.syncUsage();
        result["usageSync"] = usage.success;
        auto limits = co_await radius_.enforceLimits();
        result["enforceLimits"] = limits.success;
        result["enforceDetail"] = limits.result;
        auto outbox = co_await ops_.drainOutbox(100);
        result["outbox"] = outbox.result;
        co_return okResponse("Ten-minute jobs complete", result);
    }

    Task<dto::BaseApiResponse> MagmaNetService::runHourlyJobs() {
        try {
            auto db = app().getDbClient();
            auto businesses = co_await db->execSqlCoro(
                "SELECT DISTINCT business_id FROM magma_zones WHERE status <> 'RETIRED'");
            Json::Value result(Json::arrayValue);
            for (const auto &row : businesses) {
                const std::string businessId = row["business_id"].as<std::string>();
                Json::Value entry;
                entry["businessId"] = businessId;
                entry["health"] = (co_await devices_.pollHealth(businessId)).result;
                entry["reconcile"] = (co_await billing_.reconcilePendingPayments(businessId)).result;
                result.append(entry);
            }
            Json::Value payload;
            payload["businesses"] = result;
            co_return okResponse("Hourly jobs complete", payload);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] runHourlyJobs error: " << e.what();
            co_return errorResponse("Hourly jobs failed");
        }
    }

    Task<dto::BaseApiResponse> MagmaNetService::runDailyJobs() {
        try {
            auto db = app().getDbClient();
            auto businesses = co_await db->execSqlCoro(
                "SELECT DISTINCT business_id FROM magma_zones WHERE status <> 'RETIRED'");
            Json::Value result(Json::arrayValue);
            for (const auto &row : businesses) {
                const std::string businessId = row["business_id"].as<std::string>();
                Json::Value entry;
                entry["businessId"] = businessId;
                entry["invoices"] = (co_await billing_.issueInvoices(businessId)).result;
                entry["suspended"] = (co_await billing_.suspendOverdue(businessId)).result;
                result.append(entry);
            }
            Json::Value payload;
            payload["businesses"] = result;
            co_return okResponse("Daily jobs complete", payload);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] runDailyJobs error: " << e.what();
            co_return errorResponse("Daily jobs failed");
        }
    }

    Task<dto::BaseApiResponse> MagmaNetService::registerScheduledJobs(QuartzApi &quartz) {
        const std::string base = MagmaSettings::instance().portalBaseUrl +
                                 "/api/v1/magma/internal/jobs/";
        struct Job {
            const char *name;
            const char *schedule;
            const char *path;
        };
        const Job jobs[] = {
            {"magma-ten-minute", "0 */10 * * * ?", "ten-minute"},
            {"magma-hourly", "0 0 * * * ?", "hourly"},
            {"magma-daily", "0 30 2 * * ?", "daily"},
        };

        Json::Value scheduled(Json::arrayValue);
        for (const auto &job : jobs) {
            dto::QuartzJobDto dto;
            dto.name = job.name;
            dto.description = "MagmaNet scheduled job";
            dto.schedule = job.schedule;
            dto.customData.callbackUrl = base + job.path;
            dto.customData.uniqueId = job.name;
            const bool ok = co_await quartz.scheduleJob(dto);
            Json::Value entry;
            entry["name"] = job.name;
            entry["schedule"] = job.schedule;
            entry["scheduled"] = ok;
            scheduled.append(entry);
        }

        Json::Value result;
        result["jobs"] = scheduled;
        co_return okResponse("Schedules registered", result);
    }

} // namespace data_support::services
