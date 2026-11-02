//
// MagmaNet — minimal RouterOS v7 REST client.
//
// Devices are managed over the WireGuard tunnel, so plain HTTP is used by
// default (the tunnel already provides confidentiality) and RouterOS' default
// self-signed certificate never gets in the way. Set scheme=https if you have
// installed a trusted certificate on the device.
//

#ifndef DATASUPPORTAPI_ROUTEROSCLIENT_H
#define DATASUPPORTAPI_ROUTEROSCLIENT_H

#include <drogon/drogon.h>
#include <json/json.h>

#include <string>

namespace data_support::utils {

    struct RouterOsResult {
        bool ok = false;
        int statusCode = 0;
        std::string error;
        std::string body;
        Json::Value json = Json::Value(Json::arrayValue);
    };

    class RouterOsClient {
    public:
        RouterOsClient(std::string scheme,
                       std::string host,
                       int port,
                       std::string username,
                       std::string password,
                       double timeoutSeconds = 5.0);

        drogon::Task<RouterOsResult> get(const std::string &path);
        drogon::Task<RouterOsResult> post(const std::string &path, const Json::Value &body);
        drogon::Task<RouterOsResult> put(const std::string &path, const Json::Value &body);
        drogon::Task<RouterOsResult> patch(const std::string &path, const Json::Value &body);
        drogon::Task<RouterOsResult> remove(const std::string &path);

        // ---- convenience endpoints -------------------------------------
        drogon::Task<RouterOsResult> systemResource();
        drogon::Task<RouterOsResult> interfaces();
        drogon::Task<RouterOsResult> hotspotHosts();
        drogon::Task<RouterOsResult> hotspotActive();
        drogon::Task<RouterOsResult> wirelessRegistrationTable();
        drogon::Task<RouterOsResult> reboot();

        // Server-side hotspot login — the authoritative way to bring a paid
        // client online without ever putting credentials in the browser URL.
        drogon::Task<RouterOsResult> loginHotspotClient(const std::string &ip,
                                                        const std::string &mac,
                                                        const std::string &username,
                                                        const std::string &password);

        // Returns true when `mac` is present in /ip/hotspot/host for this NAS,
        // optionally matching the reported IP as well.
        drogon::Task<bool> hostTableContains(const std::string &mac, const std::string &ip);

    private:
        drogon::Task<RouterOsResult> request(drogon::HttpMethod method,
                                             const std::string &path,
                                             const Json::Value *body);

        std::string baseUrl_;
        std::string username_;
        std::string password_;
        double timeoutSeconds_;
    };

} // namespace data_support::utils

#endif //DATASUPPORTAPI_ROUTEROSCLIENT_H
