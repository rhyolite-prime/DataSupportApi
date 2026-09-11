#include "HubtelCallbackProcessorController.h"
#include "constants/TransactionStatusCode.h"
#include "dto/BaseApiResponse.h"
#include "plugins/DataSupportServicePlugin.h"


Task<HttpResponsePtr> HubtelCallbackProcessorController::handleSmartSuiteDirectReceiveMoney(HttpRequestPtr req, std::string businessId)
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


Task<HttpResponsePtr> HubtelCallbackProcessorController::handleSmartSuiteCheckout(HttpRequestPtr req, std::string businessId)
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


Task<HttpResponsePtr> HubtelCallbackProcessorController::handleSmartSuiteDirectSendMobileMoney(HttpRequestPtr req, std::string businessId)
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


Task<HttpResponsePtr> HubtelCallbackProcessorController::handleSmartSuiteDirectSendToBank(HttpRequestPtr req, std::string businessId)
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

//waec voucher
Task<HttpResponsePtr> HubtelCallbackProcessorController::handleWaecVoucherSaleDirectReceiveMoney(HttpRequestPtr req, std::string businessId, std::string saleType) {
    data_support::dto::BaseApiResponse response;

    auto jsonPtr = req->getJsonObject();

    if (!jsonPtr) {

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
    auto &waecVoucherService = plugin->getWaecVoucherService();

    bool isTransactionSuccessful = responseCode == data_support::constants::TransactionStatusCode::HUBTEL_TRANSACTION_SUCCESS_CODE;


    if (saleType == "single") {
        response = co_await waecVoucherService.completeSingleVoucherSale(businessId, clientReference);
    }

    if (saleType == "bulk") {
        response = co_await waecVoucherService.completeBulkVoucherSale(businessId, clientReference);
    }

    if (saleType == "affiliate") {
        response = co_await waecVoucherService.completeAffiliateVoucherSale(businessId, clientReference);
    }

    auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
    co_return resp;
}



Task<HttpResponsePtr> HubtelCallbackProcessorController::handleWaecVoucherSaleCheckout(HttpRequestPtr req, std::string businessId, std::string saleType)
{
    data_support::dto::BaseApiResponse response;
    auto jsonPtr = req->getJsonObject();

    if (!jsonPtr) {

        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    LOG_DEBUG << "Received checkout callback from_hubtel =>" << jsonPtr->toStyledString();

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &waecVoucherService = plugin->getWaecVoucherService();

    std::string responseCode = (*jsonPtr)["ResponseCode"].asString();
    std::string clientReference = (*jsonPtr)["Data"]["ClientReference"].asString();

    bool isTransactionSuccessful = responseCode == data_support::constants::TransactionStatusCode::HUBTEL_TRANSACTION_SUCCESS_CODE;

    if (saleType == "single") {
        response = co_await waecVoucherService.completeSingleVoucherSale(businessId, clientReference);
    }

    if (saleType == "bulk") {
        response = co_await waecVoucherService.completeBulkVoucherSale(businessId, clientReference);
    }

    if (saleType == "affiliate") {
        response = co_await waecVoucherService.completeAffiliateVoucherSale(businessId, clientReference);
    }

    auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
    co_return resp;
}


Task<HttpResponsePtr> HubtelCallbackProcessorController::handleWaecVoucherMerchantDirectSendMobileMoney(HttpRequestPtr req, std::string businessId)
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
    auto &waecVoucherService = plugin->getWaecVoucherService();

    std::string responseCode = (*jsonPtr)["ResponseCode"].asString();
    std::string clientReference = (*jsonPtr)["Data"]["ClientReference"].asString();
    std::string recipientName = (*jsonPtr)["Data"]["RecipientName"].asString();

    bool isTransactionSuccessful = responseCode == data_support::constants::TransactionStatusCode::HUBTEL_TRANSACTION_SUCCESS_CODE;

    auto result = co_await waecVoucherService.completeMerchantCommissionWithdrawal(businessId, clientReference);

    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());

    co_return resp;

}