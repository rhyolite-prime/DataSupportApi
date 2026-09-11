#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class HubtelCallbackProcessorController : public drogon::HttpController<HubtelCallbackProcessorController>
{
  public:
    static constexpr const char *PREFIX = "/api/v1/hubtel-callback-processor/";
    METHOD_LIST_BEGIN
    //smart suite core banking
    ADD_METHOD_TO(HubtelCallbackProcessorController::handleSmartSuiteDirectReceiveMoney,std::string(PREFIX) + "smart-suite/direct-receive-money/{1}", Post, Options);
    ADD_METHOD_TO(HubtelCallbackProcessorController::handleSmartSuiteCheckout,std::string(PREFIX) + "smart-suite/checkout/{1}", Post, Options);
    ADD_METHOD_TO(HubtelCallbackProcessorController::handleSmartSuiteDirectSendMobileMoney,std::string(PREFIX) + "smart-suite/direct-send-mobile-money/{1}", Post, Options);
    ADD_METHOD_TO(HubtelCallbackProcessorController::handleSmartSuiteDirectSendToBank,std::string(PREFIX) + "smart-suite/direct-send-to-bank/{1}", Post, Options);
    //waec evoucher
    ADD_METHOD_TO(HubtelCallbackProcessorController::handleWaecVoucherSaleDirectReceiveMoney,std::string(PREFIX) + "waec-evoucher-sale/direct-receive-money/{1}/{2}", Post, Options);
    ADD_METHOD_TO(HubtelCallbackProcessorController::handleWaecVoucherSaleCheckout,std::string(PREFIX) + "waec-evoucher-sale/checkout/{1}/{2}", Post, Options);

    ADD_METHOD_TO(HubtelCallbackProcessorController::handleWaecVoucherMerchantDirectSendMobileMoney,std::string(PREFIX) + "waec-evoucher-commissions/direct-send-mobile-money/{1}", Post, Options);

    METHOD_LIST_END

    Task<HttpResponsePtr> handleSmartSuiteDirectReceiveMoney(HttpRequestPtr req, std::string businessId);
    Task<HttpResponsePtr> handleSmartSuiteCheckout(HttpRequestPtr req, std::string businessId);
    Task<HttpResponsePtr> handleSmartSuiteDirectSendMobileMoney(HttpRequestPtr req, std::string businessId);
    Task<HttpResponsePtr> handleSmartSuiteDirectSendToBank(HttpRequestPtr req, std::string businessId);

    //waec evoucher
    Task<HttpResponsePtr> handleWaecVoucherSaleDirectReceiveMoney(HttpRequestPtr req, std::string businessId, std::string saleType);
    Task<HttpResponsePtr> handleWaecVoucherSaleCheckout(HttpRequestPtr req, std::string businessId, std::string saleType);

    Task<HttpResponsePtr> handleWaecVoucherMerchantDirectSendMobileMoney(HttpRequestPtr req, std::string accountId);

};
