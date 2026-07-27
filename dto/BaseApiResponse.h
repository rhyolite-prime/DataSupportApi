//
// Created by Emmanuel Addo-Odame on 22/07/2026.
//

#ifndef DATASUPPORTAPI_BASEAPIRESPONSE_H
#define DATASUPPORTAPI_BASEAPIRESPONSE_H

#include <json/json.h>
#include <string>

namespace data_support::dto {
    class BaseApiResponse {
    public:
        Json::Value result;
        std::string targetUrl;
        std::string message;
        bool success;
        Json::Value error;

        BaseApiResponse() : success(false) {}

        [[nodiscard]]
        Json::Value toJson() const {
            Json::Value json;
            json["result"] = result;
            json["targetUrl"] = targetUrl;
            json["message"] = message;
            json["success"] = success;
            json["error"] = error;
            return json;
        }
    };

}

#endif //DATASUPPORTAPI_BASEAPIRESPONSE_H