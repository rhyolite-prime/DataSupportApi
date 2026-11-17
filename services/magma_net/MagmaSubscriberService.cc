//
// MagmaNet — subscribers and their client devices.
//

#include "MagmaSubscriberService.h"

#include "MagmaSupport.h"
#include "utils/DbUtils.h"
#include "utils/PhoneNumberFormatter.h"

using namespace drogon;
using namespace data_support::utils;

namespace data_support::services {

    namespace {
        std::string normaliseMsisdn(const std::string &raw) {
            const std::string formatted = PhoneNumberFormatter::formatPhoneNumber(raw);
            return formatted.empty() ? trimCopy(raw) : formatted;
        }
    }

    Task<std::string> MagmaSubscriberService::ensureSubscriber(const std::string &businessId,
                                                               const std::string &msisdn,
                                                               const std::string &fullName,
                                                               const std::string &email,
                                                               const std::string &zoneId) {
        try {
            auto db = app().getDbClient();
            const std::string normalised = normaliseMsisdn(msisdn);

            if (!normalised.empty()) {
                auto existing = co_await db->execSqlCoro(
                    "SELECT id FROM magma_subscribers WHERE business_id=$1 AND msisdn=$2",
                    businessId, normalised);
                if (!existing.empty()) {
                    const std::string id = existing[0]["id"].as<std::string>();
                    co_await db->execSqlCoro(
                        "UPDATE magma_subscribers SET last_seen_at=now(), "
                        "full_name=COALESCE(NULLIF($2,''), full_name), "
                        "email=COALESCE(NULLIF($3,''), email) WHERE id=$1",
                        id, fullName, email);
                    co_return id;
                }
            }

            const std::string id = newId();
            co_await db->execSqlCoro(
                "INSERT INTO magma_subscribers (id, business_id, msisdn, full_name, email, "
                "subscriber_type, auth_type, home_zone_id, first_seen_at) "
                "VALUES ($1,$2,NULLIF($3,''),NULLIF($4,''),NULLIF($5,''),'WALK_IN','HOTSPOT',"
                "NULLIF($6,'')::uuid, now())",
                id, businessId, normalised, fullName, email, zoneId);
            co_return id;
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] ensureSubscriber error: " << e.what();
            co_return "";
        }
    }

    Task<std::string> MagmaSubscriberService::findSubscriberIdByMsisdn(const std::string &businessId,
                                                                       const std::string &msisdn) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT id FROM magma_subscribers WHERE business_id=$1 AND msisdn=$2",
                businessId, normaliseMsisdn(msisdn));
            if (!rows.empty()) {
                co_return rows[0]["id"].as<std::string>();
            }
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] findSubscriberIdByMsisdn error: " << e.what();
        }
        co_return "";
    }

    Task<std::string> MagmaSubscriberService::findSubscriberIdByMac(const std::string &businessId,
                                                                    const std::string &mac) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT subscriber_id FROM magma_client_devices "
                "WHERE business_id=$1 AND mac_address=$2::macaddr LIMIT 1",
                businessId, mac);
            if (!rows.empty()) {
                co_return rows[0]["subscriber_id"].as<std::string>();
            }
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] findSubscriberIdByMac error: " << e.what();
        }
        co_return "";
    }

    Task<> MagmaSubscriberService::bindDevice(const std::string &businessId,
                                              const std::string &subscriberId,
                                              const std::string &mac) {
        if (mac.empty() || subscriberId.empty()) {
            co_return;
        }
        try {
            auto db = app().getDbClient();
            co_await db->execSqlCoro(
                "INSERT INTO magma_client_devices (id, business_id, subscriber_id, mac_address, last_seen_at) "
                "VALUES ($1,$2,$3,$4::macaddr, now()) "
                "ON CONFLICT (business_id, mac_address) DO UPDATE SET "
                "subscriber_id = EXCLUDED.subscriber_id, last_seen_at = now()",
                newId(), businessId, subscriberId, mac);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] bindDevice error: " << e.what();
        }
        co_return;
    }

    Task<bool> MagmaSubscriberService::isDeviceBlocked(const std::string &businessId,
                                                       const std::string &mac) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT is_blocked FROM magma_client_devices WHERE business_id=$1 AND mac_address=$2::macaddr",
                businessId, mac);
            if (!rows.empty()) {
                co_return rows[0]["is_blocked"].as<bool>();
            }
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] isDeviceBlocked error: " << e.what();
        }
        co_return false;
    }

    Task<dto::BaseApiResponse> MagmaSubscriberService::createSubscriber(
        const std::string &businessId, const dto::magma_net::SubscriberDto &dto) {
        if (dto.msisdn.empty()) {
            co_return errorResponse("msisdn is required");
        }
        try {
            auto db = app().getDbClient();
            const std::string id = newId();
            const std::string msisdn = normaliseMsisdn(dto.msisdn);

            std::string ghanaCardCipher;
            std::string ghanaCardMasked;
            if (!dto.ghanaCardNumber.empty()) {
                ghanaCardCipher = magma::encryptSecret(dto.ghanaCardNumber);
                ghanaCardMasked = dto.ghanaCardNumber.size() > 4
                                      ? std::string(dto.ghanaCardNumber.size() - 4, '*') +
                                            dto.ghanaCardNumber.substr(dto.ghanaCardNumber.size() - 4)
                                      : "****";
            }

            auto rows = co_await db->execSqlCoro(
                "INSERT INTO magma_subscribers (id, business_id, msisdn, full_name, email, "
                "ghana_card_ciphertext, ghana_card_masked, subscriber_type, auth_type, home_zone_id, "
                "service_address, gps_lat, gps_lng, consent_at) VALUES "
                "($1,$2,$3,NULLIF($4,''),NULLIF($5,''),NULLIF($6,''),$7,$8,$9,"
                "NULLIF($10,'')::uuid,NULLIF($11,''),$12,$13, CASE WHEN $14 THEN now() ELSE NULL END) "
                "RETURNING id, msisdn, full_name, email, subscriber_type, auth_type, home_zone_id, status, created_at",
                id, businessId, msisdn, dto.fullName, dto.email, ghanaCardCipher, ghanaCardMasked,
                dto.subscriberType, dto.authType, dto.homeZoneId, dto.serviceAddress,
                dto.gpsLat, dto.gpsLng, dto.consent);

            co_return okResponse("Subscriber created", jsonRow(rows[0]));
        } catch (const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << "[MagmaNet] createSubscriber db error: " << e.base().what();
            co_return errorResponse("Unable to create subscriber (msisdn already registered)");
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] createSubscriber error: " << e.what();
            co_return errorResponse("Unable to create subscriber");
        }
    }

    Task<dto::BaseApiResponse> MagmaSubscriberService::updateSubscriber(
        const std::string &businessId, const std::string &subscriberId,
        const dto::magma_net::SubscriberDto &dto) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "UPDATE magma_subscribers SET "
                "full_name=COALESCE(NULLIF($3,''), full_name), "
                "email=COALESCE(NULLIF($4,''), email), "
                "subscriber_type=COALESCE(NULLIF($5,''), subscriber_type), "
                "auth_type=COALESCE(NULLIF($6,''), auth_type), "
                "service_address=COALESCE(NULLIF($7,''), service_address), "
                "status=COALESCE(NULLIF($8,''), status), "
                "consent_at=CASE WHEN $9 THEN COALESCE(consent_at, now()) ELSE consent_at END, "
                "updated_at=now() WHERE id=$1 AND business_id=$2 "
                "RETURNING id, msisdn, full_name, email, subscriber_type, auth_type, status",
                subscriberId, businessId, dto.fullName, dto.email, dto.subscriberType,
                dto.authType, dto.serviceAddress, dto.status, dto.consent);
            if (rows.empty()) {
                co_return errorResponse("Subscriber not found");
            }
            co_return okResponse("Subscriber updated", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] updateSubscriber error: " << e.what();
            co_return errorResponse("Unable to update subscriber");
        }
    }

    Task<dto::BaseApiResponse> MagmaSubscriberService::getSubscriber(const std::string &businessId,
                                                                     const std::string &subscriberId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT id, msisdn, full_name, email, ghana_card_masked, subscriber_type, auth_type, "
                "home_zone_id, service_address, status, kyc_status, consent_at, created_at "
                "FROM magma_subscribers WHERE id=$1 AND business_id=$2", subscriberId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Subscriber not found");
            }
            Json::Value subscriber = jsonRow(rows[0]);

            auto devices = co_await db->execSqlCoro(
                "SELECT id, mac_address, label, first_seen_at, last_seen_at, is_blocked "
                "FROM magma_client_devices WHERE subscriber_id=$1 ORDER BY last_seen_at DESC NULLS LAST",
                subscriberId);
            subscriber["devices"] = rowsToJson(devices);

            auto subscriptions = co_await db->execSqlCoro(
                "SELECT s.id, s.radius_username, s.status, s.purchased_minutes, s.purchased_mb, "
                "s.used_seconds, s.used_mb, s.starts_at, s.expires_at, p.name AS plan_name, "
                "p.plan_type, z.zone_code "
                "FROM magma_subscriptions s JOIN magma_data_plans p ON p.id=s.plan_id "
                "JOIN magma_zones z ON z.id=s.zone_id "
                "WHERE s.subscriber_id=$1 ORDER BY s.created_at DESC LIMIT 20", subscriberId);
            subscriber["subscriptions"] = rowsToJson(subscriptions);

            co_return okResponse("Subscriber", subscriber);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getSubscriber error: " << e.what();
            co_return errorResponse("Unable to load subscriber");
        }
    }

    Task<dto::BaseApiResponse> MagmaSubscriberService::listSubscribers(const std::string &businessId,
                                                                       int pageNo, int pageSize,
                                                                       const std::string &query,
                                                                       const std::string &status) {
        try {
            auto db = app().getDbClient();
            if (pageNo < 1) pageNo = 1;
            if (pageSize < 1 || pageSize > 200) pageSize = 50;
            const int offset = (pageNo - 1) * pageSize;
            const std::string pattern = "%" + lowerCopy(query) + "%";

            auto rows = co_await db->execSqlCoro(
                "SELECT s.id, s.msisdn, s.full_name, s.email, s.subscriber_type, s.auth_type, "
                "s.status, s.created_at, "
                "(SELECT COUNT(*) FROM magma_subscriptions x WHERE x.subscriber_id=s.id AND x.status='ACTIVE') AS active_subscriptions "
                "FROM magma_subscribers s WHERE s.business_id=$1 "
                "AND ($2='' OR s.status=$2) "
                "AND ($3='' OR lower(COALESCE(s.msisdn,'')) LIKE $4 OR lower(COALESCE(s.full_name,'')) LIKE $4) "
                "ORDER BY s.created_at DESC LIMIT $5 OFFSET $6",
                businessId, status, query, pattern, pageSize, offset);

            auto total = co_await db->execSqlCoro(
                "SELECT COUNT(*) AS count FROM magma_subscribers WHERE business_id=$1 "
                "AND ($2='' OR status=$2) "
                "AND ($3='' OR lower(COALESCE(msisdn,'')) LIKE $4 OR lower(COALESCE(full_name,'')) LIKE $4)",
                businessId, status, query, pattern);

            Json::Value result;
            result["subscribers"] = rowsToJson(rows);
            result["pageNo"] = pageNo;
            result["pageSize"] = pageSize;
            result["total"] = total.empty() ? 0 : total[0]["count"].as<int64_t>();
            co_return okResponse("Subscribers", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] listSubscribers error: " << e.what();
            co_return errorResponse("Unable to list subscribers");
        }
    }

    Task<dto::BaseApiResponse> MagmaSubscriberService::listClientDevices(
        const std::string &businessId, const std::string &subscriberId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT d.id, d.mac_address, d.label, d.first_seen_at, d.last_seen_at, d.is_blocked, "
                "d.blocked_reason FROM magma_client_devices d "
                "JOIN magma_subscribers s ON s.id=d.subscriber_id "
                "WHERE d.subscriber_id=$1 AND s.business_id=$2 ORDER BY d.last_seen_at DESC NULLS LAST",
                subscriberId, businessId);
            co_return okResponse("Client devices", rowsToJson(rows));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] listClientDevices error: " << e.what();
            co_return errorResponse("Unable to list devices");
        }
    }

    Task<dto::BaseApiResponse> MagmaSubscriberService::blockClientDevice(
        const std::string &businessId, const std::string &mac, const std::string &reason) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "UPDATE magma_client_devices SET is_blocked=true, blocked_reason=$3 "
                "WHERE business_id=$1 AND mac_address=$2::macaddr "
                "RETURNING id, mac_address, is_blocked", businessId, mac, reason);
            if (rows.empty()) {
                co_return errorResponse("Device not found");
            }
            // Existing sessions are terminated by the usage-enforcement job via CoA.
            co_return okResponse("Device blocked", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] blockClientDevice error: " << e.what();
            co_return errorResponse("Unable to block device");
        }
    }

    Task<dto::BaseApiResponse> MagmaSubscriberService::unblockClientDevice(
        const std::string &businessId, const std::string &mac) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "UPDATE magma_client_devices SET is_blocked=false, blocked_reason=NULL "
                "WHERE business_id=$1 AND mac_address=$2::macaddr "
                "RETURNING id, mac_address, is_blocked", businessId, mac);
            if (rows.empty()) {
                co_return errorResponse("Device not found");
            }
            co_return okResponse("Device unblocked", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] unblockClientDevice error: " << e.what();
            co_return errorResponse("Unable to unblock device");
        }
    }

    Task<dto::BaseApiResponse> MagmaSubscriberService::getSubscriberUsage(
        const std::string &businessId, const std::string &subscriberId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT s.id, s.radius_username, s.status, s.purchased_mb, s.used_mb, "
                "s.purchased_minutes, s.used_seconds, s.starts_at, s.expires_at, p.name AS plan_name "
                "FROM magma_subscriptions s JOIN magma_data_plans p ON p.id=s.plan_id "
                "WHERE s.subscriber_id=$1 AND s.business_id=$2 "
                "ORDER BY s.created_at DESC LIMIT 10", subscriberId, businessId);
            co_return okResponse("Subscriber usage", rowsToJson(rows));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getSubscriberUsage error: " << e.what();
            co_return errorResponse("Unable to load usage");
        }
    }

    Task<dto::BaseApiResponse> MagmaSubscriberService::getSubscriberSessions(
        const std::string &businessId, const std::string &subscriberId, int pageNo, int pageSize) {
        try {
            auto db = app().getDbClient();
            if (pageNo < 1) pageNo = 1;
            if (pageSize < 1 || pageSize > 200) pageSize = 50;
            auto rows = co_await db->execSqlCoro(
                "SELECT s.acct_session_id, s.mac_address, s.framed_ip, s.started_at, s.ended_at, "
                "s.input_octets, s.output_octets, s.session_seconds, s.terminate_cause, z.zone_code "
                "FROM magma_sessions s JOIN magma_zones z ON z.id=s.zone_id "
                "WHERE s.subscriber_id=$1 AND s.business_id=$2 "
                "ORDER BY s.started_at DESC LIMIT $3 OFFSET $4",
                subscriberId, businessId, pageSize, (pageNo - 1) * pageSize);
            co_return okResponse("Subscriber sessions", rowsToJson(rows));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getSubscriberSessions error: " << e.what();
            co_return errorResponse("Unable to load sessions");
        }
    }

    Task<dto::BaseApiResponse> MagmaSubscriberService::suspendSubscriber(
        const std::string &businessId, const std::string &subscriberId, const std::string &reason) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "UPDATE magma_subscribers SET status='SUSPENDED', updated_at=now() "
                "WHERE id=$1 AND business_id=$2 RETURNING id, msisdn, status", subscriberId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Subscriber not found");
            }
            co_await db->execSqlCoro(
                "UPDATE magma_subscriptions SET status='SUSPENDED', updated_at=now() "
                "WHERE subscriber_id=$1 AND status='ACTIVE'", subscriberId);
            // Drop RADIUS credentials immediately; live sessions end at their
            // Session-Timeout (<= 24 h) unless an admin also forces a disconnect.
            const std::string schema = MagmaSettings::instance().radiusSchema;
            co_await db->execSqlCoro(
                "DELETE FROM " + schema + ".radcheck WHERE username IN "
                "(SELECT radius_username FROM magma_subscriptions WHERE subscriber_id=$1)",
                subscriberId);
            co_await magma::auditLog(businessId, "STAFF", "", "SUBSCRIBER_SUSPENDED",
                                     "magma_subscribers", subscriberId,
                                     Json::Value(reason));
            Json::Value result = jsonRow(rows[0]);
            result["reason"] = reason;
            co_return okResponse("Subscriber suspended", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] suspendSubscriber error: " << e.what();
            co_return errorResponse("Unable to suspend subscriber");
        }
    }

    Task<dto::BaseApiResponse> MagmaSubscriberService::resumeSubscriber(
        const std::string &businessId, const std::string &subscriberId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "UPDATE magma_subscribers SET status='ACTIVE', updated_at=now() "
                "WHERE id=$1 AND business_id=$2 RETURNING id, msisdn, status", subscriberId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Subscriber not found");
            }
            co_await db->execSqlCoro(
                "UPDATE magma_subscriptions SET status='ACTIVE', updated_at=now() "
                "WHERE subscriber_id=$1 AND status='SUSPENDED' AND expires_at > now()", subscriberId);
            co_return okResponse("Subscriber resumed", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] resumeSubscriber error: " << e.what();
            co_return errorResponse("Unable to resume subscriber");
        }
    }

    Task<dto::BaseApiResponse> MagmaSubscriberService::churnSubscriber(
        const std::string &businessId, const std::string &subscriberId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "UPDATE magma_subscribers SET status='CHURNED', updated_at=now() "
                "WHERE id=$1 AND business_id=$2 RETURNING id, msisdn, status", subscriberId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Subscriber not found");
            }
            co_await db->execSqlCoro(
                "UPDATE magma_subscriptions SET status='CANCELLED', ended_at=now(), updated_at=now() "
                "WHERE subscriber_id=$1 AND status IN ('ACTIVE','SUSPENDED')", subscriberId);
            co_return okResponse("Subscriber churned", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] churnSubscriber error: " << e.what();
            co_return errorResponse("Unable to churn subscriber");
        }
    }

} // namespace data_support::services
