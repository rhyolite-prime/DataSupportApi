//
// MagmaNet — voucher batches, distribution and redemption.
//

#include "MagmaVoucherService.h"

#include <sstream>

#include "MagmaBillingService.h"
#include "MagmaSettings.h"
#include "MagmaSubscriberService.h"
#include "MagmaSupport.h"
#include "utils/DbUtils.h"
#include "utils/HmacUtils.h"

using namespace drogon;
using namespace data_support::utils;

namespace data_support::services {

    namespace {
        std::string voucherHash(const std::string &normalisedCode) {
            const auto &settings = MagmaSettings::instance();
            return HmacUtils::hmacSha256Hex(settings.hmacKey + "|" + settings.voucherPepper,
                                            normalisedCode);
        }

    }

    MagmaVoucherService::MagmaVoucherService(MagmaSubscriberService &subscribers,
                                             MagmaBillingService &billing)
        : subscribers_(subscribers), billing_(billing) {}

    Task<dto::BaseApiResponse> MagmaVoucherService::createVoucherBatch(
        const std::string &businessId, const dto::magma_net::VoucherBatchDto &dto) {
        const auto &settings = MagmaSettings::instance();
        if (dto.quantity <= 0) {
            co_return errorResponse("quantity must be greater than zero");
        }
        if (dto.quantity > settings.maxVoucherBatch) {
            co_return errorResponse("quantity exceeds the maximum batch size of " +
                                    std::to_string(settings.maxVoucherBatch));
        }

        try {
            auto db = app().getDbClient();

            auto planRows = co_await db->execSqlCoro(
                "SELECT * FROM magma_data_plans WHERE business_id=$1 "
                "AND ($2='' OR id::text=$2) AND ($3='' OR plan_code=$3) AND is_active LIMIT 1",
                businessId, dto.planId, dto.planCode);
            if (planRows.empty()) {
                co_return errorResponse("Active plan not found");
            }
            const auto &plan = planRows[0];
            const std::string planId = plan["id"].as<std::string>();
            const std::string planCode = plan["plan_code"].as<std::string>();
            const double unitPrice = dto.unitPrice > 0
                                         ? dto.unitPrice
                                         : std::stod(plan["price"].as<std::string>());
            const double wholesale =
                dto.wholesalePrice > 0
                    ? dto.wholesalePrice
                    : (plan["wholesale_price"].isNull()
                           ? unitPrice
                           : std::stod(plan["wholesale_price"].as<std::string>()));
            const double commission = plan["agent_commission"].isNull()
                                          ? 0.0
                                          : std::stod(plan["agent_commission"].as<std::string>());

            std::string zoneId = dto.zoneId;
            if (!zoneId.empty()) {
                auto zone = co_await db->execSqlCoro(
                    "SELECT id FROM magma_zones WHERE id=$1 AND business_id=$2", zoneId, businessId);
                if (zone.empty()) {
                    co_return errorResponse("Zone not found");
                }
            }

            const std::string batchId = newId();
            const std::string batchCode = "VB-" + HmacUtils::randomHex(4);
            const int expiresInDays = dto.expiresInDays > 0 ? dto.expiresInDays : 90;

            co_await db->execSqlCoro(
                "INSERT INTO magma_voucher_batches (id, business_id, batch_code, plan_id, zone_id, "
                "quantity, unit_price, wholesale_price, owner_type, owner_agent_id, status, expires_at) "
                "VALUES ($1,$2,$3,$4,NULLIF($5,'')::uuid,$6,$7,$8,$9,NULLIF($10,'')::uuid,'DRAFT', "
                "now() + ($11 * interval '1 day'))",
                batchId, businessId, batchCode, planId, zoneId, dto.quantity,
                unitPrice, wholesale, dto.ownerAgentId.empty() ? "BUSINESS" : "AGENT",
                dto.ownerAgentId, expiresInDays);

            std::vector<std::string> sampleCodes;
            for (int i = 0; i < dto.quantity; ++i) {
                const std::string code = HmacUtils::generateVoucherCode(3, 4);
                const std::string normalised = HmacUtils::normaliseVoucherCode(code);
                const std::string hash = voucherHash(normalised);
                const std::string cipher = magma::encryptSecret(code);
                const std::string serial = "S" + HmacUtils::randomHex(6);
                const std::string voucherId = newId();

                co_await db->execSqlCoro(
                    "INSERT INTO magma_vouchers (id, business_id, batch_id, plan_id, zone_id, serial, "
                    "code_hash, code_ciphertext, status, price, commission) "
                    "VALUES ($1,$2,$3,$4,NULLIF($5,'')::uuid,$6, $7, "
                    "$8, $9, $10, $11)",
                    voucherId, businessId, batchId, planId, zoneId, serial, hash, cipher,
                    dto.ownerAgentId.empty() ? "AVAILABLE" : "ASSIGNED",
                    unitPrice, commission);

                if (sampleCodes.size() < 3) {
                    sampleCodes.push_back(code);
                }
            }

            co_await db->execSqlCoro(
                "UPDATE magma_voucher_batches SET status='ACTIVE' WHERE id=$1", batchId);
            co_await magma::auditLog(businessId, "STAFF", "", "VOUCHER_BATCH_CREATED",
                                     "magma_voucher_batches", batchId,
                                     Json::Value(static_cast<Json::Int64>(dto.quantity)));

            Json::Value result;
            result["batchId"] = batchId;
            result["batchCode"] = batchCode;
            result["plan"] = planCode;
            result["quantity"] = dto.quantity;
            result["unitPrice"] = unitPrice;
            result["wholesalePrice"] = wholesale;
            result["expiresInDays"] = expiresInDays;
            result["sampleCodes"] = Json::Value(Json::arrayValue);
            for (const auto &code : sampleCodes) {
                result["sampleCodes"].append(code);
            }
            co_return okResponse("Voucher batch created", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] createVoucherBatch error: " << e.what();
            co_return errorResponse("Unable to create voucher batch");
        }
    }

