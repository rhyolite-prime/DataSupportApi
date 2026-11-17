//
// MagmaNet — helpers shared by every MagmaNet service:
// secret encryption, tenant config, outbox, audit, rate limiting.
//

#ifndef DATASUPPORTAPI_MAGMASUPPORT_H
#define DATASUPPORTAPI_MAGMASUPPORT_H

#include <drogon/drogon.h>
#include <json/json.h>

#include <string>

#include "MagmaSettings.h"
#include "utils/CryptoUtils.h"
#include "utils/DbUtils.h"

namespace data_support::services::magma {

    inline std::string encryptSecret(const std::string &plaintext) {
        try {
            return utils::CryptoUtils::encrypt(plaintext, MagmaSettings::instance().secretsKey);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] secret encryption failed: " << e.what();
            return "";
        }
    }

    inline std::string decryptSecret(const std::string &ciphertext) {
        if (ciphertext.empty()) {
            return "";
        }
        try {
            return utils::CryptoUtils::decrypt(ciphertext, MagmaSettings::instance().secretsKey);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] secret decryption failed: " << e.what();
            return "";
        }
    }

    inline drogon::Task<Json::Value> businessConfig(const std::string &businessId) {
        Json::Value config(Json::objectValue);
        try {
            auto db = drogon::app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT configuration_data FROM business_accounts WHERE id = $1", businessId);
            if (rows.empty()) {
                co_return config;
            }
            const std::string raw = rows[0]["configuration_data"].as<std::string>();
            bool parsed = false;
            auto value = utils::parseJson(raw, &parsed);
            if (parsed) {
                config = value;
            }
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] businessConfig failed: " << e.what();
        }
        co_return config;
    }

    inline drogon::Task<> enqueueOutbox(const std::string &eventType,
                                        const std::string &aggregateId,
                                        const Json::Value &payload) {
        try {
            auto db = drogon::app().getDbClient();
            co_await db->execSqlCoro(
                "INSERT INTO magma_outbox (event_type, aggregate_id, payload) "
                "VALUES ($1, NULLIF($2,'')::uuid, $3)",
                eventType, aggregateId, utils::toJsonString(payload));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] outbox enqueue failed (" << eventType << "): " << e.what();
        }
        co_return;
    }

    inline drogon::Task<> auditLog(const std::string &businessId,
                                   const std::string &actorType,
                                   const std::string &actorId,
                                   const std::string &action,
                                   const std::string &entity,
                                   const std::string &entityId,
                                   const Json::Value &after) {
        try {
            auto db = drogon::app().getDbClient();
            co_await db->execSqlCoro(
                "INSERT INTO magma_audit_log (business_id, actor_type, actor_id, action, entity, "
                "entity_id, after) VALUES "
                "(NULLIF($1,'')::uuid,$2,NULLIF($3,'')::uuid,$4,$5,NULLIF($6,'')::uuid,$7)",
                businessId, actorType, actorId, action, entity, entityId,
                utils::toJsonString(after));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] audit log failed: " << e.what();
        }
        co_return;
    }

    /** Fixed-window rate limit. Fails open when Redis is unavailable. */
    inline drogon::Task<bool> rateLimit(const std::string &key, int limit, int windowSeconds) {
        try {
            auto redis = drogon::app().getRedisClient();
            if (!redis) {
                co_return true;
            }
            const std::string redisKey = "magma:rl:" + key;
            auto count = co_await redis->execCommandCoro("INCR %s", redisKey.c_str());
            const long long value = count.asInteger();
            if (value == 1) {
                co_await redis->execCommandCoro("EXPIRE %s %d", redisKey.c_str(), windowSeconds);
            }
            co_return value <= limit;
        } catch (const std::exception &e) {
            LOG_WARN << "[MagmaNet] rate limit check failed, allowing request: " << e.what();
            co_return true;
        }
    }

    inline drogon::Task<std::string> zoneIdByCode(const std::string &businessId,
                                                  const std::string &zoneCode) {
        try {
            auto db = drogon::app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT id FROM magma_zones WHERE business_id = $1 AND zone_code = $2",
                businessId, zoneCode);
            if (!rows.empty()) {
                co_return rows[0]["id"].as<std::string>();
            }
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] zone lookup failed: " << e.what();
        }
        co_return "";
    }

    inline std::string sqlInterval(int seconds) {
        return std::to_string(seconds) + " seconds";
    }

} // namespace data_support::services::magma

#endif //DATASUPPORTAPI_MAGMASUPPORT_H
