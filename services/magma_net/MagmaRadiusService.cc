//
// MagmaNet — RADIUS projection, accounting ingestion and enforcement.
//

#include "MagmaRadiusService.h"

#include <regex>

#include "MagmaSettings.h"
#include "MagmaSupport.h"
#include "utils/DbUtils.h"
#include "utils/RadiusClient.h"

using namespace drogon;
using namespace data_support::utils;

namespace data_support::services {

    namespace {
        std::string radiusSchema() {
            const std::string &configured = MagmaSettings::instance().radiusSchema;
            static const std::regex valid("^[a-zA-Z_][a-zA-Z0-9_]*$");
            if (std::regex_match(configured, valid)) {
                return configured;
            }
            return "radius";
        }

        std::string rateLimitExpression(int downKbps, int upKbps) {
            // RouterOS rate-limit is rx/tx from the router's point of view:
            // rx = client upload, tx = client download.
            return std::to_string(upKbps) + "k/" + std::to_string(downKbps) + "k";
        }

        struct CoaTarget {
            std::string nasIp;
            std::string secret;
            std::string sessionId;
            std::string username;
        };

        drogon::Task<std::vector<CoaTarget>> coaTargetsFor(const std::string &username) {
            std::vector<CoaTarget> targets;
            try {
                auto db = app().getDbClient();
                auto rows = co_await db->execSqlCoro(
                    "SELECT r.acctsessionid, host(r.nasipaddress) AS nas_ip, d.radius_secret_ciphertext "
                    "FROM " + radiusSchema() + ".radacct r "
                    "LEFT JOIN magma_devices d ON host(d.tunnel_ip) = host(r.nasipaddress) "
                    "WHERE r.username=$1 AND r.acctstoptime IS NULL",
                    username);
                for (const auto &row : rows) {
                    CoaTarget target;
                    target.sessionId = row["acctsessionid"].as<std::string>();
                    target.nasIp = row["nas_ip"].as<std::string>();
                    target.username = username;
                    if (!row["radius_secret_ciphertext"].isNull()) {
                        target.secret = magma::decryptSecret(
                            row["radius_secret_ciphertext"].as<std::string>());
                    }
                    if (!target.nasIp.empty() && !target.secret.empty()) {
                        targets.push_back(target);
                    }
                }
            } catch (const std::exception &e) {
                LOG_ERROR << "[MagmaNet] coaTargetsFor error: " << e.what();
            }
            co_return targets;
        }

        void disconnectAsync(const std::vector<CoaTarget> &targets) {
            if (targets.empty()) {
                return;
            }
            const int port = MagmaSettings::instance().radiusCoaPort;
            RadiusClient::fireAndForget([targets, port]() {
                RadiusClient client;
                for (const auto &target : targets) {
                    const bool sent = client.disconnect(target.nasIp, port, target.secret,
                                                        target.sessionId, target.username);
                    if (!sent) {
                        LOG_WARN << "[MagmaNet] CoA disconnect not acknowledged for session "
                                 << target.sessionId << " on " << target.nasIp;
                    }
                }
            });
        }
    }

    Task<dto::BaseApiResponse> MagmaRadiusService::projectSubscription(
        const std::string &businessId, const std::string &subscriptionId) {
        const std::string schema = radiusSchema();

        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT s.id, s.business_id, s.status, s.radius_username, s.radius_secret_ct, "
                "s.purchased_minutes, s.purchased_mb, s.used_seconds, s.used_mb, s.expires_at, "
                "s.subscriber_id, s.zone_id, s.throttled_at, "
                "p.plan_code, p.plan_type, p.down_kbps, p.up_kbps, p.device_limit, "
                "p.fair_use_cap_mb, p.throttle_down_kbps, p.throttle_up_kbps, "
                "GREATEST(0, (s.purchased_minutes*60 - s.used_seconds)) AS remaining_seconds, "
                "GREATEST(0, (s.purchased_mb*1048576 - s.used_mb*1048576)) AS remaining_bytes, "
                "GREATEST(0, EXTRACT(EPOCH FROM (s.expires_at - now()))::bigint) AS seconds_to_expiry, "
                "(SELECT mac_address FROM magma_client_devices cd WHERE cd.subscriber_id=s.subscriber_id "
                " ORDER BY cd.last_seen_at DESC NULLS LAST LIMIT 1) AS bound_mac "
                "FROM magma_subscriptions s JOIN magma_data_plans p ON p.id=s.plan_id "
                "WHERE s.id=$1 AND s.business_id=$2", subscriptionId, businessId);

