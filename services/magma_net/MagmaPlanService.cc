//
// MagmaNet — plan catalogue and availability.
//

#include "MagmaPlanService.h"

#include "MagmaSupport.h"
#include "utils/DbUtils.h"

using namespace drogon;
using namespace data_support::utils;

namespace data_support::services {

    namespace {
        bool validPlan(const dto::magma_net::DataPlanDto &dto, std::string &error) {
            if (dto.planCode.empty() || dto.name.empty()) {
                error = "planCode and name are required";
                return false;
            }
            if (dto.downKbps <= 0 || dto.upKbps <= 0) {
                error = "downKbps and upKbps must be greater than zero";
                return false;
            }
            if (dto.price <= 0) {
                error = "price must be greater than zero";
                return false;
            }
            if (dto.planType == "TIME" && dto.durationMinutes <= 0) {
                error = "TIME plans require durationMinutes";
                return false;
            }
            if (dto.planType == "VOLUME" && dto.dataCapMb <= 0) {
                error = "VOLUME plans require dataCapMb";
                return false;
            }
            if (dto.planType != "TIME" && dto.planType != "VOLUME" && dto.planType != "UNLIMITED") {
                error = "planType must be TIME, VOLUME or UNLIMITED";
                return false;
            }
            return true;
        }

        drogon::Task<> replaceZoneScope(const std::string &planId,
                                        const std::vector<std::string> &zoneIds) {
            auto db = app().getDbClient();
            co_await db->execSqlCoro("DELETE FROM magma_plan_zones WHERE plan_id=$1", planId);
            for (const auto &zoneId : zoneIds) {
                if (zoneId.empty()) continue;
                co_await db->execSqlCoro(
                    "INSERT INTO magma_plan_zones (plan_id, zone_id) VALUES ($1,$2) "
                    "ON CONFLICT DO NOTHING", planId, zoneId);
            }
            co_return;
        }
    }

