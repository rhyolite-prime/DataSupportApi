//
// MagmaNet — device enrolment, RouterOS configuration and health polling.
//

#include "MagmaDeviceService.h"

#include <memory>

#include "MagmaRadiusService.h"
#include "MagmaSettings.h"
#include "MagmaSupport.h"
#include "utils/DbUtils.h"
#include "utils/HmacUtils.h"
#include "utils/RouterOsClient.h"

using namespace drogon;
using namespace data_support::utils;

namespace data_support::services {

    namespace {
        std::shared_ptr<RouterOsClient> clientForRow(const orm::Row &row) {
            const std::string host = row["tunnel_ip"].isNull() ? "" : row["tunnel_ip"].as<std::string>();
            if (host.empty()) {
                return nullptr;
            }
            const std::string scheme = row["rest_scheme"].isNull() ? "http" : row["rest_scheme"].as<std::string>();
            const int port = row["rest_port"].isNull() ? 80 : row["rest_port"].as<int>();
            const std::string user = row["api_username"].isNull() ? "" : row["api_username"].as<std::string>();
            const std::string secret = magma::decryptSecret(
                row["api_secret_ciphertext"].isNull() ? "" : row["api_secret_ciphertext"].as<std::string>());
            if (user.empty() || secret.empty()) {
                return nullptr;
            }
            return std::make_shared<RouterOsClient>(scheme, host, port, user, secret,
                                                    MagmaSettings::instance().routerOsTimeoutMs / 1000.0);
        }
    }

    MagmaDeviceService::MagmaDeviceService(MagmaRadiusService &radius) : radius_(radius) {}

    Task<dto::BaseApiResponse> MagmaDeviceService::registerDevice(const std::string &businessId,
                                                                  const Json::Value &body) {
        const std::string zoneId = jsonString(body, "zoneId");
        const std::string name = jsonString(body, "name");
        const std::string role = jsonString(body, "role", "AP");
        if (name.empty()) {
            co_return errorResponse("name is required");
        }

        try {
            auto db = app().getDbClient();
            if (!zoneId.empty()) {
                auto zone = co_await db->execSqlCoro(
                    "SELECT id FROM magma_zones WHERE id=$1 AND business_id=$2", zoneId, businessId);
                if (zone.empty()) {
                    co_return errorResponse("Zone not found");
                }
            }

            const std::string apiPassword = jsonString(body, "apiPassword").empty()
                                                ? HmacUtils::randomHex(16)
                                                : jsonString(body, "apiPassword");
            const std::string radiusSecret = jsonString(body, "radiusSecret").empty()
                                                 ? HmacUtils::randomHex(16)
                                                 : jsonString(body, "radiusSecret");
            const std::string apiUsername = jsonString(body, "apiUsername", "magma-api");

            const std::string id = newId();
            auto rows = co_await db->execSqlCoro(
                "INSERT INTO magma_devices (id, business_id, zone_id, name, role, vendor, model, "
                "serial_number, mac_address, tunnel_ip, management_ip, rest_scheme, rest_port, "
                "api_username, api_secret_ciphertext, radius_secret_ciphertext, enrolment_status, status) "
                "VALUES ($1,$2,NULLIF($3,'')::uuid,$4,$5,'MikroTik',NULLIF($6,''),NULLIF($7,''),"
                "NULLIF($8,'')::macaddr, NULLIF($9,'')::inet, NULLIF($10,'')::inet,$11,$12,$13,"
                "$14, $15,'ENROLLED','UNKNOWN') RETURNING *",
                id, businessId, zoneId, name, role, jsonString(body, "model"),
                jsonString(body, "serialNumber"), jsonString(body, "macAddress"),
                jsonString(body, "tunnelIp"), jsonString(body, "managementIp"),
                jsonString(body, "restScheme", "http"),
                static_cast<int>(jsonInt64(body, "restPort", 80)),
                apiUsername, magma::encryptSecret(apiPassword), magma::encryptSecret(radiusSecret));

            if (role == "GATEWAY") {
                co_await radius_.syncNas(id);
            }

            Json::Value result = jsonRow(rows[0]);
            result.removeMember("api_secret_ciphertext");
            result.removeMember("radius_secret_ciphertext");
            // Returned once so the installer can paste it into bootstrap.rsc.
            result["apiPassword"] = apiPassword;
            result["radiusSecret"] = radiusSecret;
            co_await magma::auditLog(businessId, "STAFF", "", "DEVICE_REGISTERED",
                                     "magma_devices", id, jsonRow(rows[0]));
            co_return okResponse("Device registered", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] registerDevice error: " << e.what();
            co_return errorResponse("Unable to register device");
        }
    }

