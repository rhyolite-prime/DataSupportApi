//
// Created by Emmanuel Addo-Odame on 23/07/2026.
//

#include "QuartzApi.h"
#include <drogon/HttpClient.h>
#include <drogon/utils/coroutine.h>

namespace data_support::services {

    drogon::Task<bool> QuartzApi::scheduleJob(const dto::QuartzJobDto &dto) {

        LOG_INFO << "Scheduling Job: " << dto.name;

        auto &app = drogon::app();
        auto customConfig = app.getCustomConfig();
        std::string baseUrl =
            customConfig["QuartzSchedulerApi"]["BaseUrl"].asString();

        auto client = drogon::HttpClient::newHttpClient(baseUrl);

        auto req = drogon::HttpRequest::newHttpRequest();
        req->setPath("/api/jobs");
        req->setMethod(drogon::Post);
        req->setContentTypeCode(drogon::CT_APPLICATION_JSON);

        // Convert DTO to JSON
        Json::Value json = dto.toJson();
        req->setBody(json.toStyledString());

        try {
            auto resp = co_await client->sendRequestCoro(req);

            if (resp->getStatusCode() != drogon::k200OK &&
                resp->getStatusCode() != drogon::k201Created) {
                LOG_ERROR << "Failed to schedule job. Status: " << resp->getStatusCode()
                          << " Body: " << resp->getBody();
                co_return false;
                } else {
                    LOG_INFO << "Job scheduled successfully. Response: " << resp->getBody();
                    co_return true;
                }

        } catch (const std::exception &e) {
            LOG_ERROR << "Exception scheduling job: " << e.what();
            co_return false;
        }
    }

}