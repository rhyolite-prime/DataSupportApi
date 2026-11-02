//
// MagmaNet — plan DTO.
//

#ifndef DATASUPPORTAPI_MAGMANETPLANDTO_H
#define DATASUPPORTAPI_MAGMANETPLANDTO_H

#include <json/json.h>
#include <string>
#include <vector>

namespace data_support::dto::magma_net {

    class DataPlanDto {
    public:
        DataPlanDto() = default;
        explicit DataPlanDto(const Json::Value &json) { fromJson(json); }

        void fromJson(const Json::Value &json) {
            auto str = [&json](const char *key, const std::string &fallback = "") {
                return json.isMember(key) && !json[key].isNull() ? json[key].asString() : fallback;
            };
            planCode = str("planCode");
            name = str("name");
            planType = str("planType", "VOLUME");
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
            durationMinutes = static_cast<int>(num("durationMinutes", 0));
            dataCapMb = static_cast<int64_t>(num("dataCapMb", 0));
            fairUseCapMb = static_cast<int64_t>(num("fairUseCapMb", 0));
            throttleDownKbps = static_cast<int>(num("throttleDownKbps", 0));
            throttleUpKbps = static_cast<int>(num("throttleUpKbps", 0));
            downKbps = static_cast<int>(num("downKbps", 0));
            upKbps = static_cast<int>(num("upKbps", 0));
            burstDownKbps = static_cast<int>(num("burstDownKbps", 0));
            burstUpKbps = static_cast<int>(num("burstUpKbps", 0));
            deviceLimit = static_cast<int>(num("deviceLimit", 1));
            price = num("price", 0.0);
            wholesalePrice = num("wholesalePrice", 0.0);
            agentCommission = num("agentCommission", 0.0);
            taxInclusive = boolean("taxInclusive", true);
            validityDays = static_cast<int>(num("validityDays", 30));
            isPublic = boolean("isPublic", true);
            sortOrder = static_cast<int>(num("sortOrder", 0));
            if (json.isMember("zoneIds") && json["zoneIds"].isArray()) {
                for (const auto &zone : json["zoneIds"]) {
                    zoneIds.push_back(zone.asString());
                }
            }
        }

        std::string planCode, name, planType = "VOLUME";
        int durationMinutes = 0;
        int64_t dataCapMb = 0;
        int64_t fairUseCapMb = 0;
        int throttleDownKbps = 0, throttleUpKbps = 0;
        int downKbps = 0, upKbps = 0;
        int burstDownKbps = 0, burstUpKbps = 0;
        int deviceLimit = 1;
        double price = 0, wholesalePrice = 0, agentCommission = 0;
        bool taxInclusive = true;
        int validityDays = 30;
        bool isPublic = true;
        int sortOrder = 0;
        std::vector<std::string> zoneIds;
    };

} // namespace data_support::dto::magma_net

#endif //DATASUPPORTAPI_MAGMANETPLANDTO_H