    Task<dto::BaseApiResponse> MagmaVoucherService::listVoucherBatches(
        const std::string &businessId, int pageNo, int pageSize) {
        try {
            auto db = app().getDbClient();
            if (pageNo < 1) pageNo = 1;
            if (pageSize < 1 || pageSize > 200) pageSize = 50;

            auto rows = co_await db->execSqlCoro(
                "SELECT b.*, p.plan_code, p.name AS plan_name, z.zone_code, "
                "(SELECT COUNT(*) FROM magma_vouchers v WHERE v.batch_id=b.id) AS total, "
                "(SELECT COUNT(*) FROM magma_vouchers v WHERE v.batch_id=b.id AND v.status='AVAILABLE') AS available, "
                "(SELECT COUNT(*) FROM magma_vouchers v WHERE v.batch_id=b.id AND v.status='REDEEMED') AS redeemed "
                "FROM magma_voucher_batches b JOIN magma_data_plans p ON p.id=b.plan_id "
                "LEFT JOIN magma_zones z ON z.id=b.zone_id "
                "WHERE b.business_id=$1 ORDER BY b.created_at DESC LIMIT $2 OFFSET $3",
                businessId, pageSize, (pageNo - 1) * pageSize);

            Json::Value result;
            result["batches"] = rowsToJson(rows);
            result["pageNo"] = pageNo;
            result["pageSize"] = pageSize;
            co_return okResponse("Voucher batches", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] listVoucherBatches error: " << e.what();
            co_return errorResponse("Unable to list batches");
        }
    }

    Task<dto::BaseApiResponse> MagmaVoucherService::getVoucherBatch(
        const std::string &businessId, const std::string &batchId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT b.*, p.plan_code, p.name AS plan_name, z.zone_code FROM magma_voucher_batches b "
                "JOIN magma_data_plans p ON p.id=b.plan_id LEFT JOIN magma_zones z ON z.id=b.zone_id "
                "WHERE b.id=$1 AND b.business_id=$2", batchId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Batch not found");
            }
            Json::Value batch = jsonRow(rows[0]);

