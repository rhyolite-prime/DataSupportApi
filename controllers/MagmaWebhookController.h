//
// MagmaNet — provider callbacks and device enrolment endpoints.
//

#ifndef DATASUPPORTAPI_MAGMAWEBHOOKCONTROLLER_H
#define DATASUPPORTAPI_MAGMAWEBHOOKCONTROLLER_H

#include <drogon/HttpController.h>

using namespace drogon;

class MagmaWebhookController : public HttpController<MagmaWebhookController> {
public:
    static constexpr const char *PREFIX = "/api/v1/magma/webhooks/";

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(MagmaWebhookController::hubtelCheckout, std::string(PREFIX) + "hubtel/checkout", Post, Options);
    ADD_METHOD_TO(MagmaWebhookController::paystackCheckout, std::string(PREFIX) + "paystack/checkout", Post, Options);
    ADD_METHOD_TO(MagmaWebhookController::hubtelSettlement, std::string(PREFIX) + "hubtel/settlement", Post, Options);
    ADD_METHOD_TO(MagmaWebhookController::deviceHello, std::string(PREFIX) + "device/hello", Post, Options);
    ADD_METHOD_TO(MagmaWebhookController::deviceHeartbeat, std::string(PREFIX) + "device/{1}/heartbeat", Post, Options);
    METHOD_LIST_END

    Task<HttpResponsePtr> hubtelCheckout(HttpRequestPtr req);
    Task<HttpResponsePtr> paystackCheckout(HttpRequestPtr req);
    Task<HttpResponsePtr> hubtelSettlement(HttpRequestPtr req);
    Task<HttpResponsePtr> deviceHello(HttpRequestPtr req);
    Task<HttpResponsePtr> deviceHeartbeat(HttpRequestPtr req, std::string deviceId);
};

#endif //DATASUPPORTAPI_MAGMAWEBHOOKCONTROLLER_H
