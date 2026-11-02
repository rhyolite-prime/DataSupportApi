//
// MagmaNet — revenue, subscriber and tax reporting.
//

#include "MagmaReportService.h"

#include <sstream>
#include <vector>

#include "utils/DbUtils.h"

using namespace drogon;
using namespace data_support::utils;

namespace data_support::services {

    namespace {
        constexpr const char *kDefaultFrom = "date_trunc('month', now())";

        std::string cell(const drogon::orm::Row &row, const std::string &name) {
            const auto &field = row[name.c_str()];
            return field.isNull() ? "" : field.as<std::string>();
        }

        std::string groupExpression(const std::string &groupBy) {
            if (groupBy == "plan") return "p.plan_code";
            if (groupBy == "zone") return "z.zone_code";
            if (groupBy == "method") return "t.payment_method";
            if (groupBy == "agent") return "a.agent_code";
            if (groupBy == "week") return "date_trunc('week', t.paid_at)";
            if (groupBy == "month") return "date_trunc('month', t.paid_at)";
            return "date_trunc('day', t.paid_at)";
        }
    }

    Task<dto::BaseApiResponse> MagmaReportService::getSalesReport(
        const std::string &businessId, const std::string &from, const std::string &to,
        const std::string &groupBy) {
        try {
            auto db = app().getDbClient();
            const std::string group = groupExpression(groupBy);
            auto rows = co_await db->execSqlCoro(
                "SELECT " + group + " AS bucket, COUNT(*) AS transactions, "
                "COALESCE(SUM(t.amount),0) AS gross, COALESCE(SUM(t.tax_amount),0) AS tax, "
                "COALESCE(SUM(COALESCE(t.net_amount, t.amount - t.tax_amount)),0) AS net "
                "FROM magma_transactions t "
                "LEFT JOIN magma_subscriptions s ON s.id=t.subscription_id "
                "LEFT JOIN magma_data_plans p ON p.id=s.plan_id "
                "LEFT JOIN magma_zones z ON z.id=s.zone_id "
                "LEFT JOIN magma_agents a ON a.id=t.agent_id "
                "WHERE t.business_id=$1 AND t.status='SUCCESS' "
                "AND t.paid_at >= COALESCE(NULLIF($2,'')::timestamptz, " + std::string(kDefaultFrom) + ") "
                "AND t.paid_at <= COALESCE(NULLIF($3,'')::timestamptz, now()) "
                "GROUP BY bucket ORDER BY bucket", businessId, from, to);

            auto totals = co_await db->execSqlCoro(
                "SELECT COUNT(*) AS transactions, COALESCE(SUM(amount),0) AS gross, "
                "COALESCE(SUM(tax_amount),0) AS tax, "
                "COALESCE(SUM(COALESCE(net_amount, amount - tax_amount)),0) AS net "
                "FROM magma_transactions WHERE business_id=$1 AND status='SUCCESS' "
                "AND paid_at >= COALESCE(NULLIF($2,'')::timestamptz, " + std::string(kDefaultFrom) + ") "
                "AND paid_at <= COALESCE(NULLIF($3,'')::timestamptz, now())", businessId, from, to);

            Json::Value result;
            result["groupBy"] = groupBy;
            result["rows"] = rowsToJson(rows);
            result["totals"] = totals.empty() ? Json::Value(Json::objectValue) : jsonRow(totals[0]);
            co_return okResponse("Sales report", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getSalesReport error: " << e.what();
            co_return errorResponse("Unable to build sales report");
        }
    }

    Task<dto::BaseApiResponse> MagmaReportService::getRevenueSummary(
        const std::string &businessId, const std::string &from, const std::string &to) {
        try {
            auto db = app().getDbClient();
            auto byPurpose = co_await db->execSqlCoro(
                "SELECT purpose, COUNT(*) AS count, COALESCE(SUM(amount),0) AS gross "
                "FROM magma_transactions WHERE business_id=$1 AND status='SUCCESS' "
                "AND paid_at >= COALESCE(NULLIF($2,'')::timestamptz, " + std::string(kDefaultFrom) + ") "
                "AND paid_at <= COALESCE(NULLIF($3,'')::timestamptz, now()) "
                "GROUP BY purpose ORDER BY gross DESC", businessId, from, to);
            auto byMethod = co_await db->execSqlCoro(
                "SELECT COALESCE(payment_method,'OTHER') AS method, COUNT(*) AS count, "
                "COALESCE(SUM(amount),0) AS gross FROM magma_transactions "
                "WHERE business_id=$1 AND status='SUCCESS' "
                "AND paid_at >= COALESCE(NULLIF($2,'')::timestamptz, " + std::string(kDefaultFrom) + ") "
                "AND paid_at <= COALESCE(NULLIF($3,'')::timestamptz, now()) "
                "GROUP BY method ORDER BY gross DESC", businessId, from, to);
            auto today = co_await db->execSqlCoro(
                "SELECT COALESCE(SUM(amount),0) AS gross, COUNT(*) AS count "
                "FROM magma_transactions WHERE business_id=$1 AND status='SUCCESS' "
                "AND paid_at >= date_trunc('day', now())", businessId);

            Json::Value result;
            result["byPurpose"] = rowsToJson(byPurpose);
            result["byMethod"] = rowsToJson(byMethod);
            result["today"] = today.empty() ? Json::Value(Json::objectValue) : jsonRow(today[0]);
            co_return okResponse("Revenue summary", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getRevenueSummary error: " << e.what();
            co_return errorResponse("Unable to build revenue summary");
        }
    }

    Task<dto::BaseApiResponse> MagmaReportService::getSubscriberReport(
        const std::string &businessId, int days) {
        try {
            auto db = app().getDbClient();
            if (days < 1 || days > 365) days = 30;
            auto growth = co_await db->execSqlCoro(
                "SELECT date_trunc('day', created_at) AS bucket, COUNT(*) AS new_subscribers "
                "FROM magma_subscribers WHERE business_id=$1 "
                "AND created_at >= now() - ($2 * interval '1 day') GROUP BY 1 ORDER BY 1",
                businessId, days);
            auto churn = co_await db->execSqlCoro(
                "SELECT date_trunc('day', updated_at) AS bucket, COUNT(*) AS churned "
                "FROM magma_subscribers WHERE business_id=$1 AND status='CHURNED' "
                "AND updated_at >= now() - ($2 * interval '1 day') GROUP BY 1 ORDER BY 1",
                businessId, days);
            auto totals = co_await db->execSqlCoro(
                "SELECT COUNT(*) AS total, "
                "COUNT(*) FILTER (WHERE status='ACTIVE') AS active, "
                "COUNT(*) FILTER (WHERE created_at >= now() - interval '30 days') AS new_30d, "
                "(SELECT COUNT(DISTINCT s.subscriber_id) FROM magma_subscriptions s "
                " WHERE s.business_id=$1 AND s.created_at >= now() - interval '30 days') AS paying_30d "
                "FROM magma_subscribers WHERE business_id=$1", businessId);

            Json::Value result;
            result["windowDays"] = days;
            result["growth"] = rowsToJson(growth);
            result["churn"] = rowsToJson(churn);
            result["totals"] = totals.empty() ? Json::Value(Json::objectValue) : jsonRow(totals[0]);
            co_return okResponse("Subscriber report", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getSubscriberReport error: " << e.what();
            co_return errorResponse("Unable to build subscriber report");
        }
    }

    Task<dto::BaseApiResponse> MagmaReportService::getVoucherSalesReport(
        const std::string &businessId, const std::string &from, const std::string &to) {
        try {
            auto db = app().getDbClient();
            auto byPlan = co_await db->execSqlCoro(
                "SELECT p.plan_code, p.name AS plan_name, COUNT(*) AS redeemed, "
                "COALESCE(SUM(v.price),0) AS face_value "
                "FROM magma_vouchers v JOIN magma_data_plans p ON p.id=v.plan_id "
                "WHERE v.business_id=$1 AND v.status='REDEEMED' "
                "AND v.redeemed_at >= COALESCE(NULLIF($2,'')::timestamptz, " + std::string(kDefaultFrom) + ") "
                "AND v.redeemed_at <= COALESCE(NULLIF($3,'')::timestamptz, now()) "
                "GROUP BY p.plan_code, p.name ORDER BY face_value DESC", businessId, from, to);
            auto status = co_await db->execSqlCoro(
                "SELECT status, COUNT(*) AS count FROM magma_vouchers "
                "WHERE business_id=$1 GROUP BY status ORDER BY status", businessId);

            Json::Value result;
            result["byPlan"] = rowsToJson(byPlan);
            result["inventory"] = rowsToJson(status);
            co_return okResponse("Voucher sales report", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getVoucherSalesReport error: " << e.what();
            co_return errorResponse("Unable to build voucher report");
        }
    }

    Task<dto::BaseApiResponse> MagmaReportService::getAgentPerformanceReport(
        const std::string &businessId, const std::string &from, const std::string &to) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT a.agent_code, a.name, "
                "(SELECT COUNT(*) FROM magma_vouchers v WHERE v.agent_id=a.id "
                " AND v.redeemed_at >= COALESCE(NULLIF($2,'')::timestamptz, " + std::string(kDefaultFrom) + ") "
                " AND v.redeemed_at <= COALESCE(NULLIF($3,'')::timestamptz, now())) AS vouchers_redeemed, "
                "COALESCE((SELECT SUM(l.amount) FROM magma_agent_ledger l WHERE l.agent_id=a.id "
                " AND l.entry_type='EARNING' AND l.created_at >= COALESCE(NULLIF($2,'')::timestamptz, " + std::string(kDefaultFrom) + ") "
                " AND l.created_at <= COALESCE(NULLIF($3,'')::timestamptz, now())),0) AS commission_earned, "
                "COALESCE((SELECT balance_after FROM magma_agent_ledger l WHERE l.agent_id=a.id "
                " ORDER BY l.created_at DESC LIMIT 1),0) AS wallet_balance "
                "FROM magma_agents a WHERE a.business_id=$1 ORDER BY commission_earned DESC",
                businessId, from, to);
            co_return okResponse("Agent performance", rowsToJson(rows));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getAgentPerformanceReport error: " << e.what();
            co_return errorResponse("Unable to build agent report");
        }
    }

    Task<dto::BaseApiResponse> MagmaReportService::getTaxSummary(
        const std::string &businessId, const std::string &from, const std::string &to) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT COALESCE(SUM((tax_breakdown->>'vat')::numeric),0) AS vat, "
                "COALESCE(SUM((tax_breakdown->>'nhil')::numeric),0) AS nhil, "
                "COALESCE(SUM((tax_breakdown->>'getFund')::numeric),0) AS get_fund, "
                "COALESCE(SUM(tax_amount),0) AS total_tax, "
                "COALESCE(SUM(amount),0) AS gross "
                "FROM magma_transactions WHERE business_id=$1 AND status='SUCCESS' "
                "AND paid_at >= COALESCE(NULLIF($2,'')::timestamptz, " + std::string(kDefaultFrom) + ") "
                "AND paid_at <= COALESCE(NULLIF($3,'')::timestamptz, now())", businessId, from, to);
            co_return okResponse("Tax summary",
                                 rows.empty() ? Json::Value(Json::objectValue) : jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getTaxSummary error: " << e.what();
            co_return errorResponse("Unable to build tax summary");
        }
    }

    Task<dto::BaseApiResponse> MagmaReportService::exportCsv(
        const std::string &businessId, const std::string &reportType,
        const std::string &from, const std::string &to) {
        try {
            auto db = app().getDbClient();
            std::string filename;
            std::vector<std::string> headers;
            std::vector<std::vector<std::string>> records;

            if (reportType == "vouchers") {
                auto rows = co_await db->execSqlCoro(
                    "SELECT v.serial, p.plan_code, v.status, v.price, v.sold_at, v.redeemed_at "
                    "FROM magma_vouchers v JOIN magma_data_plans p ON p.id=v.plan_id "
                    "WHERE v.business_id=$1 ORDER BY v.created_at DESC LIMIT 5000", businessId);
                headers = {"serial", "plan_code", "status", "price", "sold_at", "redeemed_at"};
                for (const auto &row : rows) {
                    records.push_back({cell(row, "serial"), cell(row, "plan_code"),
                                       cell(row, "status"), cell(row, "price"),
                                       cell(row, "sold_at"), cell(row, "redeemed_at")});
                }
                filename = "vouchers.csv";
            } else if (reportType == "subscribers") {
                auto rows = co_await db->execSqlCoro(
                    "SELECT id, msisdn, full_name, subscriber_type, status, home_zone_id, created_at "
                    "FROM magma_subscribers WHERE business_id=$1 ORDER BY created_at DESC LIMIT 5000",
                    businessId);
                headers = {"id", "msisdn", "full_name", "subscriber_type", "status",
                           "home_zone_id", "created_at"};
                for (const auto &row : rows) {
                    records.push_back({cell(row, "id"), cell(row, "msisdn"),
                                       cell(row, "full_name"), cell(row, "subscriber_type"),
                                       cell(row, "status"), cell(row, "home_zone_id"),
                                       cell(row, "created_at")});
                }
                filename = "subscribers.csv";
            } else if (reportType == "transactions") {
                auto rows = co_await db->execSqlCoro(
                    "SELECT internal_reference, purpose, amount, tax_amount, payment_method, provider, "
                    "status, paid_at, created_at FROM magma_transactions WHERE business_id=$1 "
                    "AND created_at >= COALESCE(NULLIF($2,'')::timestamptz, now() - interval '90 days') "
                    "AND created_at <= COALESCE(NULLIF($3,'')::timestamptz, now()) "
                    "ORDER BY created_at DESC LIMIT 5000", businessId, from, to);
                headers = {"internal_reference", "purpose", "amount", "tax_amount", "payment_method",
                           "provider", "status", "paid_at", "created_at"};
                for (const auto &row : rows) {
                    records.push_back({cell(row, "internal_reference"), cell(row, "purpose"),
                                       cell(row, "amount"), cell(row, "tax_amount"),
                                       cell(row, "payment_method"), cell(row, "provider"),
                                       cell(row, "status"), cell(row, "paid_at"),
                                       cell(row, "created_at")});
                }
                filename = "transactions.csv";
            } else {
                co_return errorResponse("Unknown report type");
            }

            auto escape = [](const std::string &value) {
                if (value.find(',') == std::string::npos &&
                    value.find('"') == std::string::npos &&
                    value.find('\n') == std::string::npos) {
                    return value;
                }
                std::string escaped;
                for (char c : value) {
                    if (c == '"') escaped += "\"\"";
                    else escaped += c;
                }
                return "\"" + escaped + "\"";
            };

            std::ostringstream csv;
            for (size_t i = 0; i < headers.size(); ++i) {
                if (i > 0) csv << ",";
                csv << headers[i];
            }
            csv << "\n";
            for (const auto &record : records) {
                for (size_t i = 0; i < record.size(); ++i) {
                    if (i > 0) csv << ",";
                    csv << escape(record[i]);
                }
                csv << "\n";
            }

            Json::Value result;
            result["filename"] = filename;
            result["rows"] = static_cast<int64_t>(records.size());
            result["csv"] = csv.str();
            co_return okResponse("CSV export", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] exportCsv error: " << e.what();
            co_return errorResponse("Unable to export report");
        }
    }

} // namespace data_support::services