            auto vouchers = co_await db->execSqlCoro(
                "SELECT id, serial, status, price, agent_id, sold_at, redeemed_at, created_at "
                "FROM magma_vouchers WHERE batch_id=$1 ORDER BY created_at LIMIT 500", batchId);
            batch["vouchers"] = rowsToJson(vouchers);
            co_return okResponse("Voucher batch", batch);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getVoucherBatch error: " << e.what();
            co_return errorResponse("Unable to load batch");
        }
    }

    Task<dto::BaseApiResponse> MagmaVoucherService::downloadVoucherBatch(
        const std::string &businessId, const std::string &batchId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT v.serial, v.code_ciphertext, v.status, v.price, p.plan_code, p.name AS plan_name, "
                "b.expires_at FROM magma_vouchers v "
                "JOIN magma_voucher_batches b ON b.id=v.batch_id "
                "JOIN magma_data_plans p ON p.id=v.plan_id "
                "WHERE v.batch_id=$1 AND v.business_id=$2 ORDER BY v.created_at", batchId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Batch not found or empty");
            }

            std::ostringstream csv;
            csv << "serial,code,plan,plan_name,status,price,expires_at\n";
            for (const auto &row : rows) {
                const std::string code = magma::decryptSecret(
                    row["code_ciphertext"].isNull() ? "" : row["code_ciphertext"].as<std::string>());
                csv << row["serial"].as<std::string>() << ","
                    << (code.empty() ? "UNAVAILABLE" : code) << ","
                    << row["plan_code"].as<std::string>() << ","
                    << "\"" << row["plan_name"].as<std::string>() << "\","
                    << row["status"].as<std::string>() << ","
                    << row["price"].as<std::string>() << ","
                    << (row["expires_at"].isNull() ? "" : row["expires_at"].as<std::string>())
                    << "\n";
            }

            Json::Value result;
            result["batchId"] = batchId;
            result["format"] = "CSV";
            result["count"] = static_cast<int64_t>(rows.size());
            result["csv"] = csv.str();
            co_return okResponse("Voucher batch export", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] downloadVoucherBatch error: " << e.what();
            co_return errorResponse("Unable to export batch");
        }
    }

    Task<dto::BaseApiResponse> MagmaVoucherService::assignBatchToAgent(
        const std::string &businessId, const std::string &batchId, const std::string &agentId) {
        try {
            auto db = app().getDbClient();
            auto agent = co_await db->execSqlCoro(
                "SELECT id, status FROM magma_agents WHERE id=$1 AND business_id=$2", agentId, businessId);
            if (agent.empty() || agent[0]["status"].as<std::string>() != "ACTIVE") {
                co_return errorResponse("Active agent not found");
            }

            auto updated = co_await db->execSqlCoro(
                "UPDATE magma_vouchers SET agent_id=$3, status='ASSIGNED', updated_at=now() "
                "WHERE batch_id=$1 AND business_id=$2 AND status='AVAILABLE' RETURNING id",
                batchId, businessId, agentId);
            co_await db->execSqlCoro(
                "UPDATE magma_voucher_batches SET owner_type='AGENT', owner_agent_id=$3 WHERE id=$1 AND business_id=$2",
                batchId, businessId, agentId);

            Json::Value result;
            result["batchId"] = batchId;
            result["agentId"] = agentId;
            result["vouchersAssigned"] = static_cast<int64_t>(updated.size());
            co_return okResponse("Batch assigned to agent", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] assignBatchToAgent error: " << e.what();
            co_return errorResponse("Unable to assign batch");
        }
    }

    Task<dto::BaseApiResponse> MagmaVoucherService::voidVouchers(
        const std::string &businessId, const std::string &batchId,
        const std::vector<std::string> &voucherIds) {
        try {
            auto db = app().getDbClient();
            int64_t voided = 0;

            if (voucherIds.empty()) {
                auto rows = co_await db->execSqlCoro(
                    "UPDATE magma_vouchers SET status='VOID', updated_at=now() "
                    "WHERE batch_id=$1 AND business_id=$2 AND status IN ('AVAILABLE','ASSIGNED') "
                    "RETURNING id", batchId, businessId);
                voided = static_cast<int64_t>(rows.size());
            } else {
                for (const auto &voucherId : voucherIds) {
                    auto rows = co_await db->execSqlCoro(
                        "UPDATE magma_vouchers SET status='VOID', updated_at=now() "
                        "WHERE id=$1 AND business_id=$2 AND status IN ('AVAILABLE','ASSIGNED') "
                        "RETURNING id", voucherId, businessId);
                    voided += static_cast<int64_t>(rows.size());
                }
            }

            Json::Value count;
            count["voided"] = voided;
            co_return okResponse("Vouchers voided", count);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] voidVouchers error: " << e.what();
            co_return errorResponse("Unable to void vouchers");
        }
    }

    Task<dto::BaseApiResponse> MagmaVoucherService::getVoucherInventory(
        const std::string &businessId, const std::string &zoneId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT p.plan_code, p.name AS plan_name, v.status, COUNT(*) AS count, "
                "COALESCE(SUM(v.price),0) AS face_value "
                "FROM magma_vouchers v JOIN magma_data_plans p ON p.id=v.plan_id "
                "WHERE v.business_id=$1 AND ($2='' OR v.zone_id::text=$2) "
                "GROUP BY p.plan_code, p.name, v.status ORDER BY p.plan_code, v.status",
                businessId, zoneId);
            co_return okResponse("Voucher inventory", rowsToJson(rows));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getVoucherInventory error: " << e.what();
            co_return errorResponse("Unable to load inventory");
        }
    }

    Task<dto::BaseApiResponse> MagmaVoucherService::redeemVoucher(
        const dto::magma_net::VoucherRedeemDto &dto) {
        const std::string normalised = HmacUtils::normaliseVoucherCode(dto.code);
        if (normalised.empty()) {
            co_return errorResponse("Enter a voucher code");
        }
        const bool allowed = co_await magma::rateLimit("voucher:" + dto.mac, 5, 600);
        if (!allowed) {
            co_return errorResponse("Too many voucher attempts. Try again in a few minutes.");
        }

        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT v.id, v.business_id, v.plan_id, v.zone_id, v.status, v.price, v.commission, "
                "v.agent_id, b.expires_at, b.status AS batch_status, p.name AS plan_name, "
                "z.zone_code, z.id AS resolved_zone_id "
                "FROM magma_vouchers v "
                "JOIN magma_voucher_batches b ON b.id=v.batch_id "
                "JOIN magma_data_plans p ON p.id=v.plan_id "
                "LEFT JOIN magma_zones z ON z.business_id = v.business_id AND z.id = COALESCE(v.zone_id, "
                "   (SELECT id FROM magma_zones WHERE zone_code=$2 LIMIT 1)) "
                "WHERE v.code_hash = $1 LIMIT 1", voucherHash(normalised), dto.zoneCode);

            if (rows.empty()) {
                co_return errorResponse("Invalid voucher code");
            }
            const auto &voucher = rows[0];
            if (voucher["status"].as<std::string>() != "AVAILABLE" &&
                voucher["status"].as<std::string>() != "ASSIGNED" &&
                voucher["status"].as<std::string>() != "SOLD") {
                co_return errorResponse("This voucher has already been used or voided");
            }
            if (voucher["batch_status"].as<std::string>() != "ACTIVE") {
                co_return errorResponse("This voucher batch is no longer active");
            }
            if (!voucher["expires_at"].isNull()) {
                auto expired = co_await db->execSqlCoro(
                    "SELECT ($1::timestamptz < now()) AS expired",
                    voucher["expires_at"].as<std::string>());
                if (!expired.empty() && expired[0]["expired"].as<bool>()) {
                    co_return errorResponse("This voucher has expired");
                }
            }
            if (voucher["resolved_zone_id"].isNull()) {
                co_return errorResponse("Voucher is not valid for this zone");
            }

            const std::string businessId = voucher["business_id"].as<std::string>();
            const std::string voucherId = voucher["id"].as<std::string>();
            const std::string planId = voucher["plan_id"].as<std::string>();
            const std::string zoneId = voucher["resolved_zone_id"].as<std::string>();
            const std::string agentId = voucher["agent_id"].isNull()
                                            ? "" : voucher["agent_id"].as<std::string>();
            const double price = voucher["price"].isNull()
                                     ? 0 : std::stod(voucher["price"].as<std::string>());
            const double commission = voucher["commission"].isNull()
                                          ? 0 : std::stod(voucher["commission"].as<std::string>());

            std::string subscriberId;
            if (!dto.msisdn.empty()) {
                subscriberId = co_await subscribers_.ensureSubscriber(businessId, dto.msisdn, "", "", zoneId);
            } else {
                subscriberId = co_await subscribers_.findSubscriberIdByMac(businessId, dto.mac);
                if (subscriberId.empty()) {
                    subscriberId = co_await subscribers_.ensureSubscriber(businessId, "", "", "", zoneId);
                }
            }
            if (subscriberId.empty()) {
                co_return errorResponse("Unable to create the customer record");
            }

            auto claimed = co_await db->execSqlCoro(
                "UPDATE magma_vouchers SET status='REDEEMED', redeemed_at=now(), "
                "redeemed_subscriber_id=$2, updated_at=now() "
                "WHERE id=$1 AND status IN ('AVAILABLE','ASSIGNED','SOLD') RETURNING id",
                voucherId, subscriberId);
            if (claimed.empty()) {
                co_return errorResponse("Voucher was redeemed by another request");
            }

            const std::string subscriptionId = co_await billing_.createPaidSubscription(
                businessId, subscriberId, planId, zoneId, "", voucherId, agentId, price);
            if (subscriptionId.empty()) {
                // Release the voucher so the customer can try again.
                co_await db->execSqlCoro(
                    "UPDATE magma_vouchers SET status='AVAILABLE', redeemed_at=NULL, "
                    "redeemed_subscriber_id=NULL, updated_at=now() WHERE id=$1", voucherId);
                co_return errorResponse("Unable to activate the plan");
            }

            co_await db->execSqlCoro(
                "UPDATE magma_vouchers SET subscription_id=$2, sold_at=COALESCE(sold_at, now()) "
                "WHERE id=$1", voucherId, subscriptionId);
            co_await subscribers_.bindDevice(businessId, subscriberId, dto.mac);

            if (!agentId.empty() && commission > 0) {
                co_await db->execSqlCoro(
                    "INSERT INTO magma_agent_ledger (id, business_id, agent_id, entry_type, amount, "
                    "balance_after, reference, voucher_id, description) "
                    "SELECT $1,$2,$3,'EARNING',$4, "
                    "COALESCE((SELECT balance_after FROM magma_agent_ledger WHERE agent_id=$3 "
                    "          ORDER BY created_at DESC LIMIT 1),0) + $4, $5,$6,'Voucher commission'",
                    newId(), businessId, agentId, commission,
                    "VC-" + voucherId.substr(0, 8), voucherId);
            }

            Json::Value result;
            result["voucherId"] = voucherId;
            result["subscriptionId"] = subscriptionId;
            result["plan"] = voucher["plan_name"].as<std::string>();
            result["zone"] = voucher["zone_code"].isNull() ? "" : voucher["zone_code"].as<std::string>();

            auto login = co_await billing_.loginDevice(businessId, zoneId, subscriptionId,
                                                       dto.mac, dto.ip);
            result["login"] = login.result;
            result["online"] = login.success;
            if (!login.success) {
                result["loginMessage"] = login.message;
            }

            co_return okResponse("Voucher redeemed", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] redeemVoucher error: " << e.what();
            co_return errorResponse("Unable to redeem voucher");
        }
    }

} // namespace data_support::services
