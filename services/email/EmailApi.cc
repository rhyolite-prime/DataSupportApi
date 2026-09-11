//
// Created by Emmanuel Addo-Odame on 23/08/2026.
//
#include "EmailApi.h"

#include <drogon/orm/CoroMapper.h>

#include "BusinessAccounts.h"

namespace data_support::services {


drogon::Task<dto::BaseApiResponse> EmailApi::sendEmailAsync(const std::string &businessId, const dto::SendEmailDto &dto) {

  dto::BaseApiResponse apiResponse;

  try {
    // call an api to push the email.
    auto client = drogon::HttpClient::newHttpClient("https://dse-api.rhyoliteprime.com");

    auto dbClient = drogon::app().getDbClient();
    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> mapper(dbClient);

    auto businessAccount = co_await mapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "Failed to parse ConfigurationData: " << errs;
      apiResponse.success = false;
      apiResponse.message = "Configuration error. Request Failed !";
      co_return apiResponse;
    }


    const auto &notificationsConfig = configJson["Notifications"];

    if (notificationsConfig.isNull() || !notificationsConfig.isMember("PaymentProviderLowBalanceAlert")) {
      apiResponse.success = false;
      apiResponse.message = "Notifications configuration missing. Request Failed !";
      co_return apiResponse;
    }

    const auto &alertConfig = notificationsConfig["PaymentProviderLowBalanceAlert"];

    // 1. Extract Email Body
    std::string emailBody = alertConfig.get("EmailBody", "").asString();


    // 2. Extract Recipients List into JSON Array
    Json::Value recipientsList = Json::arrayValue;
    if (alertConfig.isMember("Recipients") && alertConfig["Recipients"].isMember("Email")) {
      recipientsList = alertConfig["Recipients"]["Email"];
    }

    auto &app = drogon::app();
    auto customConfig = app.getCustomConfig();
    std::string host = customConfig["MailConfiguration"]["Host"].asString();
    std::string userName = customConfig["MailConfiguration"]["UserName"].asString();
    std::string password = customConfig["MailConfiguration"]["Password"].asString();
    std::string senderName = customConfig["MailConfiguration"]["SenderName"].asString();

    Json::Value jsonBody;
    jsonBody["To"] = "";
    jsonBody["Subject"] = dto.getSubject();
    jsonBody["Body"] = emailBody;
    jsonBody["Host"] = host;
    jsonBody["Attachments"] = Json::arrayValue;
    jsonBody["Port"] = 465;
    jsonBody["EnableSsl"] = true;
    jsonBody["UserName"] = userName;
    jsonBody["Password"] = password;
    jsonBody["SenderName"] = senderName;
    jsonBody["IsBodyHtml"] = true;

    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("/api/services/app/Auxillary/SendEmail");
    req->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    req->setBody(jsonBody.toStyledString());

    auto response = co_await client->sendRequestCoro(req);

    if (response->statusCode() == drogon::k200OK) {
      apiResponse.success = true;
      apiResponse.message = "Email sent successfully";
    } else {
      apiResponse.success = false;
      apiResponse.message = "Failed to send email";
      if (response) {
        apiResponse.error["statusCode"] = response->statusCode();
        apiResponse.error["body"] = std::string(response->body());
      } else {
        apiResponse.error["message"] = "Unknown error";
      }
    }
  } catch (const std::exception &ex) {
    apiResponse.success = false;
    apiResponse.message = "Failed to send email";
    apiResponse.error["message"] = ex.what();
  }

  co_return apiResponse;
}

}
