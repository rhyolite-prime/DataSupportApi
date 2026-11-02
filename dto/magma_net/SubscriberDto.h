//
// MagmaNet — subscriber DTO.
//

#ifndef DATASUPPORTAPI_MAGMANETSUBSCRIBERDTO_H
#define DATASUPPORTAPI_MAGMANETSUBSCRIBERDTO_H

#include <json/json.h>
#include <string>

namespace data_support::dto::magma_net {

    class SubscriberDto {
    public:
        SubscriberDto() = default;
        explicit SubscriberDto(const Json::Value &json) { fromJson(json); }

        void fromJson(const Json::Value &json) {
            auto str = [&json](const char *key, const std::string &fallback = "") {
                return json.isMember(key) && !json[key].isNull() ? json[key].asString() : fallback;
            };
            subscriberId = str("subscriberId");
            msisdn = str("msisdn");
            fullName = str("fullName");
            email = str("email");
            subscriberType = str("subscriberType", "WALK_IN");
            authType = str("authType", "HOTSPOT");
            homeZoneId = str("homeZoneId");
            serviceAddress = str("serviceAddress");
            ghanaCardNumber = str("ghanaCardNumber");
            status = str("status", "ACTIVE");
            consent = json.get("consent", false).asBool();
            gpsLat = json.isMember("gpsLat") ? json["gpsLat"].asDouble() : 0.0;
            gpsLng = json.isMember("gpsLng") ? json["gpsLng"].asDouble() : 0.0;
        }

        std::string subscriberId, msisdn, fullName, email;
        std::string subscriberType = "WALK_IN", authType = "HOTSPOT";
        std::string homeZoneId, serviceAddress, ghanaCardNumber, status = "ACTIVE";
        bool consent = false;
        double gpsLat = 0, gpsLng = 0;
    };

} // namespace data_support::dto::magma_net

#endif //DATASUPPORTAPI_MAGMANETSUBSCRIBERDTO_H
