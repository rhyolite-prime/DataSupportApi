#include "HubtelCallbackProcessorController.h"
#include "constants/TransactionStatusCode.h"
#include "dto/BaseApiResponse.h"
#include "plugins/DataSupportServicePlugin.h"


Task<HttpResponsePtr> HubtelCallbackProcessorController::handleDirectReceiveMoney(HttpRequestPtr req, std::string businessId)
{

    auto jsonPtr = req->getJsonObject();

    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    LOG_DEBUG << "Received Direct Receive Money callback from_hubtel =>" << jsonPtr->toStyledString();

    std::string responseCode = (*jsonPtr)["ResponseCode"].asString();
    std::string clientReference = (*jsonPtr)["Data"]["ClientReference"].asString();
    std::string recipientName = (*jsonPtr)["Data"]["RecipientName"].asString();

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &smartSuiteBankingService = plugin->getSmartSuiteBankingService();

    bool isTransactionSuccessful = responseCode == data_support::constants::TransactionStatusCode::HUBTEL_TRANSACTION_SUCCESS_CODE;

    auto result = co_await smartSuiteBankingService.completeDeposit(businessId, clientReference, recipientName, isTransactionSuccessful);

    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}


Task<HttpResponsePtr> HubtelCallbackProcessorController::handleCheckout(HttpRequestPtr req, std::string businessId)
{

    auto jsonPtr = req->getJsonObject();

    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    LOG_DEBUG << "Received checkout callback from_hubtel =>" << jsonPtr->toStyledString();

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &smartSuiteBankingService = plugin->getSmartSuiteBankingService();

    std::string responseCode = (*jsonPtr)["ResponseCode"].asString();
    std::string clientReference = (*jsonPtr)["Data"]["ClientReference"].asString();

    bool isTransactionSuccessful = responseCode == data_support::constants::TransactionStatusCode::HUBTEL_TRANSACTION_SUCCESS_CODE;

    auto result = co_await smartSuiteBankingService.completeDeposit(businessId, clientReference,"N/A", isTransactionSuccessful);

    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;
}


Task<HttpResponsePtr> HubtelCallbackProcessorController::handleDirectSendMobileMoney(HttpRequestPtr req, std::string businessId)
{
    auto jsonPtr = req->getJsonObject();

    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    LOG_DEBUG << "Direct Send Money callback from_hubtel => " << jsonPtr->toStyledString();

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &smartSuiteBankingService = plugin->getSmartSuiteBankingService();

    std::string responseCode = (*jsonPtr)["ResponseCode"].asString();
    std::string clientReference = (*jsonPtr)["Data"]["ClientReference"].asString();
    std::string recipientName = (*jsonPtr)["Data"]["RecipientName"].asString();

    bool isTransactionSuccessful = responseCode == data_support::constants::TransactionStatusCode::HUBTEL_TRANSACTION_SUCCESS_CODE;

    auto result = co_await smartSuiteBankingService.completeFlexyWithdrawalToMomo(businessId, clientReference, recipientName, isTransactionSuccessful);

    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());

    co_return resp;


}


Task<HttpResponsePtr> HubtelCallbackProcessorController::handleDirectSendToBank(HttpRequestPtr req, std::string businessId)
{

    auto jsonPtr = req->getJsonObject();

    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &smartSuiteBankingService = plugin->getSmartSuiteBankingService();

    std::string responseCode = (*jsonPtr)["ResponseCode"].asString();
    std::string clientReference = (*jsonPtr)["Data"]["ClientReference"].asString();
    std::string recipientName = (*jsonPtr)["Data"]["RecipientName"].asString();

    LOG_DEBUG << "Direct Send To Bank callback from_hubtel => " << jsonPtr->toStyledString();

    bool isTransactionSuccessful = responseCode == data_support::constants::TransactionStatusCode::HUBTEL_TRANSACTION_SUCCESS_CODE;

    auto result = co_await smartSuiteBankingService.completeFlexyWithdrawalToBank(businessId, clientReference, recipientName, isTransactionSuccessful);

    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());

    co_return resp;

}

