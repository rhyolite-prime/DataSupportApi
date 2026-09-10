//
// Created by Emmanuel Addo-Odame on 15/08/2026.
//

#ifndef DATASUPPORTAPI_CREATEINIVIDUALACCOUNTDTO_H
#define DATASUPPORTAPI_CREATEINIVIDUALACCOUNTDTO_H
#include <json/json.h>

namespace data_support::dto {

    class CreateIndividualAccountDto {
    public:
        std::string accountNo;
        std::string externalId;
        int statusEnum = 0;
        int subStatus = 0;
        std::string activationDate;
        std::string officeJoiningDate;
        std::string officeId;
        std::string transferToOfficeId;
        std::string staffId;
        std::string firstname;
        std::string middlename;
        std::string lastname;
        std::string fullname;
        std::string displayName;
        std::string mobileNo;
        bool isStaff = false;
        std::string dateOfBirth;
        std::string genderCvId;
        int legalStructure = 0;
        std::string emailAddress;

        void fromJson(const Json::Value& json) {
            if (json.isMember("accountNo") && !json["accountNo"].isNull()) accountNo = json["accountNo"].asString();
            if (json.isMember("externalId") && !json["externalId"].isNull()) externalId = json["externalId"].asString();
            if (json.isMember("statusEnum") && !json["statusEnum"].isNull()) statusEnum = json["statusEnum"].asInt();
            if (json.isMember("subStatus") && !json["subStatus"].isNull()) subStatus = json["subStatus"].asInt();
            if (json.isMember("activationDate") && !json["activationDate"].isNull()) activationDate = json["activationDate"].asString();
            if (json.isMember("officeJoiningDate") && !json["officeJoiningDate"].isNull()) officeJoiningDate = json["officeJoiningDate"].asString();
            if (json.isMember("officeId") && !json["officeId"].isNull()) officeId = json["officeId"].asString();
            if (json.isMember("transferToOfficeId") && !json["transferToOfficeId"].isNull()) transferToOfficeId = json["transferToOfficeId"].asString();
            if (json.isMember("staffId") && !json["staffId"].isNull()) staffId = json["staffId"].asString();
            if (json.isMember("firstname") && !json["firstname"].isNull()) firstname = json["firstname"].asString();
            if (json.isMember("middlename") && !json["middlename"].isNull()) middlename = json["middlename"].asString();
            if (json.isMember("lastname") && !json["lastname"].isNull()) lastname = json["lastname"].asString();
            if (json.isMember("fullname") && !json["fullname"].isNull()) fullname = json["fullname"].asString();
            if (json.isMember("displayName") && !json["displayName"].isNull()) displayName = json["displayName"].asString();
            if (json.isMember("mobileNo") && !json["mobileNo"].isNull()) mobileNo = json["mobileNo"].asString();
            if (json.isMember("isStaff") && !json["isStaff"].isNull()) isStaff = json["isStaff"].asBool();
            if (json.isMember("genderCvId") && !json["genderCvId"].isNull()) genderCvId = json["genderCvId"].asString();
            if (json.isMember("dateOfBirth") && !json["dateOfBirth"].isNull()) dateOfBirth = json["dateOfBirth"].asString();
            if (json.isMember("legalStructure") && !json["legalStructure"].isNull()) legalStructure = json["legalStructure"].asInt();
            if (json.isMember("emailAddress") && !json["emailAddress"].isNull()) emailAddress = json["emailAddress"].asString();
        }

        [[nodiscard]] Json::Value toJson() const {
            Json::Value json;
            json["accountNo"] = accountNo;
            json["externalId"] = externalId;
            json["statusEnum"] = statusEnum;
            json["subStatus"] = subStatus;
            json["activationDate"] = activationDate;
            json["officeJoiningDate"] = officeJoiningDate;
            json["officeId"] = officeId;
            json["transferToOfficeId"] = transferToOfficeId;
            json["staffId"] = staffId;
            json["firstname"] = firstname;
            json["middlename"] = middlename;
            json["lastname"] = lastname;
            json["fullname"] = fullname;
            json["displayName"] = displayName;
            json["mobileNo"] = mobileNo;
            json["isStaff"] = isStaff;
            json["genderCvId"] = genderCvId;
            json["dateOfBirth"] = dateOfBirth;
            json["legalStructure"] = legalStructure;
            json["emailAddress"] = emailAddress;
            return json;
        }
    };


}
#endif //DATASUPPORTAPI_CREATEINIVIDUALACCOUNTDTO_H
