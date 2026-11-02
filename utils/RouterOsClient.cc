//
// MagmaNet — minimal RouterOS v7 REST client.
//

#include "RouterOsClient.h"

#include <drogon/utils/Utilities.h>

#include <algorithm>
#include <cctype>

namespace data_support::utils {

    namespace {
        std::string normalisePath(const std::string &path) {
            if (path.empty()) {
                return "/rest";
            }
            if (path.rfind("/rest", 0) == 0) {
                return path;
            }
            if (path.front() == '/') {
                return "/rest" + path;
            }
            return "/rest/" + path;
        }

        std::string lowerCopy(std::string value) {
            std::transform(value.begin(), value.end(), value.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return value;
        }
    }

    RouterOsClient::RouterOsClient(std::string scheme,
                                   std::string host,
                                   int port,
                                   std::string username,
                                   std::string password,
                                   double timeoutSeconds)
        : username_(std::move(username)),
          password_(std::move(password)),
          timeoutSeconds_(timeoutSeconds) {
        if (scheme.empty()) {
            scheme = "http";
        }
        baseUrl_ = scheme + "://" + host + ":" + std::to_string(port);
    }

    drogon::Task<RouterOsResult> RouterOsClient::request(drogon::HttpMethod method,
                                                         const std::string &path,
                                                         const Json::Value *body) {
        RouterOsResult result;

        try {
            auto client = drogon::HttpClient::newHttpClient(baseUrl_);
            auto req = drogon::HttpRequest::newHttpRequest();
            req->setMethod(method);
            req->setPath(normalisePath(path));
            req->setContentTypeCode(drogon::CT_APPLICATION_JSON);

            const std::string credentials = username_ + ":" + password_;
            req->addHeader("Authorization",
                           "Basic " + drogon::utils::base64Encode(
                                         reinterpret_cast<const unsigned char *>(credentials.data()),
                                         credentials.size()));

            if (body != nullptr) {
                Json::StreamWriterBuilder writer;
                writer["indentation"] = "";
                req->setBody(Json::writeString(writer, *body));
            }

            auto httpResponse = co_await client->sendRequestCoro(req, timeoutSeconds_);
            if (httpResponse == nullptr) {
                result.error = "No response from RouterOS device";
                co_return result;
            }

            result.statusCode = static_cast<int>(httpResponse->getStatusCode());
            result.body = std::string(httpResponse->getBody());
            result.ok = result.statusCode >= 200 && result.statusCode < 300;

            if (!result.body.empty()) {
                Json::CharReaderBuilder builder;
                std::string errors;
                std::istringstream stream(result.body);
                Json::Value parsed;
                if (Json::parseFromStream(builder, stream, &parsed, &errors)) {
                    result.json = parsed;
                }
            }

            if (!result.ok) {
                // RouterOS returns {"error":..., "detail":...}
                const std::string detail = result.json.isObject()
                                               ? (result.json["detail"].asString() +
                                                  " " + result.json["error"].asString())
                                               : result.body;
                result.error = "RouterOS HTTP " + std::to_string(result.statusCode) + ": " +
                               lowerCopy(detail);
            }
        } catch (const std::exception &e) {
            result.ok = false;
            result.error = std::string("RouterOS request failed: ") + e.what();
        }

        co_return result;
    }

    drogon::Task<RouterOsResult> RouterOsClient::get(const std::string &path) {
        return request(drogon::Get, path, nullptr);
    }

    drogon::Task<RouterOsResult> RouterOsClient::post(const std::string &path,
                                                      const Json::Value &body) {
        return request(drogon::Post, path, &body);
    }

    drogon::Task<RouterOsResult> RouterOsClient::put(const std::string &path,
                                                     const Json::Value &body) {
        co_return co_await request(drogon::Put, path, &body);
    }

    drogon::Task<RouterOsResult> RouterOsClient::patch(const std::string &path,
                                                       const Json::Value &body) {
        return request(drogon::Patch, path, &body);
    }

    drogon::Task<RouterOsResult> RouterOsClient::remove(const std::string &path) {
        return request(drogon::Delete, path, nullptr);
    }

    drogon::Task<RouterOsResult> RouterOsClient::systemResource() {
        return get("/system/resource");
    }

    drogon::Task<RouterOsResult> RouterOsClient::interfaces() {
        return get("/interface");
    }

    drogon::Task<RouterOsResult> RouterOsClient::hotspotHosts() {
        return get("/ip/hotspot/host");
    }

    drogon::Task<RouterOsResult> RouterOsClient::hotspotActive() {
        return get("/ip/hotspot/active");
    }

    drogon::Task<RouterOsResult> RouterOsClient::wirelessRegistrationTable() {
        return get("/interface/wireless/registration-table");
    }

    drogon::Task<RouterOsResult> RouterOsClient::reboot() {
        Json::Value body(Json::objectValue);
        return post("/system/reboot", body);
    }

    drogon::Task<RouterOsResult> RouterOsClient::loginHotspotClient(const std::string &ip,
                                                                    const std::string &mac,
                                                                    const std::string &username,
                                                                    const std::string &password) {
        Json::Value body(Json::objectValue);
        body["ip"] = ip;
        body["mac"] = mac;
        body["username"] = username;
        body["password"] = password;
        return post("/ip/hotspot/active/login", body);
    }

    drogon::Task<bool> RouterOsClient::hostTableContains(const std::string &mac,
                                                         const std::string &ip) {
        auto result = co_await hotspotHosts();
        if (!result.ok || !result.json.isArray()) {
            co_return false;
        }

        const std::string wantedMac = lowerCopy(mac);
        for (const auto &entry : result.json) {
            const std::string entryMac = lowerCopy(entry["mac-address"].asString());
            if (entryMac.empty() || entryMac != wantedMac) {
                continue;
            }
            if (ip.empty()) {
                co_return true;
            }
            const std::string entryAddress = entry["address"].asString();
            if (entryAddress.empty() || entryAddress == ip) {
                co_return true;
            }
        }
        co_return false;
    }

} // namespace data_support::utils
