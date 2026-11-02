//
// MagmaNet — zone DTO.
//

#ifndef DATASUPPORTAPI_MAGMANETZONEDTO_H
#define DATASUPPORTAPI_MAGMANETZONEDTO_H

#include <json/json.h>
#include <string>

namespace data_support::dto::magma_net {

    class ZoneDto {
    public:
        ZoneDto() = default;
        explicit ZoneDto(const Json::Value &json) { fromJson(json); }

        void fromJson(const Json::Value &json) {
            auto str = [&json](const char *key, const std::string &fallback = "") {
                return json.isMember(key) && !json[key].isNull() ? json[key].asString() : fallback;
            };
            zoneCode = str("zoneCode");
            name = str("name");
            district = str("district");
            region = str("region");
            backhaulProvider = str("backhaulProvider");
            backhaulCircuitRef = str("backhaulCircuitRef");
            publicIp = str("publicIp");
            tunnelIp = str("tunnelIp");
            status = str("status", "PLANNED");
            notes = str("notes");
            auto num = [&json](const char *key, double fallback) {
                if (!json.isMember(key) || json[key].isNull()) return fallback;
                if (json[key].isString()) { try { return std::stod(json[key].asString()); } catch (...) { return fallback; } }
                return json[key].asDouble();
            };
            auto boolean = [&json](const char *key, bool fallback) {
                if (!json.isMember(key) || json[key].isNull()) return fallback;
                if (json[key].isString()) return json[key].asString() == "true";
                return json[key].asBool();
            };
            gpsLat = num("gpsLat", 0.0);
            gpsLng = num("gpsLng", 0.0);
            upstreamDownMbps = num("upstreamDownMbps", 0.0);
            upstreamUpMbps = num("upstreamUpMbps", 0.0);
            upstreamMonthlyCost = num("upstreamMonthlyCost", 0.0);
            cgnat = boolean("upstreamIsCgnat", true);
        }

        std::string zoneCode, name, district, region, backhaulProvider, backhaulCircuitRef;
        std::string publicIp, tunnelIp, status, notes;
        double gpsLat = 0, gpsLng = 0;
        double upstreamDownMbps = 0, upstreamUpMbps = 0, upstreamMonthlyCost = 0;
        bool cgnat = true;
    };

} // namespace data_support::dto::magma_net

#endif //DATASUPPORTAPI_MAGMANETZONEDTO_H
