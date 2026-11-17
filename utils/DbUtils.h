//
// MagmaNet — small helpers shared by the service layer.
//

#ifndef DATASUPPORTAPI_DBUTILS_H
#define DATASUPPORTAPI_DBUTILS_H

#include <drogon/drogon.h>
#include <drogon/orm/Result.h>
#include <drogon/orm/Row.h>
#include <json/json.h>

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include "dto/BaseApiResponse.h"
#include "utils/IdGeneratorUtils.h"

namespace data_support::utils {

    /** Convert one SQL row to a JSON object. DECIMAL/NUMERIC come back as text. */
    inline Json::Value jsonRow(const drogon::orm::Row &row) {
        Json::Value object(Json::objectValue);
        for (size_t i = 0; i < row.size(); ++i) {
            const auto &field = row[i];
            const std::string name = field.name() ? field.name() : "";
            if (name.empty()) {
                continue;
            }
            if (field.isNull()) {
                object[name] = Json::Value();
            } else {
                object[name] = field.as<std::string>();
            }
        }
        return object;
    }

    inline Json::Value rowsToJson(const drogon::orm::Result &result) {
        Json::Value array(Json::arrayValue);
        for (const auto &row : result) {
            array.append(jsonRow(row));
        }
        return array;
    }

    inline std::string newId() {
        return IdGeneratorUtils::generateGuid();
    }

    inline dto::BaseApiResponse okResponse(const std::string &message,
                                           const Json::Value &result = Json::Value()) {
        dto::BaseApiResponse response;
        response.success = true;
        response.message = message;
        response.result = result;
        return response;
    }

    inline dto::BaseApiResponse errorResponse(const std::string &message, int code = 0) {
        dto::BaseApiResponse response;
        response.success = false;
        response.message = message;
        response.error["message"] = message;
        response.error["code"] = code;
        return response;
    }

    inline std::string jsonString(const Json::Value &json, const char *key,
                                  const std::string &fallback = "") {
        if (json.isObject() && json.isMember(key) && !json[key].isNull()) {
            if (json[key].isString()) {
                return json[key].asString();
            }
            return Json::writeString(Json::StreamWriterBuilder(), json[key]);
        }
        return fallback;
    }

    inline int64_t jsonInt64(const Json::Value &json, const char *key, int64_t fallback = 0) {
        if (json.isObject() && json.isMember(key) && !json[key].isNull()) {
            if (json[key].isString()) {
                try { return std::stoll(json[key].asString()); } catch (...) { return fallback; }
            }
            return json[key].asInt64();
        }
        return fallback;
    }

    inline double jsonDouble(const Json::Value &json, const char *key, double fallback = 0.0) {
        if (json.isObject() && json.isMember(key) && !json[key].isNull()) {
            if (json[key].isString()) {
                try { return std::stod(json[key].asString()); } catch (...) { return fallback; }
            }
            return json[key].asDouble();
        }
        return fallback;
    }

    inline bool jsonBool(const Json::Value &json, const char *key, bool fallback = false) {
        if (json.isObject() && json.isMember(key) && !json[key].isNull()) {
            if (json[key].isString()) {
                const auto v = json[key].asString();
                return v == "true" || v == "TRUE" || v == "1" || v == "yes";
            }
            return json[key].asBool();
        }
        return fallback;
    }

    inline std::string toJsonString(const Json::Value &value) {
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        return Json::writeString(writer, value);
    }

    inline Json::Value parseJson(const std::string &text, bool *ok = nullptr) {
        Json::Value value;
        Json::CharReaderBuilder builder;
        std::string errors;
        std::istringstream stream(text);
        const bool parsed = Json::parseFromStream(builder, stream, &value, &errors);
        if (ok != nullptr) {
            *ok = parsed;
        }
        return parsed ? value : Json::Value(Json::objectValue);
    }

    inline std::string lowerCopy(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }

    inline std::string trimCopy(const std::string &value) {
        const auto begin = value.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos) return "";
        const auto end = value.find_last_not_of(" \t\r\n");
        return value.substr(begin, end - begin + 1);
    }

} // namespace data_support::utils

#endif //DATASUPPORTAPI_DBUTILS_H