            if (rows.empty()) {
                co_return errorResponse("Subscription not found");
            }

            const auto &row = rows[0];
            const std::string username = row["radius_username"].as<std::string>();
            const std::string status = row["status"].as<std::string>();

            // Clear previous attributes first — projection must be idempotent.
            co_await db->execSqlCoro("DELETE FROM " + schema + ".radcheck WHERE username=$1", username);
            co_await db->execSqlCoro("DELETE FROM " + schema + ".radreply WHERE username=$1", username);
            co_await db->execSqlCoro("DELETE FROM " + schema + ".radusergroup WHERE username=$1", username);

            if (status != "ACTIVE") {
                co_return okResponse("Subscription is not active; credentials removed");
            }

            const std::string secret = magma::decryptSecret(row["radius_secret_ct"].as<std::string>());
            if (secret.empty()) {
                co_return errorResponse("Unable to decrypt RADIUS secret — check MAGMA_SECRETS_KEY");
            }

            const std::string planCode = row["plan_code"].as<std::string>();
            const std::string planType = row["plan_type"].as<std::string>();
            const std::string groupName = "magma-" + planCode;

            int downKbps = row["down_kbps"].as<int>();
            int upKbps = row["up_kbps"].as<int>();
            const bool throttled = !row["throttled_at"].isNull();
            const int throttleDown = row["throttle_down_kbps"].isNull()
                                         ? 0 : row["throttle_down_kbps"].as<int>();
            const int throttleUp = row["throttle_up_kbps"].isNull()
                                       ? 0 : row["throttle_up_kbps"].as<int>();

            if (planType == "UNLIMITED" && throttled && throttleDown > 0) {
                downKbps = throttleDown;
                upKbps = throttleUp > 0 ? throttleUp : downKbps;
            }

            const int64_t remainingSeconds = row["remaining_seconds"].as<int64_t>();
            const int64_t secondsToExpiry = row["seconds_to_expiry"].as<int64_t>();
            const int64_t remainingBytes = row["remaining_bytes"].as<int64_t>();
            const int deviceLimit = row["device_limit"].as<int>();

            int64_t sessionTimeout = 86400;
            if (planType == "TIME" && remainingSeconds > 0) {
                sessionTimeout = remainingSeconds;
            } else if (secondsToExpiry > 0) {
                sessionTimeout = secondsToExpiry;
            }
            if (sessionTimeout > 86400) sessionTimeout = 86400;
            if (sessionTimeout < 60) sessionTimeout = 60;

            co_await db->execSqlCoro(
                "INSERT INTO " + schema + ".radcheck (username, attribute, op, value) "
                "VALUES ($1,'Cleartext-Password',':=',$2)",
                username, secret);

            if (deviceLimit > 1) {
                co_await db->execSqlCoro(
                    "INSERT INTO " + schema + ".radcheck (username, attribute, op, value) "
                    "VALUES ($1,'Simultaneous-Use',':=',$2)",
                    username, std::to_string(deviceLimit));
            } else if (!row["bound_mac"].isNull()) {
                co_await db->execSqlCoro(
                    "INSERT INTO " + schema + ".radcheck (username, attribute, op, value) "
                    "VALUES ($1,'Calling-Station-Id','==',$2)",
                    username, row["bound_mac"].as<std::string>());
            }

            auto addReply = [&db, &schema, &username](const std::string &attribute,
                                                       const std::string &value) -> drogon::Task<> {
                co_await db->execSqlCoro(
                    "INSERT INTO " + schema + ".radreply (username, attribute, op, value) "
                    "VALUES ($1,$2,'=',$3)", username, attribute, value);
                co_return;
            };