    Task<dto::BaseApiResponse> MagmaPlanService::createDataPlan(
        const std::string &businessId, const dto::magma_net::DataPlanDto &dto) {
        std::string error;
        if (!validPlan(dto, error)) {
            co_return errorResponse(error);
        }

        try {
            auto db = app().getDbClient();
            const std::string id = newId();
            auto rows = co_await db->execSqlCoro(
                "INSERT INTO magma_data_plans (id, business_id, plan_code, name, plan_type, "
                "duration_minutes, data_cap_mb, fair_use_cap_mb, throttle_down_kbps, throttle_up_kbps, "
                "down_kbps, up_kbps, burst_down_kbps, burst_up_kbps, device_limit, price, "
                "wholesale_price, agent_commission, tax_inclusive, validity_days, is_public, sort_order) "
                "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,$16,$17,$18,$19,$20,$21,$22) "
                "RETURNING *",
                id, businessId, dto.planCode, dto.name, dto.planType,
                dto.durationMinutes, dto.dataCapMb, dto.fairUseCapMb,
                dto.throttleDownKbps, dto.throttleUpKbps, dto.downKbps, dto.upKbps,
                dto.burstDownKbps, dto.burstUpKbps, dto.deviceLimit, dto.price,
                dto.wholesalePrice, dto.agentCommission, dto.taxInclusive,
                dto.validityDays, dto.isPublic, dto.sortOrder);

            co_await replaceZoneScope(id, dto.zoneIds);
            co_await magma::auditLog(businessId, "STAFF", "", "PLAN_CREATED", "magma_data_plans",
                                     id, jsonRow(rows[0]));
            co_return okResponse("Plan created", jsonRow(rows[0]));
        } catch (const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << "[MagmaNet] createDataPlan db error: " << e.base().what();
            co_return errorResponse("Unable to create plan (planCode must be unique)");
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] createDataPlan error: " << e.what();
            co_return errorResponse("Unable to create plan");
        }
    }

    Task<dto::BaseApiResponse> MagmaPlanService::updateDataPlan(
        const std::string &businessId, const std::string &planId,
        const dto::magma_net::DataPlanDto &dto) {
        std::string error;
        if (!validPlan(dto, error)) {
            co_return errorResponse(error);
        }

        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "UPDATE magma_data_plans SET name=$3, plan_type=$4, duration_minutes=$5, "
                "data_cap_mb=$6, fair_use_cap_mb=$7, throttle_down_kbps=$8, throttle_up_kbps=$9, "
                "down_kbps=$10, up_kbps=$11, burst_down_kbps=$12, burst_up_kbps=$13, "
                "device_limit=$14, price=$15, wholesale_price=$16, agent_commission=$17, "
                "tax_inclusive=$18, validity_days=$19, is_public=$20, sort_order=$21, "
                "is_active=$22, updated_at=now() "
                "WHERE id=$1 AND business_id=$2 RETURNING *",
                planId, businessId, dto.name, dto.planType, dto.durationMinutes,
                dto.dataCapMb, dto.fairUseCapMb, dto.throttleDownKbps, dto.throttleUpKbps,
                dto.downKbps, dto.upKbps, dto.burstDownKbps, dto.burstUpKbps,
                dto.deviceLimit, dto.price, dto.wholesalePrice, dto.agentCommission,
                dto.taxInclusive, dto.validityDays, dto.isPublic, dto.sortOrder, true);

            if (rows.empty()) {
                co_return errorResponse("Plan not found");
            }
            co_await replaceZoneScope(planId, dto.zoneIds);
            co_await magma::auditLog(businessId, "STAFF", "", "PLAN_UPDATED",
                                     "magma_data_plans", planId, jsonRow(rows[0]));
            co_return okResponse("Plan updated", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] updateDataPlan error: " << e.what();
            co_return errorResponse("Unable to update plan");
        }
    }

    Task<dto::BaseApiResponse> MagmaPlanService::deactivateDataPlan(
        const std::string &businessId, const std::string &planId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "UPDATE magma_data_plans SET is_active=false, updated_at=now() "
                "WHERE id=$1 AND business_id=$2 RETURNING id, plan_code, is_active",
                planId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Plan not found");
            }
            // Existing subscriptions keep running: the snapshot carries their terms.
            co_return okResponse("Plan deactivated", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] deactivateDataPlan error: " << e.what();
            co_return errorResponse("Unable to deactivate plan");
        }
    }

    Task<dto::BaseApiResponse> MagmaPlanService::listDataPlans(const std::string &businessId,
                                                               bool includeInactive) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT p.*, "
                "(SELECT COUNT(*) FROM magma_subscriptions s WHERE s.plan_id=p.id AND s.status='ACTIVE') AS active_subscriptions, "
                "ARRAY(SELECT z.zone_code FROM magma_plan_zones pz JOIN magma_zones z ON z.id=pz.zone_id "
                "      WHERE pz.plan_id=p.id ORDER BY z.zone_code) AS zone_codes "
                "FROM magma_data_plans p WHERE p.business_id=$1 AND ($2 OR p.is_active) "
                "ORDER BY p.sort_order, p.price",
                businessId, includeInactive);

            Json::Value plans = rowsToJson(rows);
            // ARRAY(...) arrives as a Postgres array literal; expose it as a string array.
            for (auto &plan : plans) {
                const std::string raw = plan.get("zone_codes", "").asString();
                Json::Value zones(Json::arrayValue);
                if (!raw.empty() && raw.front() == '{') {
                    std::string token;
                    for (size_t i = 1; i < raw.size(); ++i) {
                        const char c = raw[i];
                        if (c == ',' || c == '}') {
                            if (!token.empty()) {
                                if (token.front() == '"') token.erase(0, 1);
                                if (!token.empty() && token.back() == '"') token.pop_back();
                                zones.append(token);
                                token.clear();
                            }
                        } else {
                            token.push_back(c);
                        }
                    }
                }
                plan["zone_codes"] = zones;
            }
            co_return okResponse("Plans", plans);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] listDataPlans error: " << e.what();
            co_return errorResponse("Unable to list plans");
        }
    }

    Task<dto::BaseApiResponse> MagmaPlanService::getPublicPlansForZone(
        const std::string &zoneCode, const std::string &businessId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT p.id, p.plan_code, p.name, p.plan_type, p.duration_minutes, p.data_cap_mb, "
                "p.down_kbps, p.up_kbps, p.device_limit, p.price, p.validity_days "
                "FROM magma_data_plans p "
                "JOIN magma_zones z ON z.id = (SELECT id FROM magma_zones "
                "   WHERE zone_code=$1 AND ($2='' OR business_id::text=$2) LIMIT 1) "
                "WHERE p.business_id=z.business_id AND p.is_active AND p.is_public "
                "AND (NOT EXISTS (SELECT 1 FROM magma_plan_zones pz WHERE pz.plan_id=p.id) "
                "     OR EXISTS (SELECT 1 FROM magma_plan_zones pz WHERE pz.plan_id=p.id AND pz.zone_id=z.id)) "
                "ORDER BY p.sort_order, p.price",
                zoneCode, businessId);

            if (rows.empty()) {
                // Distinguish "no plans" from "unknown zone".
                auto zone = co_await db->execSqlCoro(
                    "SELECT id FROM magma_zones WHERE zone_code=$1 AND ($2='' OR business_id::text=$2) LIMIT 1",
                    zoneCode, businessId);
                if (zone.empty()) {
                    co_return errorResponse("Unknown zone");
                }
            }

            Json::Value plans = rowsToJson(rows);
            for (auto &plan : plans) {
                const std::string type = plan.get("plan_type", "").asString();
                if (type == "TIME") {
                    plan["allowance"] = plan.get("duration_minutes", "").asString() + " minutes";
                } else if (type == "VOLUME") {
                    plan["allowance"] = plan.get("data_cap_mb", "").asString() + " MB";
                } else {
                    plan["allowance"] = "Unlimited (fair use)";
                }
            }
            co_return okResponse("Plans for zone", plans);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getPublicPlansForZone error: " << e.what();
            co_return errorResponse("Unable to list plans");
        }
    }

    Task<dto::BaseApiResponse> MagmaPlanService::getPlanAvailability(
        const std::string &businessId, const std::string &planId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT z.id, z.zone_code, z.name, z.status, "
                "EXISTS (SELECT 1 FROM magma_plan_zones pz WHERE pz.plan_id=$2 AND pz.zone_id=z.id) AS explicitly_enabled, "
                "NOT EXISTS (SELECT 1 FROM magma_plan_zones pz WHERE pz.plan_id=$2) AS available_everywhere "
                "FROM magma_zones z WHERE z.business_id=$1 ORDER BY z.zone_code",
                businessId, planId);
            co_return okResponse("Plan availability", rowsToJson(rows));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getPlanAvailability error: " << e.what();
            co_return errorResponse("Unable to load availability");
        }
    }

    Task<dto::BaseApiResponse> MagmaPlanService::simulatePlanChange(
        const std::string &businessId, const std::string &planId,
        const dto::magma_net::DataPlanDto &dto) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT p.price AS current_price, p.plan_type AS current_type, "
                "(SELECT COUNT(*) FROM magma_subscriptions s WHERE s.plan_id=p.id AND s.status='ACTIVE') AS active_subscriptions "
                "FROM magma_data_plans p WHERE p.id=$1 AND p.business_id=$2", planId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Plan not found");
            }

            const double currentPrice = std::stod(rows[0]["current_price"].as<std::string>());
            const int64_t active = rows[0]["active_subscriptions"].as<int64_t>();
            const double delta = dto.price - currentPrice;

            Json::Value result;
            result["current_price"] = currentPrice;
            result["proposed_price"] = dto.price;
            result["delta_per_subscription"] = delta;
            result["active_subscriptions"] = active;
            result["monthly_revenue_impact"] = delta * static_cast<double>(active);
            result["existing_subscriptions_keep_snapshot"] = true;
            co_return okResponse("Plan change simulated", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] simulatePlanChange error: " << e.what();
            co_return errorResponse("Unable to simulate plan change");
        }
    }

} // namespace data_support::services
