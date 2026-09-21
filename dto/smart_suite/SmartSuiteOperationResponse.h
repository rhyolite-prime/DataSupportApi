//
// Created by Emmanuel Addo-Odame on 26/07/2026.
//

#ifndef DATASUPPORTAPI_SMARTSUITEOPERATIONRESPONSE_H
#define DATASUPPORTAPI_SMARTSUITEOPERATIONRESPONSE_H

namespace data_support::dto::smart_suite {

    class SmartSuiteOperationResponse {
    public:
        std::string message;
        int code {1};
        bool success {false};
        std::string checkoutUrl;
        std::string transactionReference;
        Json::Value data {Json::nullValue};

        [[nodiscard]]
        Json::Value toJson() const {
            Json::Value json;
            json["checkoutUrl"] = checkoutUrl;
            json["code"] = code;
            json["message"] = message;
            json["success"] = success;
            if (!transactionReference.empty()) {
                json["transactionReference"] = transactionReference;
            }
            if (!data.isNull()) {
                json["data"] = data;
            }
            return json;
        }

    };


}
#endif //DATASUPPORTAPI_SMARTSUITEOPERATIONRESPONSE_H