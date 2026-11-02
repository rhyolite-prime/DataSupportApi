//
// MagmaNet — agents, stock, commission wallet and settlement.
//

#include "MagmaAgentService.h"

#include <bcrypt.h>

#include "MagmaSettings.h"
#include "MagmaSmsService.h"
#include "MagmaSupport.h"
#include "MagmaVoucherService.h"
#include "services/magma_pay/MagmaPayService.h"
#include "utils/DbUtils.h"
#include "utils/HmacUtils.h"
#include "utils/PhoneNumberFormatter.h"

using namespace drogon;
using namespace data_support::utils;

namespace data_support::services {

    namespace {
        std::string normaliseMsisdn(const std::string &raw) {
            const std::string formatted = PhoneNumberFormatter::formatPhoneNumber(raw);
            return formatted.empty() ? trimCopy(raw) : formatted;
        }

        Task<> recordLedger(const std::string &businessId, const std::string &agentId,
                            const std::string &entryType, double amount,
                            const std::string &reference, const std::string &voucherId,
                            const std::string &description) {
            auto db = app().getDbClient();
            co_await db->execSqlCoro(
                "INSERT INTO magma_agent_ledger (id, business_id, agent_id, entry_type, amount, "
                "balance_after, reference, voucher_id, description) "
                "SELECT $1,$2,$3,$4,$5, "
                "COALESCE((SELECT balance_after FROM magma_agent_ledger WHERE agent_id=$3 "
                "          ORDER BY created_at DESC LIMIT 1),0) + $5, $6, NULLIF($7,'')::uuid, $8",
                newId(), businessId, agentId, entryType, amount, reference, voucherId, description);
            co_return;
        }
    }

    MagmaAgentService::MagmaAgentService(MagmaVoucherService &vouchers, MagmaPayService &payments,
                                         MagmaSmsService &sms)
        : vouchers_(vouchers), payments_(payments), sms_(sms) {}