            co_await addReply("Mikrotik-Rate-Limit", rateLimitExpression(downKbps, upKbps));
            co_await addReply("Session-Timeout", std::to_string(sessionTimeout));
            co_await addReply("Idle-Timeout", "1800");
            co_await addReply("Acct-Interim-Interval", "60");
            // NOTE: do not send Mikrotik-Group here — RouterOS treats it as a
            // hotspot user-profile name and rejects logins when the profile
            // does not exist locally. Rate/total limits travel as VSAs instead.

            if (planType == "VOLUME" && remainingBytes > 0) {
                const int64_t gigawords = remainingBytes / 4294967296LL;
                const int64_t low = remainingBytes % 4294967296LL;
                co_await addReply("Mikrotik-Total-Limit", std::to_string(low));
                co_await addReply("Mikrotik-Total-Limit-Gigawords", std::to_string(gigawords));
            }

            co_await db->execSqlCoro(
                "INSERT INTO " + schema + ".radusergroup (username, groupname, priority) "
                "VALUES ($1,$2,1)", username, groupName);

            Json::Value result;
            result["username"] = username;
            result["rate_limit"] = rateLimitExpression(downKbps, upKbps);
            result["session_timeout"] = sessionTimeout;
            result["remaining_bytes"] = remainingBytes;
            result["plan_type"] = planType;
            co_return okResponse("RADIUS credentials projected", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] projectSubscription error: " << e.what();
            co_return errorResponse("Unable to project subscription into RADIUS");
        }
    }

    Task<dto::BaseApiResponse> MagmaRadiusService::removeSubscription(
        const std::string &businessId, const std::string &subscriptionId) {
        const std::string schema = radiusSchema();
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT radius_username FROM magma_subscriptions WHERE id=$1 AND business_id=$2",
                subscriptionId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Subscription not found");
            }
            const std::string username = rows[0]["radius_username"].as<std::string>();

            co_await db->execSqlCoro("DELETE FROM " + schema + ".radcheck WHERE username=$1", username);
            co_await db->execSqlCoro("DELETE FROM " + schema + ".radreply WHERE username=$1", username);
            co_await db->execSqlCoro("DELETE FROM " + schema + ".radusergroup WHERE username=$1", username);

            const auto targets = co_await coaTargetsFor(username);
            disconnectAsync(targets);

            auto result = Json::Value(Json::objectValue);
            result["username"] = username;
            result["sessions_disconnected"] = static_cast<int>(targets.size());
            co_return okResponse("Credentials removed", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] removeSubscription error: " << e.what();
            co_return errorResponse("Unable to remove RADIUS credentials");
        }
    }

    Task<dto::BaseApiResponse> MagmaRadiusService::syncUsage() {
        const std::string schema = radiusSchema();
        try {
            auto db = app().getDbClient();

            // Insert newly seen sessions. `ON CONFLICT DO NOTHING` (no target)
            // works both on the plain table and after 0004 partitioning, where
            // the unique index is (acct_session_id, started_at).
            auto inserted = co_await db->execSqlCoro(
                "INSERT INTO magma_sessions (id, business_id, subscription_id, subscriber_id, zone_id, "
                "acct_session_id, nas_ip, framed_ip, mac_address, started_at, last_interim_at, ended_at, "
                "input_octets, output_octets, session_seconds, terminate_cause) "
                "SELECT gen_random_uuid(), s.business_id, s.id, s.subscriber_id, s.zone_id, "
                "r.acctsessionid, r.nasipaddress, r.framedipaddress, "
                "CASE WHEN r.callingstationid ~ '^([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}$' "
                "     THEN r.callingstationid::macaddr ELSE NULL END, "
                "COALESCE(r.acctstarttime, now()), r.acctupdatetime, r.acctstoptime, "
                "COALESCE(r.acctinputoctets,0), COALESCE(r.acctoutputoctets,0), "
                "COALESCE(r.acctsessiontime,0), r.acctterminatecause "
                "FROM " + schema + ".radacct r "
                "JOIN magma_subscriptions s ON s.radius_username = r.username "
                "WHERE r.acctstoptime IS NULL OR r.acctupdatetime > now() - interval '1 day' "
                "ON CONFLICT DO NOTHING "
                "RETURNING id");

            // Refresh the sessions that already exist (interim updates and stop records).
            co_await db->execSqlCoro(
                "UPDATE magma_sessions ms SET "
                "input_octets = COALESCE(r.acctinputoctets,0), "
                "output_octets = COALESCE(r.acctoutputoctets,0), "
                "session_seconds = COALESCE(r.acctsessiontime,0), "
                "last_interim_at = r.acctupdatetime, "
                "ended_at = r.acctstoptime, "
                "terminate_cause = r.acctterminatecause, "
                "framed_ip = r.framedipaddress, "
                "nas_ip = r.nasipaddress "
                "FROM " + schema + ".radacct r "
                "WHERE ms.acct_session_id = r.acctsessionid "
                "AND r.acctupdatetime > now() - interval '1 day'");

            auto updated = co_await db->execSqlCoro(
                "WITH usage AS ("
                "  SELECT subscription_id, "
                "         COALESCE(SUM(input_octets + output_octets),0)::bigint AS total_bytes, "
                "         COALESCE(SUM(session_seconds),0)::bigint AS total_seconds "
                "  FROM magma_sessions GROUP BY subscription_id"
                ") "
                "UPDATE magma_subscriptions s SET "
                "used_mb = (usage.total_bytes / 1048576)::bigint, "
                "used_seconds = usage.total_seconds, updated_at = now() "
                "FROM usage WHERE usage.subscription_id = s.id AND s.status='ACTIVE' "
                "RETURNING s.id");

            Json::Value result;
            result["sessions_synced"] = static_cast<int64_t>(inserted.size());
            result["subscriptions_updated"] = static_cast<int64_t>(updated.size());
            co_return okResponse("Usage synchronised", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] syncUsage error: " << e.what();
            co_return errorResponse("Unable to synchronise usage");
        }
    }

    Task<dto::BaseApiResponse> MagmaRadiusService::enforceLimits() {
        try {
            auto db = app().getDbClient();
            const std::string schema = radiusSchema();

            // 1. Expire / deplete plans whose allowance is used up.
            auto offenders = co_await db->execSqlCoro(
                "SELECT s.id, s.radius_username, p.plan_type, "
                "COALESCE((SELECT SUM(ms.input_octets + ms.output_octets) FROM magma_sessions ms "
                "          WHERE ms.subscription_id=s.id),0)::bigint AS used_bytes, "
                "COALESCE((SELECT SUM(ms.session_seconds) FROM magma_sessions ms "
                "          WHERE ms.subscription_id=s.id),0)::bigint AS used_seconds, "
                "COALESCE(s.purchased_mb,0) AS purchased_mb, "
                "COALESCE(s.purchased_minutes,0) AS purchased_minutes "
                "FROM magma_subscriptions s JOIN magma_data_plans p ON p.id=s.plan_id "
                "WHERE s.status='ACTIVE' AND ("
                "  s.expires_at <= now() "
                "  OR (p.plan_type='VOLUME' AND s.purchased_mb > 0 "
                "      AND COALESCE((SELECT SUM(ms.input_octets + ms.output_octets) FROM magma_sessions ms "
                "                    WHERE ms.subscription_id=s.id),0) >= s.purchased_mb * 1048576) "
                "  OR (p.plan_type='TIME' AND s.purchased_minutes > 0 "
                "      AND COALESCE((SELECT SUM(ms.session_seconds) FROM magma_sessions ms "
                "                    WHERE ms.subscription_id=s.id),0) >= s.purchased_minutes * 60)"
                ") LIMIT 500");

            int expiredCount = 0;
            int depletedCount = 0;

            for (const auto &row : offenders) {
                const std::string id = row["id"].as<std::string>();
                const std::string username = row["radius_username"].as<std::string>();
                const std::string planType = row["plan_type"].as<std::string>();
                const int64_t usedBytes = row["used_bytes"].as<int64_t>();
                const int64_t usedSeconds = row["used_seconds"].as<int64_t>();
                const int64_t purchasedMb = row["purchased_mb"].as<int64_t>();
                const int64_t purchasedMinutes = row["purchased_minutes"].as<int64_t>();

                const bool isDepleted =
                    (planType == "VOLUME" && purchasedMb > 0 && usedBytes >= purchasedMb * 1048576) ||
                    (planType == "TIME" && purchasedMinutes > 0 &&
                     usedSeconds >= purchasedMinutes * 60);

                const std::string newStatus = isDepleted ? "DEPLETED" : "EXPIRED";
                if (isDepleted) {
                    ++depletedCount;
                } else {
                    ++expiredCount;
                }

                co_await db->execSqlCoro(
                    "UPDATE magma_subscriptions SET status=$2, ended_at=now(), updated_at=now() "
                    "WHERE id=$1 AND status='ACTIVE'", id, newStatus);
                co_await db->execSqlCoro(
                    "DELETE FROM " + schema + ".radcheck WHERE username=$1", username);

                const auto targets = co_await coaTargetsFor(username);
                disconnectAsync(targets);

                co_await magma::enqueueOutbox("SUBSCRIPTION_ENDED", id,
                                              Json::Value(username));
            }

            // 2. Apply fair-use throttles to unlimited plans (once per subscription).
            auto toThrottle = co_await db->execSqlCoro(
                "SELECT s.id, s.radius_username, p.throttle_down_kbps, p.throttle_up_kbps "
                "FROM magma_subscriptions s JOIN magma_data_plans p ON p.id=s.plan_id "
                "WHERE s.status='ACTIVE' AND p.plan_type='UNLIMITED' "
                "AND p.fair_use_cap_mb > 0 AND p.throttle_down_kbps > 0 "
                "AND s.throttled_at IS NULL "
                "AND s.used_mb >= p.fair_use_cap_mb LIMIT 500");

            int throttled = 0;
            for (const auto &row : toThrottle) {
                const std::string id = row["id"].as<std::string>();
                const std::string username = row["radius_username"].as<std::string>();
                const std::string rate = rateLimitExpression(row["throttle_down_kbps"].as<int>(),
                                                             row["throttle_up_kbps"].isNull()
                                                                 ? 0
                                                                 : row["throttle_up_kbps"].as<int>());

                co_await db->execSqlCoro(
                    "UPDATE magma_subscriptions SET throttled_at=now(), updated_at=now() "
                    "WHERE id=$1 AND throttled_at IS NULL", id);
                co_await db->execSqlCoro(
                    "DELETE FROM " + schema + ".radreply WHERE username=$1 "
                    "AND attribute='Mikrotik-Rate-Limit'", username);
                co_await db->execSqlCoro(
                    "INSERT INTO " + schema + ".radreply (username, attribute, op, value) "
                    "VALUES ($1,'Mikrotik-Rate-Limit','=',$2)", username, rate);

                const auto targets = co_await coaTargetsFor(username);
                if (!targets.empty()) {
                    RadiusClient::fireAndForget([targets, rate]() {
                        RadiusClient client;
                        for (const auto &target : targets) {
                            client.changeRateLimit(target.nasIp, 3799, target.secret,
                                                   target.sessionId, target.username, rate);
                        }
                    });
                }
                ++throttled;
            }

            Json::Value result;
            result["expired"] = expiredCount;
            result["depleted"] = depletedCount;
            result["throttled"] = throttled;
            co_return okResponse("Limits enforced", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] enforceLimits error: " << e.what();
            co_return errorResponse("Unable to enforce limits");
        }
    }

    Task<int> MagmaRadiusService::disconnectSubscription(const std::string &businessId,
                                                         const std::string &subscriptionId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT radius_username FROM magma_subscriptions WHERE id=$1 AND business_id=$2",
                subscriptionId, businessId);
            if (rows.empty()) {
                co_return 0;
            }
            const auto targets = co_await coaTargetsFor(rows[0]["radius_username"].as<std::string>());
            disconnectAsync(targets);
            co_return static_cast<int>(targets.size());
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] disconnectSubscription error: " << e.what();
            co_return 0;
        }
    }

    Task<dto::BaseApiResponse> MagmaRadiusService::syncNas(const std::string &deviceId) {
        const std::string schema = radiusSchema();
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT d.id, host(d.tunnel_ip) AS tunnel_ip, d.radius_secret_ciphertext, z.zone_code "
                "FROM magma_devices d LEFT JOIN magma_zones z ON z.id=d.zone_id "
                "WHERE d.id=$1 AND d.role='GATEWAY'", deviceId);
            if (rows.empty()) {
                co_return errorResponse("Gateway device not found or not a GATEWAY role");
            }
            if (rows[0]["tunnel_ip"].isNull()) {
                co_return errorResponse("Gateway has no tunnel IP");
            }

            const std::string nasIp = rows[0]["tunnel_ip"].as<std::string>();
            const std::string shortname = rows[0]["zone_code"].isNull()
                                              ? "magma"
                                              : rows[0]["zone_code"].as<std::string>();
            const std::string secret = magma::decryptSecret(
                rows[0]["radius_secret_ciphertext"].isNull()
                    ? ""
                    : rows[0]["radius_secret_ciphertext"].as<std::string>());
            if (secret.empty()) {
                co_return errorResponse("Gateway RADIUS secret missing");
            }

            auto existing = co_await db->execSqlCoro(
                "SELECT id FROM " + schema + ".nas WHERE nasname=$1", nasIp);
            if (existing.empty()) {
                co_await db->execSqlCoro(
                    "INSERT INTO " + schema + ".nas (nasname, shortname, type, ports, secret, description) "
                    "VALUES ($1,$2,'mikrotik',3799,$3,'MagmaNet zone gateway')",
                    nasIp, shortname, secret);
            } else {
                co_await db->execSqlCoro(
                    "UPDATE " + schema + ".nas SET shortname=$2, secret=$3, type='mikrotik' WHERE nasname=$1",
                    nasIp, shortname, secret);
            }

            Json::Value result;
            result["nas_ip"] = nasIp;
            result["shortname"] = shortname;
            co_return okResponse("NAS synchronised", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] syncNas error: " << e.what();
            co_return errorResponse("Unable to synchronise NAS");
        }
    }

    Task<dto::BaseApiResponse> MagmaRadiusService::getActiveSessions(const std::string &businessId,
                                                                     const std::string &zoneId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT ms.id, ms.acct_session_id, ms.mac_address, host(ms.framed_ip) AS framed_ip, "
                "host(ms.nas_ip) AS nas_ip, ms.started_at, ms.last_interim_at, "
                "(ms.input_octets + ms.output_octets) AS bytes_total, ms.session_seconds, "
                "s.radius_username, s.id AS subscription_id, s.expires_at, "
                "sub.msisdn, z.zone_code, p.name AS plan_name "
                "FROM magma_sessions ms "
                "JOIN magma_subscriptions s ON s.id=ms.subscription_id "
                "JOIN magma_subscribers sub ON sub.id=ms.subscriber_id "
                "JOIN magma_zones z ON z.id=ms.zone_id "
                "JOIN magma_data_plans p ON p.id=s.plan_id "
                "WHERE ms.business_id=$1 AND ms.ended_at IS NULL AND ($2='' OR ms.zone_id::text=$2) "
                "ORDER BY ms.started_at DESC LIMIT 1000",
                businessId, zoneId);
            co_return okResponse("Active sessions", rowsToJson(rows));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getActiveSessions error: " << e.what();
            co_return errorResponse("Unable to list sessions");
        }
    }

    Task<dto::BaseApiResponse> MagmaRadiusService::getAccountingSummary(
        const std::string &businessId, const std::string &from, const std::string &to) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT date_trunc('day', ms.started_at)::date AS day, COUNT(*) AS sessions, "
                "COALESCE(SUM(ms.input_octets + ms.output_octets),0)::bigint AS bytes_total, "
                "COALESCE(SUM(ms.session_seconds),0)::bigint AS seconds_total "
                "FROM magma_sessions ms WHERE ms.business_id=$1 "
                "AND ms.started_at >= COALESCE(NULLIF($2,'')::timestamptz, now() - interval '30 days') "
                "AND ms.started_at <= COALESCE(NULLIF($3,'')::timestamptz, now()) "
                "GROUP BY 1 ORDER BY 1 DESC", businessId, from, to);
            co_return okResponse("Accounting summary", rowsToJson(rows));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getAccountingSummary error: " << e.what();
            co_return errorResponse("Unable to build accounting summary");
        }
    }

} // namespace data_support::services
