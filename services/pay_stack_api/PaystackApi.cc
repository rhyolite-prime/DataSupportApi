//
// Created by Emmanuel Addo-Odame on 09/09/2026.
//
#include "PaystackApi.h"

#include <drogon/orm/CoroMapper.h>

#include "BusinessAccounts.h"
#include "dto/DirectReceiveMoneyDto.h"

namespace data_support::services {


    drogon::Task<dto::TransactionResponse> PaystackApi::initializeWebCheckout(const std::string &businessId, const dto::PaystackWebCheckoutDto &dto) {

        dto::TransactionResponse response;

        try {
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
                response.clientReference = dto.getClientReference();
                response.code = 1;
                response.message = "Failed to initiate web_checkout. Configuration error.";
                co_return response;
            }

            const auto &paystackConfig = configJson["PaystackApi"];
            if (paystackConfig.isNull()) {
                response.clientReference = dto.getClientReference();
                response.code = 1;
                response.message = "Failed to initiate web_checkout. Hubtel configuration missing.";
                co_return response;
            }

            std::string apiKey = paystackConfig["ApiKey"].asString();

            Json::Value payload;
            payload["amount"] = dto.getTotalAmount();
            payload["email"] = dto.getEmail();
            payload["callback_url"] = dto.getCallbackUrl() ;
            payload["reference"] = dto.getClientReference();

            Json::StreamWriterBuilder writer;
            writer["indentation"] = "";
            std::string body = Json::writeString(writer, payload);


            auto client = drogon::HttpClient::newHttpClient("https://api.paystack.co");
            auto req = drogon::HttpRequest::newHttpRequest();
            req->setMethod(drogon::Post);
            req->setPath("/transaction/initialize");
            req->setContentTypeCode(drogon::CT_APPLICATION_JSON);
            req->setBody(body);
            req->addHeader("Authorization", "Bearer "+apiKey);

            auto httpResp = co_await client->sendRequestCoro(req);

            if (httpResp && (httpResp->statusCode() == drogon::k200OK || httpResp->statusCode() == drogon::k201Created)) {
                const std::string responseStr = std::string(httpResp->getBody());

                Json::Value jsonResponse;
                std::istringstream respIss(responseStr);
                if (Json::parseFromStream(readerBuilder, respIss, &jsonResponse, &errs)) {

                    auto result = dto::PaystackWebCheckoutResponse::fromJson(jsonResponse);

                    LOG_INFO << "InitializeCheckout response => " << responseStr << ", reference: " << dto.getClientReference();

                    response.clientReference = dto.getClientReference();
                    response.code = result.status ? 0 : 1;
                    response.checkoutUrl = result.data.authorizationUrl;
                    response.message = result.status ? "Web Checkout initialized successfully." : "Failed to initialize transaction.";
                    co_return response;
                }
            }

            response.clientReference = dto.getClientReference();
            response.code = 1;
            response.checkoutUrl = "";
            response.message = "Failed to initiate web_checkout check. Please try again.";
            co_return response;

        } catch (const std::exception &e) {
            LOG_ERROR << "Error! Failed to initiate Paystack web_checkout. " << e.what();
            response.clientReference = "";
            response.code = 1;
            response.checkoutUrl = "";
            response.message = "Failed to initiate Paystack web_checkout. Please try again.";
            co_return response;
        }

    }


  drogon::Task<dto::TransactionResponse> PaystackApi::transactionStatusCheck(const std::string &businessId, const std::string &paymentReference) {
    dto::TransactionResponse response;
    response.clientReference = paymentReference; // Use the payment reference as client reference for tracking

    try {
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
            response.code = 1;
            response.message = "Failed to verify transaction. Configuration error.";
            co_return response;
        }

        const auto &paystackConfig = configJson["PaystackApi"];
        if (paystackConfig.isNull()) {
            response.code = 1;
            response.message = "Failed to verify transaction. Paystack configuration missing.";
            co_return response;
        }

        std::string apiKey = paystackConfig["ApiKey"].asString();
        if (apiKey.empty()) {
            response.code = 1;
            response.message = "Failed to verify transaction. API key missing.";
            co_return response;
        }

        // Build the verification URL
        std::string verifyPath = "/transaction/verify/" + paymentReference;

        auto client = drogon::HttpClient::newHttpClient("https://api.paystack.co");
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Get);
        req->setPath(verifyPath);
        req->addHeader("Authorization", "Bearer " + apiKey);
        // No body needed for GET

        auto httpResp = co_await client->sendRequestCoro(req);

        if (httpResp && (httpResp->statusCode() == drogon::k200OK || httpResp->statusCode() == drogon::k201Created)) {
            const std::string responseStr = std::string(httpResp->getBody());

            Json::Value jsonResponse;
            std::istringstream respIss(responseStr);
            if (Json::parseFromStream(readerBuilder, respIss, &jsonResponse, &errs)) {

                auto result = dto::PaystackTransactionStatusResponse::fromJson(jsonResponse);

                LOG_INFO << "TransactionStatusCheck response => " << responseStr << ", reference: " << paymentReference;

                response.code = result.data.status == "success" ? 0 : 1;
                response.message = result.message;
                co_return response;
            }
        }

        LOG_INFO << "PaystackTransactionStatusCheck response => " << (httpResp ? httpResp->getBody() : "no response") << ", reference: " << paymentReference;
        response.code = 1;
        response.message = "Failed to initiate transaction status check. Please try again.";
        co_return response;

    } catch (const std::exception &e) {
        LOG_ERROR << "Error during Paystack transaction verification: " << e.what();
        response.code = 1;
        response.message = "An internal error occurred while verifying the transaction.";
        co_return response;
    }
 }

}