    Task<std::string> MagmaAgentService::resolveAgentId(const std::string &businessId,
                                                        const std::string &identifier) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT id FROM magma_agents WHERE business_id=$1 "
                "AND (msisdn=$2 OR lower(COALESCE(email,''))=lower($2)) LIMIT 1",
                businessId, identifier);
            if (rows.empty()) {
                auto normalised = co_await db->execSqlCoro(
                    "SELECT id FROM magma_agents WHERE business_id=$1 AND msisdn=$2 LIMIT 1",
                    businessId, normaliseMsisdn(identifier));
                if (!normalised.empty()) {
                    co_return normalised[0]["id"].as<std::string>();
                }
                co_return "";
            }
            co_return rows[0]["id"].as<std::string>();
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] resolveAgentId error: " << e.what();
            co_return "";
        }
    }

    Task<double> MagmaAgentService::getBalance(const std::string &businessId,
                                               const std::string &agentId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT balance_after FROM magma_agent_ledger "
                "WHERE agent_id=$1 AND business_id=$2 ORDER BY created_at DESC LIMIT 1",
                agentId, businessId);
            if (!rows.empty()) {
                co_return std::stod(rows[0]["balance_after"].as<std::string>());
            }
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getBalance error: " << e.what();
        }
        co_return 0.0;
    }

    Task<dto::BaseApiResponse> MagmaAgentService::createAgent(
        const std::string &businessId, const dto::magma_net::AgentDto &dto) {
        if (dto.name.empty() || dto.msisdn.empty() || dto.pin.empty()) {
            co_return errorResponse("name, msisdn and pin are required");
        }
        if (dto.pin.size() < 4) {
            co_return errorResponse("PIN must be at least 4 digits");
        }
        try {
            auto db = app().getDbClient();
            const std::string id = newId();
            const std::string agentCode =
                "AG-" + HmacUtils::randomHex(3);
            auto rows = co_await db->execSqlCoro(
                "INSERT INTO magma_agents (id, business_id, agent_code, name, msisdn, email, pin_hash, "
                "tier, commission_rate, territory_zone_id, status, kyc_status) "
                "VALUES ($1,$2,$3,$4,$5,NULLIF($6,''),$7,$8,$9,NULLIF($10,'')::uuid,'ACTIVE',$11) "
                "RETURNING id, agent_code, name, msisdn, email, tier, commission_rate, status",
                id, businessId, agentCode, dto.name, normaliseMsisdn(dto.msisdn), dto.email,
                bcrypt::generateHash(dto.pin, 8), dto.tier, dto.commissionRate,
                dto.territoryZoneId, dto.kycStatus);

            co_await magma::auditLog(businessId, "STAFF", "", "AGENT_CREATED", "magma_agents",
                                     id, jsonRow(rows[0]));
            co_return okResponse("Agent created", jsonRow(rows[0]));
        } catch (const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << "[MagmaNet] createAgent db error: " << e.base().what();
            co_return errorResponse("Unable to create agent (msisdn already registered)");
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] createAgent error: " << e.what();
            co_return errorResponse("Unable to create agent");
        }
    }

    Task<dto::BaseApiResponse> MagmaAgentService::updateAgent(
        const std::string &businessId, const std::string &agentId,
        const dto::magma_net::AgentDto &dto) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "UPDATE magma_agents SET name=COALESCE(NULLIF($3,''),name), "
                "email=COALESCE(NULLIF($4,''),email), tier=COALESCE(NULLIF($5,''),tier), "
                "commission_rate=CASE WHEN $6 > 0 THEN $6 ELSE commission_rate END, "
                "territory_zone_id=COALESCE(NULLIF($7,'')::uuid, territory_zone_id), "
                "kyc_status=COALESCE(NULLIF($8,''),kyc_status), updated_at=now() "
                "WHERE id=$1 AND business_id=$2 "
                "RETURNING id, agent_code, name, msisdn, email, tier, commission_rate, status",
                agentId, businessId, dto.name, dto.email, dto.tier, dto.commissionRate,
                dto.territoryZoneId, dto.kycStatus);
            if (rows.empty()) {
                co_return errorResponse("Agent not found");
            }
            co_return okResponse("Agent updated", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] updateAgent error: " << e.what();
            co_return errorResponse("Unable to update agent");
        }
    }

    Task<dto::BaseApiResponse> MagmaAgentService::listAgents(const std::string &businessId,
                                                             int pageNo, int pageSize,
                                                             const std::string &status) {
        try {
            auto db = app().getDbClient();
            if (pageNo < 1) pageNo = 1;
            if (pageSize < 1 || pageSize > 200) pageSize = 50;
            auto rows = co_await db->execSqlCoro(
                "SELECT a.id, a.agent_code, a.name, a.msisdn, a.email, a.tier, a.commission_rate, "
                "a.status, a.kyc_status, a.created_at, "
                "COALESCE((SELECT balance_after FROM magma_agent_ledger l WHERE l.agent_id=a.id "
                "          ORDER BY l.created_at DESC LIMIT 1),0) AS wallet_balance, "
                "(SELECT COUNT(*) FROM magma_vouchers v WHERE v.agent_id=a.id AND v.status='ASSIGNED') AS stock_available "
                "FROM magma_agents a WHERE a.business_id=$1 AND ($2='' OR a.status=$2) "
                "ORDER BY a.created_at DESC LIMIT $3 OFFSET $4",
                businessId, status, pageSize, (pageNo - 1) * pageSize);
            co_return okResponse("Agents", rowsToJson(rows));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] listAgents error: " << e.what();
            co_return errorResponse("Unable to list agents");
        }
    }

    Task<dto::BaseApiResponse> MagmaAgentService::agentLogin(
        const std::string &businessId, const dto::magma_net::AgentAuthDto &dto) {
        const bool allowed = co_await magma::rateLimit(
            "agentlogin:" + dto.getAgentIdentifier(), 8, 600);
        if (!allowed) {
            co_return errorResponse("Too many login attempts. Try again later.");
        }

        try {
            auto db = app().getDbClient();
            const std::string agentId = co_await resolveAgentId(businessId, dto.getAgentIdentifier());
            if (agentId.empty()) {
                co_return errorResponse("Invalid agent credentials");
            }
            auto rows = co_await db->execSqlCoro(
                "SELECT id, agent_code, name, pin_hash, status FROM magma_agents "
                "WHERE id=$1 AND business_id=$2", agentId, businessId);
            if (rows.empty() || rows[0]["status"].as<std::string>() != "ACTIVE") {
                co_return errorResponse("Agent account is not active");
            }
            if (!bcrypt::validatePassword(dto.getPin(), rows[0]["pin_hash"].as<std::string>())) {
                co_return errorResponse("Invalid agent credentials");
            }

            const std::string token = HmacUtils::signToken(
                MagmaSettings::instance().hmacKey, businessId, agentId, "AGENT", 43200);

            Json::Value result;
            result["token"] = token;
            result["agentId"] = agentId;
            result["agentCode"] = rows[0]["agent_code"];
            result["name"] = rows[0]["name"];
            result["walletBalance"] = co_await getBalance(businessId, agentId);
            co_return okResponse("Agent logged in", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] agentLogin error: " << e.what();
            co_return errorResponse("Unable to log in");
        }
    }

    Task<dto::BaseApiResponse> MagmaAgentService::changeAgentPin(
        const std::string &businessId, const dto::magma_net::AgentAuthDto &dto,
        const std::string &newPin) {
        if (newPin.size() < 4) {
            co_return errorResponse("New PIN must be at least 4 digits");
        }
        try {
            auto db = app().getDbClient();
            const std::string agentId = co_await resolveAgentId(businessId, dto.getAgentIdentifier());
            if (agentId.empty()) {
                co_return errorResponse("Agent not found");
            }
            auto rows = co_await db->execSqlCoro(
                "SELECT pin_hash FROM magma_agents WHERE id=$1 AND business_id=$2", agentId, businessId);
            if (rows.empty() ||
                !bcrypt::validatePassword(dto.getPin(), rows[0]["pin_hash"].as<std::string>())) {
                co_return errorResponse("Current PIN is incorrect");
            }
            co_await db->execSqlCoro(
                "UPDATE magma_agents SET pin_hash=$2, updated_at=now() WHERE id=$1",
                agentId, bcrypt::generateHash(newPin, 8));
            co_return okResponse("PIN changed", Json::Value(Json::objectValue));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] changeAgentPin error: " << e.what();
            co_return errorResponse("Unable to change PIN");
        }
    }

    Task<dto::BaseApiResponse> MagmaAgentService::requestAgentPinReset(
        const std::string &businessId, const std::string &agentIdentifier) {
        try {
            const std::string agentId = co_await resolveAgentId(businessId, agentIdentifier);
            if (agentId.empty()) {
                co_return okResponse("If the agent exists, a code has been sent",
                                     Json::Value(Json::objectValue));
            }
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT msisdn FROM magma_agents WHERE id=$1 AND business_id=$2", agentId, businessId);
            if (rows.empty()) {
                co_return okResponse("If the agent exists, a code has been sent",
                                     Json::Value(Json::objectValue));
            }
            const std::string otp = IdGeneratorUtils::generateRandomSixDigit();
            auto redis = app().getRedisClient();
            if (redis) {
                co_await redis->execCommandCoro("SETEX %s %d %s",
                                                ("magma:otp:agent:" + agentId).c_str(),
                                                MagmaSettings::instance().otpTtlSeconds,
                                                otp.c_str());
            }
            co_await sms_.sendOtp(businessId, rows[0]["msisdn"].as<std::string>(), otp);

            Json::Value result;
            result["agentId"] = agentId;
            result["delivery"] = "SMS";
            co_return okResponse("Verification code sent", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] requestAgentPinReset error: " << e.what();
            co_return errorResponse("Unable to start PIN reset");
        }
    }

    Task<dto::BaseApiResponse> MagmaAgentService::resetAgentPin(
        const std::string &businessId, const dto::magma_net::AgentPinResetDto &dto) {
        if (dto.newPin.size() < 4) {
            co_return errorResponse("New PIN must be at least 4 digits");
        }
        try {
            const std::string agentId = co_await resolveAgentId(businessId, dto.agentIdentifier);
            if (agentId.empty()) {
                co_return errorResponse("Agent not found");
            }
            auto redis = app().getRedisClient();
            if (!redis) {
                co_return errorResponse("Verification service unavailable");
            }
            auto stored = co_await redis->execCommandCoro("GET %s",
                                                          ("magma:otp:agent:" + agentId).c_str());
            if (stored.type() != nosql::RedisResultType::kString ||
                !HmacUtils::secureEquals(stored.asString(), dto.otp)) {
                co_return errorResponse("Invalid or expired verification code");
            }
            auto db = app().getDbClient();
            co_await db->execSqlCoro(
                "UPDATE magma_agents SET pin_hash=$2, updated_at=now() WHERE id=$1 AND business_id=$3",
                agentId, bcrypt::generateHash(dto.newPin, 8), businessId);
            co_await redis->execCommandCoro("DEL %s", ("magma:otp:agent:" + agentId).c_str());
            co_return okResponse("PIN reset", Json::Value(Json::objectValue));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] resetAgentPin error: " << e.what();
            co_return errorResponse("Unable to reset PIN");
        }
    }

    Task<dto::BaseApiResponse> MagmaAgentService::getAgentDashboard(
        const std::string &businessId, const std::string &agentId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT a.id, a.agent_code, a.name, a.msisdn, a.tier, a.commission_rate, "
                "COALESCE((SELECT balance_after FROM magma_agent_ledger l WHERE l.agent_id=a.id "
                "          ORDER BY l.created_at DESC LIMIT 1),0) AS wallet_balance, "
                "(SELECT COUNT(*) FROM magma_vouchers v WHERE v.agent_id=a.id AND v.status='ASSIGNED') AS stock_available, "
                "(SELECT COUNT(*) FROM magma_vouchers v WHERE v.agent_id=a.id AND v.status='REDEEMED' "
                " AND v.redeemed_at >= date_trunc('day', now())) AS sold_today, "
                "(SELECT COUNT(*) FROM magma_vouchers v WHERE v.agent_id=a.id AND v.status='REDEEMED' "
                " AND v.redeemed_at >= date_trunc('month', now())) AS sold_month, "
                "COALESCE((SELECT SUM(amount) FROM magma_agent_ledger l WHERE l.agent_id=a.id "
                "          AND l.entry_type='EARNING' AND l.created_at >= date_trunc('month', now())),0) AS commission_month "
                "FROM magma_agents a WHERE a.id=$1 AND a.business_id=$2", agentId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Agent not found");
            }
            co_return okResponse("Agent dashboard", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getAgentDashboard error: " << e.what();
            co_return errorResponse("Unable to load dashboard");
        }
    }

    Task<dto::BaseApiResponse> MagmaAgentService::getAgentTransactions(
        const std::string &businessId, const std::string &agentId, int pageNo, int pageSize) {
        try {
            auto db = app().getDbClient();
            if (pageNo < 1) pageNo = 1;
            if (pageSize < 1 || pageSize > 200) pageSize = 50;
            auto rows = co_await db->execSqlCoro(
                "SELECT id, entry_type, amount, balance_after, reference, description, created_at "
                "FROM magma_agent_ledger WHERE agent_id=$1 AND business_id=$2 "
                "ORDER BY created_at DESC LIMIT $3 OFFSET $4",
                agentId, businessId, pageSize, (pageNo - 1) * pageSize);
            co_return okResponse("Agent transactions", rowsToJson(rows));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getAgentTransactions error: " << e.what();
            co_return errorResponse("Unable to load transactions");
        }
    }

    Task<dto::BaseApiResponse> MagmaAgentService::getAgentStatement(
        const std::string &businessId, const std::string &agentId,
        const std::string &from, const std::string &to) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT id, entry_type, amount, balance_after, reference, description, created_at "
                "FROM magma_agent_ledger WHERE agent_id=$1 AND business_id=$2 "
                "AND created_at >= COALESCE(NULLIF($3,'')::timestamptz, date_trunc('month', now())) "
                "AND created_at <= COALESCE(NULLIF($4,'')::timestamptz, now()) "
                "ORDER BY created_at", agentId, businessId, from, to);
            auto totals = co_await db->execSqlCoro(
                "SELECT COALESCE(SUM(amount) FILTER (WHERE entry_type='EARNING'),0) AS earned, "
                "COALESCE(SUM(amount) FILTER (WHERE entry_type IN ('PAYOUT','STOCK_PURCHASE')),0) AS outflow "
                "FROM magma_agent_ledger WHERE agent_id=$1 AND business_id=$2 "
                "AND created_at >= COALESCE(NULLIF($3,'')::timestamptz, date_trunc('month', now()))",
                agentId, businessId, from);

            Json::Value result;
            result["entries"] = rowsToJson(rows);
            result["totals"] = totals.empty() ? Json::Value(Json::objectValue) : jsonRow(totals[0]);
            result["balance"] = co_await getBalance(businessId, agentId);
            co_return okResponse("Agent statement", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getAgentStatement error: " << e.what();
            co_return errorResponse("Unable to load statement");
        }
    }

    Task<dto::BaseApiResponse> MagmaAgentService::purchaseBatch(
        const std::string &businessId, const dto::magma_net::AgentStockPurchaseDto &dto) {
        try {
            auto db = app().getDbClient();
            if (dto.quantity <= 0) {
                co_return errorResponse("quantity must be greater than zero");
            }
            auto planRows = co_await db->execSqlCoro(
                "SELECT id, plan_code, wholesale_price, price FROM magma_data_plans "
                "WHERE business_id=$1 AND (id::text=$2 OR plan_code=$2) AND is_active LIMIT 1",
                businessId, dto.planCode);
            if (planRows.empty()) {
                co_return errorResponse("Plan not found");
            }
            const double unitCost = planRows[0]["wholesale_price"].isNull()
                                        ? std::stod(planRows[0]["price"].as<std::string>())
                                        : std::stod(planRows[0]["wholesale_price"].as<std::string>());
            const double totalCost = unitCost * dto.quantity;
            const double balance = co_await getBalance(businessId, dto.agentId);
            if (balance < totalCost) {
                co_return errorResponse("Insufficient wallet balance");
            }

            dto::magma_net::VoucherBatchDto batch;
            batch.planId = planRows[0]["id"].as<std::string>();
            batch.quantity = dto.quantity;
            batch.zoneId = dto.zoneId;
            batch.ownerAgentId = dto.agentId;
            batch.unitPrice = std::stod(planRows[0]["price"].as<std::string>());
            batch.wholesalePrice = unitCost;

            auto created = co_await vouchers_.createVoucherBatch(businessId, batch);
            if (!created.success) {
                co_return created;
            }

            const std::string batchId = created.result["batchId"].as<std::string>();
            co_await recordLedger(businessId, dto.agentId, "STOCK_PURCHASE", -totalCost,
                                  "SP-" + batchId.substr(0, 8), "", "Voucher stock purchase");
            created.result["walletBalance"] = balance - totalCost;
            created.result["amountCharged"] = totalCost;
            co_return okResponse("Stock purchased", created.result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] purchaseBatch error: " << e.what();
            co_return errorResponse("Unable to purchase stock");
        }
    }

    Task<dto::BaseApiResponse> MagmaAgentService::initializeSettlement(
        const std::string &businessId, const dto::magma_net::AgentSettlementDto &dto) {
        if (dto.amount <= 0) {
            co_return errorResponse("amount must be greater than zero");
        }
        try {
            auto db = app().getDbClient();
            const double balance = co_await getBalance(businessId, dto.agentId);
            if (balance < dto.amount) {
                co_return errorResponse("Settlement exceeds wallet balance");
            }

            auto agent = co_await db->execSqlCoro(
                "SELECT name, msisdn FROM magma_agents WHERE id=$1 AND business_id=$2",
                dto.agentId, businessId);
            if (agent.empty()) {
                co_return errorResponse("Agent not found");
            }

            const std::string transactionId = newId();
            const std::string internalReference = "AS-" + HmacUtils::randomHex(6);
            const std::string msisdn = dto.msisdn.empty() ? agent[0]["msisdn"].as<std::string>()
                                                          : dto.msisdn;

            co_await db->execSqlCoro(
                "INSERT INTO magma_transactions (id, business_id, agent_id, purpose, amount, "
                "payment_method, provider, internal_reference, status) "
                "VALUES ($1,$2,$3,'SETTLEMENT',$4,$5,'HUBTEL',$6,'PENDING')",
                transactionId, businessId, dto.agentId, dto.amount,
                dto.provider.empty() ? "MOMO_MTN" : dto.provider, internalReference);

            auto payout = co_await payments_.payoutMomo(
                businessId, msisdn, agent[0]["name"].as<std::string>(), dto.amount,
                internalReference, "MagmaNet agent settlement",
                MagmaSettings::instance().portalBaseUrl + "/api/v1/magma/webhooks/hubtel/settlement",
                dto.provider);

            if (!payout.success) {
                co_await db->execSqlCoro(
                    "UPDATE magma_transactions SET status='FAILED', failure_reason=$2, updated_at=now() "
                    "WHERE id=$1", transactionId, payout.message);
                co_return errorResponse(payout.message);
            }

            co_await recordLedger(businessId, dto.agentId, "PAYOUT", -dto.amount,
                                  internalReference, "", "Settlement to " + msisdn);
            co_await db->execSqlCoro(
                "UPDATE magma_transactions SET provider_reference=$2, updated_at=now() WHERE id=$1",
                transactionId, payout.providerReference);

            Json::Value result;
            result["transactionId"] = transactionId;
            result["reference"] = internalReference;
            result["amount"] = dto.amount;
            result["status"] = "PENDING_CONFIRMATION";
            result["walletBalance"] = balance - dto.amount;
            co_return okResponse("Settlement submitted", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] initializeSettlement error: " << e.what();
            co_return errorResponse("Unable to submit settlement");
        }
    }

    Task<dto::BaseApiResponse> MagmaAgentService::completeSettlementByReference(
        const std::string &reference) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT business_id FROM magma_transactions "
                "WHERE internal_reference=$1 AND purpose='SETTLEMENT' LIMIT 1", reference);
            if (rows.empty()) {
                co_return errorResponse("Settlement not found");
            }
            co_return co_await completeSettlement(rows[0]["business_id"].as<std::string>(), reference);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] completeSettlementByReference error: " << e.what();
            co_return errorResponse("Unable to confirm settlement");
        }
    }

    Task<dto::BaseApiResponse> MagmaAgentService::completeSettlement(
        const std::string &businessId, const std::string &reference) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT id, agent_id, amount, provider, provider_reference FROM magma_transactions "
                "WHERE business_id=$1 AND internal_reference=$2 AND purpose='SETTLEMENT'",
                businessId, reference);
            if (rows.empty()) {
                co_return errorResponse("Settlement not found");
            }
            auto verification = co_await payments_.verify(
                businessId, rows[0]["provider"].as<std::string>(),
                rows[0]["provider_reference"].isNull() ? reference
                                                       : rows[0]["provider_reference"].as<std::string>(),
                reference);

            if (!verification.paid) {
                co_return errorResponse("Payout is not confirmed yet");
            }

            co_await db->execSqlCoro(
                "UPDATE magma_transactions SET status='SUCCESS', paid_at=now(), updated_at=now() "
                "WHERE id=$1 AND status='PENDING'", rows[0]["id"].as<std::string>());

            Json::Value result;
            result["reference"] = reference;
            result["status"] = "PAID";
            co_return okResponse("Settlement confirmed", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] completeSettlement error: " << e.what();
            co_return errorResponse("Unable to confirm settlement");
        }
    }

} // namespace data_support::services
