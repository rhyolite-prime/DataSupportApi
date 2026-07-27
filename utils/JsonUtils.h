//
// Created by Emmanuel Addo-Odame on 23/07/2026.
//

#ifndef DATASUPPORTAPI_JSONUTILS_H
#define DATASUPPORTAPI_JSONUTILS_H

#include <json/json.h>
#include <string>
#include <cctype>

namespace data_support::utils {

class JsonUtils {
public:
    static std::string toCamelCase(const std::string& snakeCaseStr) {
        std::string camelKey;
        bool nextUpper = false;
        for (char c : snakeCaseStr) {
            if (c == '_') {
                nextUpper = true;
            } else {
                if (nextUpper) {
                    camelKey += static_cast<char>(std::toupper(c));
                    nextUpper = false;
                } else {
                    camelKey += c;
                }
            }
        }
        return camelKey;
    }

    static Json::Value convertKeysToCamelCase(const Json::Value& json) {
        if (!json.isObject()) {
            return json;
        }
        
        Json::Value camelJson;
        for (auto const& id : json.getMemberNames()) {
            camelJson[toCamelCase(id)] = json[id];
        }
        return camelJson;
    }
};

} // namespace data_support::utils

#endif //DATASUPPORTAPI_JSONUTILS_H
