//
// Created by Emmanuel Addo-Odame on 23/07/2026.
//

#ifndef DATASUPPORTAPI_QUARTZJOBDTO_H
#define DATASUPPORTAPI_QUARTZJOBDTO_H

#include <string>
#include <json/json.h>

namespace data_support::dto {

    class QuartzJobDto {
    public:
        std::string name;
        std::string description;
        std::string schedule;
        std::string startDate;
        std::string endDate;
        struct CustomData {
            std::string callbackUrl;
            std::string uniqueId;
        } customData;

        [[nodiscard]] Json::Value toJson() const {
            Json::Value json;
            json["name"] = name;
            json["description"] = description;
            json["schedule"] = schedule;
            json["startDate"] = toIso8601(startDate);
            json["endDate"] = toIso8601(endDate);

            Json::Value customDataJson;
            customDataJson["callbackUrl"] = customData.callbackUrl;
            customDataJson["uniqueId"] = customData.uniqueId;
            json["customData"] = customDataJson;

            return json;
        }

    private:
        static std::string toIso8601(const std::string &dbDate) {
            if (dbDate.size() >= 10 && dbDate[10] == ' ') {
                std::string iso = dbDate;
                iso[10] = 'T';
                return iso;
            }
            return dbDate;
        }
    };

}


#endif //DATASUPPORTAPI_QUARTZJOBDTO_H