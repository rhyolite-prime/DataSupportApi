//
// Created by Emmanuel Addo-Odame on 26/07/2026.
//

#ifndef DATASUPPORTAPI_SMARTSUITEAPIRESPONSE_H
#define DATASUPPORTAPI_SMARTSUITEAPIRESPONSE_H
#include <string>
#include <json/json.h>
#include <type_traits>

namespace data_support::dto {

    template<typename T>
    class SmartSuiteApiResponseModel {
    public:
        Json::Value vPhoneno;
        Json::Value vAmntIp;
        Json::Value vTransT;
        Json::Value vActype;
        Json::Value vAccr;
        Json::Value vPin;
        Json::Value vPinNew;
        Json::Value vTransref;
        Json::Value vSerialno;

        std::string savings;
        std::string shares;
        std::string respcode;
        std::string respdesc;
        std::string flexy;
        std::string acname;

        T respdataresults;

        // Getters
        const Json::Value& getVPhoneno() const { return vPhoneno; }
        const Json::Value& getVAmntIp() const { return vAmntIp; }
        const Json::Value& getVTransT() const { return vTransT; }
        const Json::Value& getVActype() const { return vActype; }
        const Json::Value& getVAccr() const { return vAccr; }
        const Json::Value& getVPin() const { return vPin; }
        const Json::Value& getVPinNew() const { return vPinNew; }
        const Json::Value& getVTransref() const { return vTransref; }
        const Json::Value& getVSerialno() const { return vSerialno; }
        
        const std::string& getSavings() const { return savings; }
        const std::string& getShares() const { return shares; }
        const std::string& getRespcode() const { return respcode; }
        const std::string& getRespdesc() const { return respdesc; }
        const std::string& getFlexy() const { return flexy; }
        const std::string& getAcname() const { return acname; }
        
        const T& getRespdataresults() const { return respdataresults; }

        static SmartSuiteApiResponseModel<T> fromJson(const Json::Value& json) {
            SmartSuiteApiResponseModel<T> res;

            if (json.isMember("v_PHONENO")) res.vPhoneno = json["v_PHONENO"];
            if (json.isMember("v_Amnt_ip")) res.vAmntIp = json["v_Amnt_ip"];
            if (json.isMember("v_transT")) res.vTransT = json["v_transT"];
            if (json.isMember("v_ACTYPE")) res.vActype = json["v_ACTYPE"];
            if (json.isMember("v_ACCR")) res.vAccr = json["v_ACCR"];
            if (json.isMember("V_Pin")) res.vPin = json["V_Pin"];
            if (json.isMember("V_PinNew")) res.vPinNew = json["V_PinNew"];
            if (json.isMember("V_TRANSREF")) res.vTransref = json["V_TRANSREF"];
            if (json.isMember("v_SERIALNO")) res.vSerialno = json["v_SERIALNO"];

            if (json.isMember("SAVINGS")) res.savings = json["SAVINGS"].asString();
            if (json.isMember("SHARES")) res.shares = json["SHARES"].asString();
            if (json.isMember("RESPCODE")) res.respcode = json["RESPCODE"].asString();
            if (json.isMember("RESPDESC")) res.respdesc = json["RESPDESC"].asString();
            if (json.isMember("FLEXY")) res.flexy = json["FLEXY"].asString();
            if (json.isMember("ACNAME")) res.acname = json["ACNAME"].asString();

            if (json.isMember("RESPDATARESULTS")) {
                if constexpr (std::is_same_v<T, Json::Value>) {
                    res.respdataresults = json["RESPDATARESULTS"];
                } else if constexpr (std::is_same_v<T, std::string>) {
                    if (json["RESPDATARESULTS"].isString()) {
                        res.respdataresults = json["RESPDATARESULTS"].asString();
                    }
                } else if constexpr (std::is_same_v<T, int>) {
                    if (json["RESPDATARESULTS"].isInt()) {
                        res.respdataresults = json["RESPDATARESULTS"].asInt();
                    }
                } else {
                    // For custom DTOs, assume they implement a static fromJson
                    res.respdataresults = T::fromJson(json["RESPDATARESULTS"]);
                }
            }
            
            return res;
        }
    };

}

#endif //DATASUPPORTAPI_SMARTSUITEAPIRESPONSE_H