    Task<dto::BaseApiResponse> MagmaDeviceService::listDevices(const std::string &businessId,
                                                               const std::string &zoneId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT d.id, d.zone_id, z.zone_code, d.name, d.role, d.model, d.serial_number, "
                "d.routeros_version, host(d.tunnel_ip) AS tunnel_ip, d.status, d.last_seen_at, "
                "d.cpu_load, d.free_memory_mb, d.temperature_c, d.uptime_seconds "
                "FROM magma_devices d LEFT JOIN magma_zones z ON z.id=d.zone_id "
                "WHERE d.business_id=$1 AND ($2='' OR d.zone_id::text=$2) "
                "ORDER BY z.zone_code, d.role, d.name", businessId, zoneId);
            co_return okResponse("Devices", rowsToJson(rows));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] listDevices error: " << e.what();
            co_return errorResponse("Unable to list devices");
        }
    }

    Task<dto::BaseApiResponse> MagmaDeviceService::getDevice(const std::string &businessId,
                                                             const std::string &deviceId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT d.id, d.zone_id, z.zone_code, d.name, d.role, d.model, d.serial_number, "
                "d.mac_address, d.routeros_version, host(d.tunnel_ip) AS tunnel_ip, d.rest_scheme, "
                "d.rest_port, d.api_username, d.status, d.last_seen_at, d.cpu_load, d.free_memory_mb, "
                "d.temperature_c, d.uptime_seconds, d.enrolment_status, d.notes "
                "FROM magma_devices d LEFT JOIN magma_zones z ON z.id=d.zone_id "
                "WHERE d.id=$1 AND d.business_id=$2", deviceId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Device not found");
            }
            co_return okResponse("Device", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getDevice error: " << e.what();
            co_return errorResponse("Unable to load device");
        }
    }

    Task<dto::BaseApiResponse> MagmaDeviceService::decommissionDevice(
        const std::string &businessId, const std::string &deviceId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "UPDATE magma_devices SET enrolment_status='REVOKED', status='OFFLINE', updated_at=now() "
                "WHERE id=$1 AND business_id=$2 RETURNING id, name, host(tunnel_ip) AS tunnel_ip", deviceId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Device not found");
            }
            if (!rows[0]["tunnel_ip"].isNull()) {
                const std::string schema = MagmaSettings::instance().radiusSchema;
                co_await db->execSqlCoro(
                    "DELETE FROM " + schema + ".nas WHERE nasname=$1",
                    rows[0]["tunnel_ip"].as<std::string>());
            }
            co_await magma::auditLog(businessId, "STAFF", "", "DEVICE_DECOMMISSIONED",
                                     "magma_devices", deviceId, Json::Value(Json::objectValue));
            co_return okResponse("Device decommissioned", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] decommissionDevice error: " << e.what();
            co_return errorResponse("Unable to decommission device");
        }
    }

    Task<dto::BaseApiResponse> MagmaDeviceService::pushDeviceConfig(
        const std::string &businessId, const std::string &deviceId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT d.id, d.business_id, d.zone_id, d.name, d.role, d.serial_number, d.rest_scheme, d.rest_port, d.api_username, d.api_secret_ciphertext, d.radius_secret_ciphertext, d.status, d.enrolment_status, host(d.tunnel_ip) AS tunnel_ip, z.zone_code, z.name AS zone_name "
                "FROM magma_devices d LEFT JOIN magma_zones z ON z.id=d.zone_id "
                "WHERE d.id=$1 AND d.business_id=$2", deviceId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Device not found");
            }
            auto client = clientForRow(rows[0]);
            if (!client) {
                co_return errorResponse("Device has no tunnel IP or API credentials");
            }

            const std::string radiusSecret = magma::decryptSecret(
                rows[0]["radius_secret_ciphertext"].isNull()
                    ? "" : rows[0]["radius_secret_ciphertext"].as<std::string>());
            if (radiusSecret.empty()) {
                co_return errorResponse("Device RADIUS secret missing");
            }

            Json::Value steps(Json::arrayValue);
            const auto &settings = MagmaSettings::instance();

            // 1. RADIUS client entry (idempotent by address).
            auto radiusList = co_await client->get("/radius");
            bool radiusExists = false;
            if (radiusList.ok && radiusList.json.isArray()) {
                for (const auto &entry : radiusList.json) {
                    if (entry["address"].asString() == settings.radiusHost) {
                        radiusExists = true;
                        Json::Value patch;
                        patch["secret"] = radiusSecret;
                        patch["service"] = "hotspot,pppoe";
                        auto applied = co_await client->patch(
                            "/radius/" + entry[".id"].asString(), patch);
                        steps.append(Json::Value("radius:" + std::string(applied.ok ? "ok" : "failed")));
                    }
                }
            }
            if (!radiusExists) {
                Json::Value create;
                create["address"] = settings.radiusHost;
                create["secret"] = radiusSecret;
                create["service"] = "hotspot,pppoe";
                create["authentication-port"] = settings.radiusAuthPort;
                create["accounting-port"] = settings.radiusAcctPort;
                create["timeout"] = "1s";
                auto applied = co_await client->put("/radius", create);
                steps.append(Json::Value("radius:" + std::string(applied.ok ? "created" : "failed")));
            }

            // 2. Accept CoA / disconnect from RADIUS.
            Json::Value incoming;
            incoming["accept"] = "yes";
            incoming["port"] = settings.radiusCoaPort;
            auto coaConfig = co_await client->patch("/radius/incoming", incoming);
            steps.append(Json::Value("coa:" + std::string(coaConfig.ok ? "ok" : "failed")));

            // 3. Hotspot profile uses RADIUS instead of the local user database.
            auto profiles = co_await client->get("/ip/hotspot/profile");
            if (profiles.ok && profiles.json.isArray()) {
                for (const auto &profile : profiles.json) {
                    Json::Value patch;
                    patch["use-radius"] = "yes";
                    patch["radius-accounting"] = "yes";
                    patch["interim-update"] = "1m";
                    auto applied = co_await client->patch(
                        "/ip/hotspot/profile/" + profile[".id"].asString(), patch);
                    steps.append(Json::Value("hotspot-profile:" +
                                             std::string(applied.ok ? "ok" : "failed")));
                }
            }

            // 4. Walled garden: portal + payment providers must be reachable pre-auth.
            std::string portalHost = settings.portalBaseUrl;
            const auto schemePos = portalHost.find("://");
            if (schemePos != std::string::npos) {
                portalHost = portalHost.substr(schemePos + 3);
            }
            portalHost = portalHost.substr(0, portalHost.find('/'));

            Json::Value garden;
            garden["dst-host"] = portalHost;
            garden["action"] = "accept";
            auto gardenResult = co_await client->put("/ip/hotspot/walled-garden", garden);
            steps.append(Json::Value("walled-garden:" +
                                     std::string(gardenResult.ok ? "ok" : "failed")));

            // 5. Lock down management services.
            auto serviceList = co_await client->get("/ip/service");
            if (serviceList.ok && serviceList.json.isArray()) {
                for (const auto &service : serviceList.json) {
                    const std::string name = service["name"].asString();
                    if (name != "telnet" && name != "ftp" && name != "api" && name != "www") {
                        continue;
                    }
                    Json::Value patch;
                    patch["disabled"] = (name == "www") ? "no" : "yes";
                    auto applied = co_await client->patch(
                        "/ip/service/" + service[".id"].asString(), patch);
                    steps.append(Json::Value("service-" + name + ":" +
                                             std::string(applied.ok ? "ok" : "failed")));
                }
            }

            co_await db->execSqlCoro(
                "UPDATE magma_devices SET last_seen_at=now(), status='ONLINE', updated_at=now() "
                "WHERE id=$1", deviceId);

            Json::Value result;
            result["deviceId"] = deviceId;
            result["steps"] = steps;
            result["radiusHost"] = settings.radiusHost;
            co_return okResponse("Configuration pushed", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] pushDeviceConfig error: " << e.what();
            co_return errorResponse("Unable to push configuration");
        }
    }

    Task<dto::BaseApiResponse> MagmaDeviceService::rebootDevice(const std::string &businessId,
                                                                const std::string &deviceId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT d.id, d.business_id, d.zone_id, d.name, d.role, d.rest_scheme, d.rest_port, d.api_username, d.api_secret_ciphertext, d.radius_secret_ciphertext, d.status, d.enrolment_status, host(d.tunnel_ip) AS tunnel_ip FROM magma_devices d "
                "WHERE d.id=$1 AND d.business_id=$2", deviceId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Device not found");
            }
            auto client = clientForRow(rows[0]);
            if (!client) {
                co_return errorResponse("Device has no tunnel IP or API credentials");
            }
            auto rebooted = co_await client->reboot();
            if (!rebooted.ok) {
                co_return errorResponse("Reboot failed: " + rebooted.error);
            }
            co_await magma::auditLog(businessId, "STAFF", "", "DEVICE_REBOOTED",
                                     "magma_devices", deviceId, Json::Value(Json::objectValue));
            co_return okResponse("Reboot issued", Json::Value(Json::objectValue));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] rebootDevice error: " << e.what();
            co_return errorResponse("Unable to reboot device");
        }
    }

    Task<dto::BaseApiResponse> MagmaDeviceService::runDeviceDiagnostics(
        const std::string &businessId, const std::string &deviceId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT d.id, d.business_id, d.zone_id, d.name, d.role, d.rest_scheme, d.rest_port, d.api_username, d.api_secret_ciphertext, d.radius_secret_ciphertext, d.status, d.enrolment_status, host(d.tunnel_ip) AS tunnel_ip FROM magma_devices d "
                "WHERE d.id=$1 AND d.business_id=$2", deviceId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Device not found");
            }
            auto client = clientForRow(rows[0]);
            if (!client) {
                co_return errorResponse("Device has no tunnel IP or API credentials");
            }

            auto resource = co_await client->systemResource();
            auto interfaces = co_await client->interfaces();

            Json::Value result;
            result["reachable"] = resource.ok;
            result["resource"] = resource.json;
            result["interfaces"] = interfaces.json;
            result["error"] = resource.error;
            co_return okResponse(resource.ok ? "Device reachable" : "Device unreachable", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] runDeviceDiagnostics error: " << e.what();
            co_return errorResponse("Diagnostics failed");
        }
    }

    Task<dto::BaseApiResponse> MagmaDeviceService::pollHealth(const std::string &businessId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT d.id, d.business_id, d.zone_id, d.name, d.role, d.rest_scheme, d.rest_port, d.api_username, d.api_secret_ciphertext, d.radius_secret_ciphertext, d.status, d.enrolment_status, host(d.tunnel_ip) AS tunnel_ip FROM magma_devices d "
                "WHERE d.business_id=$1 AND d.enrolment_status='ENROLLED' "
                "AND d.role IN ('GATEWAY','AP') AND d.tunnel_ip IS NOT NULL", businessId);

            int online = 0;
            int offline = 0;

            for (const auto &row : rows) {
                auto client = clientForRow(row);
                const std::string deviceId = row["id"].as<std::string>();
                const std::string previous = row["status"].as<std::string>();

                if (!client) {
                    continue;
                }
                auto resource = co_await client->systemResource();
                if (resource.ok && resource.json.isObject()) {
                    auto &json = resource.json;
                    const int cpu = json["cpu-load"].isString()
                                        ? std::stoi(json["cpu-load"].asString())
                                        : json["cpu-load"].asInt();
                    const int64_t freeMemory = json["free-memory"].isString()
                                                   ? std::stoll(json["free-memory"].asString())
                                                   : json["free-memory"].asInt64();
                    const std::string uptime = json["uptime"].asString();
                    const std::string version = json["version"].asString();

                    co_await db->execSqlCoro(
                        "UPDATE magma_devices SET status='ONLINE', last_seen_at=now(), cpu_load=$2, "
                        "free_memory_mb=$3, routeros_version=COALESCE(NULLIF($4,''), routeros_version), "
                        "updated_at=now() WHERE id=$1",
                        deviceId, cpu, static_cast<int>(freeMemory / (1024 * 1024)), version);
                    (void)uptime;
                    ++online;
                    if (previous != "ONLINE") {
                        co_await db->execSqlCoro(
                            "INSERT INTO magma_alerts (business_id, zone_id, device_id, alert_type, "
                            "severity, message) VALUES ($1,NULLIF($2,'')::uuid,$3,'DEVICE_ONLINE','INFO',$4)",
                            businessId,
                            row["zone_id"].isNull() ? std::string("") : row["zone_id"].as<std::string>(),
                            deviceId,
                            "Device " + row["name"].as<std::string>() + " is back online");
                    }
                } else {
                    co_await db->execSqlCoro(
                        "UPDATE magma_devices SET status='OFFLINE', updated_at=now() WHERE id=$1",
                        deviceId);
                    ++offline;
                    if (previous == "ONLINE" || previous == "UNKNOWN") {
                        co_await db->execSqlCoro(
                            "INSERT INTO magma_alerts (business_id, zone_id, device_id, alert_type, "
                            "severity, message, payload) VALUES ($1,NULLIF($2,'')::uuid,$3,'DEVICE_OFFLINE','CRITICAL',$4,$5::jsonb)",
                            businessId,
                            row["zone_id"].isNull() ? std::string("") : row["zone_id"].as<std::string>(),
                            deviceId,
                            "Device " + row["name"].as<std::string>() + " is unreachable: " + resource.error,
                            toJsonString(Json::Value(resource.error)));
                    }
                }
            }

            Json::Value result;
            result["checked"] = static_cast<int64_t>(rows.size());
            result["online"] = online;
            result["offline"] = offline;
            co_return okResponse("Health sweep complete", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] pollHealth error: " << e.what();
            co_return errorResponse("Health sweep failed");
        }
    }

    Task<dto::BaseApiResponse> MagmaDeviceService::hello(const Json::Value &body) {
        const std::string serial = jsonString(body, "serialNumber");
        if (serial.empty()) {
            co_return errorResponse("serialNumber is required");
        }
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT d.*, host(d.tunnel_ip) AS tunnel_ip_text, z.zone_code "
                "FROM magma_devices d LEFT JOIN magma_zones z ON z.id=d.zone_id "
                "WHERE d.serial_number=$1", serial);
            if (rows.empty()) {
                co_return errorResponse("Device is not registered");
            }
            if (rows[0]["enrolment_status"].as<std::string>() == "REVOKED") {
                co_return errorResponse("Device enrolment has been revoked");
            }

            co_await db->execSqlCoro(
                "UPDATE magma_devices SET enrolment_status='ENROLLED', status='ONLINE', "
                "last_seen_at=now(), routeros_version=COALESCE(NULLIF($2,''), routeros_version) "
                "WHERE id=$1",
                rows[0]["id"].as<std::string>(), jsonString(body, "routerosVersion"));

            Json::Value config = co_await desiredConfig(rows[0]["id"].as<std::string>());
            Json::Value result = config.result;
            result["deviceId"] = rows[0]["id"];
            result["zone"] = rows[0]["zone_code"].isNull() ? "" : rows[0]["zone_code"];
            co_return okResponse("Device enrolled", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] device hello error: " << e.what();
            co_return errorResponse("Enrolment failed");
        }
    }

    Task<dto::BaseApiResponse> MagmaDeviceService::heartbeat(const std::string &deviceId,
                                                             const Json::Value &body) {
        try {
            auto db = app().getDbClient();
            co_await db->execSqlCoro(
                "UPDATE magma_devices SET status='ONLINE', last_seen_at=now(), "
                "cpu_load=COALESCE(NULLIF($2,-1), cpu_load), "
                "free_memory_mb=COALESCE(NULLIF($3,-1), free_memory_mb), "
                "uptime_seconds=COALESCE(NULLIF($4,-1), uptime_seconds), updated_at=now() WHERE id=$1",
                deviceId,
                static_cast<int>(jsonInt64(body, "cpuLoad", -1)),
                static_cast<int>(jsonInt64(body, "freeMemoryMb", -1)),
                jsonInt64(body, "uptimeSeconds", -1));
            co_return okResponse("Heartbeat recorded", Json::Value(Json::objectValue));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] heartbeat error: " << e.what();
            co_return errorResponse("Heartbeat failed");
        }
    }

    Task<dto::BaseApiResponse> MagmaDeviceService::desiredConfig(const std::string &deviceId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT d.id, d.role, d.api_username, d.api_secret_ciphertext, "
                "d.radius_secret_ciphertext, host(d.tunnel_ip) AS tunnel_ip, z.zone_code, z.name AS zone_name "
                "FROM magma_devices d LEFT JOIN magma_zones z ON z.id=d.zone_id WHERE d.id=$1", deviceId);
            if (rows.empty()) {
                co_return errorResponse("Device not found");
            }
            const auto &settings = MagmaSettings::instance();

            Json::Value config;
            config["role"] = rows[0]["role"];
            config["zone"] = rows[0]["zone_code"].isNull() ? "" : rows[0]["zone_code"];
            config["radius"] = Json::Value(Json::objectValue);
            config["radius"]["address"] = settings.radiusHost;
            config["radius"]["authPort"] = settings.radiusAuthPort;
            config["radius"]["acctPort"] = settings.radiusAcctPort;
            config["radius"]["coaPort"] = settings.radiusCoaPort;
            config["radius"]["secret"] = magma::decryptSecret(
                rows[0]["radius_secret_ciphertext"].isNull()
                    ? "" : rows[0]["radius_secret_ciphertext"].as<std::string>());
            config["api"] = Json::Value(Json::objectValue);
            config["api"]["username"] = rows[0]["api_username"].isNull()
                                            ? "" : rows[0]["api_username"].as<std::string>();
            config["portalBaseUrl"] = settings.portalBaseUrl;
            config["heartbeatSeconds"] = 60;
            co_return okResponse("Desired configuration", config);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] desiredConfig error: " << e.what();
            co_return errorResponse("Unable to build configuration");
        }
    }

} // namespace data_support::services
