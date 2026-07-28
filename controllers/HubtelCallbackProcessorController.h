#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class HubtelCallbackProcessorController : public drogon::HttpController<HubtelCallbackProcessorController>
{
  public:
    static constexpr const char *PREFIX = "/api/v1/hubtel-callback-processor/";
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HubtelCallbackProcessorController::handleDirectReceiveMoney,std::string(PREFIX) + "smart-suite/direct-receive-money/{1}", Post, Options);
    ADD_METHOD_TO(HubtelCallbackProcessorController::handleCheckout,std::string(PREFIX) + "smart-suite/checkout/{1}", Post, Options);
    ADD_METHOD_TO(HubtelCallbackProcessorController::handleDirectSendMobileMoney,std::string(PREFIX) + "smart-suite/direct-send-mobile-money/{1}", Post, Options);
    ADD_METHOD_TO(HubtelCallbackProcessorController::handleDirectSendToBank,std::string(PREFIX) + "smart-suite/direct-send-to-bank/{1}", Post, Options);
    METHOD_LIST_END

    Task<HttpResponsePtr> handleDirectReceiveMoney(HttpRequestPtr req, std::string businessId);
    Task<HttpResponsePtr> handleCheckout(HttpRequestPtr req, std::string businessId);
    Task<HttpResponsePtr> handleDirectSendMobileMoney(HttpRequestPtr req, std::string businessId);
    Task<HttpResponsePtr> handleDirectSendToBank(HttpRequestPtr req, std::string businessId);

};
