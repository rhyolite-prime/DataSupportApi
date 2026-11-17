//
// MagmaNet — provider callbacks and device enrolment endpoints.
//

#include "MagmaWebhookController.h"

#include "MagmaControllerSupport.h"
#include "plugins/DataSupportServicePlugin.h"
#include "services/magma_net/MagmaSettings.h"
#include "utils/DbUtils.h"

using namespace drogon;
using namespace data_support::magma;

namespace {
    data_support::services::MagmaNetService &magmaService() {
        return app().getPlugin<data_support::plugins::DataSupportServicePlugin>()->getMagmaNetService();
    }

    bool deviceTokenValid(const HttpRequestPtr &req) {
        const std::string token = req->getHeader("X-Magma-Device-Token");
        const std::string expected = data_support::services::MagmaSettings::instance().internalSecret;
        return !expected.empty() && !token.empty() && token == expected;
    }
}

Task<HttpResponsePtr> MagmaWebhookController::hubtelCheckout(HttpRequestPtr req) {
    const auto json = req->getJsonObject();
    if (!json) {
        co_return jsonGuard();
    }
    auto result = co_await magmaService().billing().handleProviderCallback("HUBTEL", *json);
    co_return toResponse(result, result.success ? k200OK : k400BadRequest);
}

Task<HttpResponsePtr> MagmaWebhookController::paystackCheckout(HttpRequestPtr req) {
    const auto json = req->getJsonObject();
    if (!json) {
        co_return jsonGuard();
    }
    auto result = co_await magmaService().billing().handleProviderCallback("PAYSTACK", *json);
    co_return toResponse(result, result.success ? k200OK : k400BadRequest);
}

Task<HttpResponsePtr> MagmaWebhookController::hubtelSettlement(HttpRequestPtr req) {
    const auto json = req->getJsonObject();
    if (!json) {
        co_return jsonGuard();
    }
    std::string reference = json->get("ClientReference", "").asString();
    if (reference.empty()) {
        reference = json->get("clientReference", "").asString();
    }
    if (reference.empty()) {
        reference = json->get("reference", "").asString();
    }
    if (reference.empty()) {
        co_return toResponse(data_support::utils::errorResponse("ClientReference is required"),
                             k400BadRequest);
    }
    auto result = co_await magmaService().agents().completeSettlementByReference(reference);
    co_return toResponse(result, result.success ? k200OK : k400BadRequest);
}

Task<HttpResponsePtr> MagmaWebhookController::deviceHello(HttpRequestPtr req) {
    if (!deviceTokenValid(req)) {
        co_return toResponse(data_support::utils::errorResponse("Invalid device token"),
                             k401Unauthorized);
    }
    auto result = co_await magmaService().devices().hello(bodyJson(req));
    co_return toResponse(result, result.success ? k200OK : k403Forbidden);
}

Task<HttpResponsePtr> MagmaWebhookController::deviceHeartbeat(HttpRequestPtr req,
                                                              std::string deviceId) {
    if (!deviceTokenValid(req)) {
        co_return toResponse(data_support::utils::errorResponse("Invalid device token"),
                             k401Unauthorized);
    }
    auto result = co_await magmaService().devices().heartbeat(deviceId, bodyJson(req));
    co_return toResponse(result, result.success ? k200OK : k400BadRequest);
}
