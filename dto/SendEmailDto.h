//
// Created by Emmanuel Addo-Odame on 23/08/2026.
//

#ifndef DATASUPPORTAPI_SENDEMAILDTO_H
#define DATASUPPORTAPI_SENDEMAILDTO_H

#include <json/json.h>
#include <string>

namespace data_support::dto {

    class SendEmailDto {

    public:

        SendEmailDto() = default;

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const std::string& getTo() const { return to_; }
        [[nodiscard]] const std::string& getSubject() const { return subject_; }
        [[nodiscard]] const std::string& getBody() const { return body_; }

        // Setters
        void setTo(const std::string& to) { to_ = to; }
        void setSubject(const std::string& subject) { subject_ = subject; }
        void setBody(const std::string& body) { body_ = body; }

    private:

        std::string to_;
        std::string subject_;
        std::string body_;

    };


    inline void SendEmailDto::fromJson(const Json::Value& json) {

        if (json.isMember("To") && !json["To"].isNull()) {
            to_ = json["To"].asString();
        }

        if (json.isMember("Subject") && !json["Subject"].isNull()) {
            subject_ = json["Subject"].asString();
        }

        if (json.isMember("Body") && !json["Body"].isNull()) {
            body_ = json["Body"].asString();
        }

    }

}

#endif //DATASUPPORTAPI_SENDEMAILDTO_H
