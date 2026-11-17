//
// MagmaNet — public captive-portal API.
//

#include "MagmaPortalController.h"

#include "MagmaControllerSupport.h"
#include "dto/magma_net/AgentDto.h"
#include "dto/magma_net/CheckoutDto.h"
#include "plugins/DataSupportServicePlugin.h"

using namespace drogon;
using namespace data_support::magma;

namespace {
    data_support::services::MagmaNetService &magmaService() {
        return app().getPlugin<data_support::plugins::DataSupportServicePlugin>()->getMagmaNetService();
    }

    std::string zoneCodeOf(const HttpRequestPtr &req) {
        return queryValue(req, "zone");
    }
}

Task<HttpResponsePtr> MagmaPortalController::bootstrap(HttpRequestPtr req) {
    auto result = co_await magmaService().getPortalBootstrap(zoneCodeOf(req), queryValue(req, "business"));
    co_return toResponse(result, result.success ? k200OK : k404NotFound);
}

Task<HttpResponsePtr> MagmaPortalController::plans(HttpRequestPtr req) {
    auto bootstrap = co_await magmaService().getPortalBootstrap(zoneCodeOf(req), queryValue(req, "business"));
    if (!bootstrap.success) {
        co_return toResponse(bootstrap, k404NotFound);
    }
    data_support::dto::BaseApiResponse response;
    response.success = true;
    response.message = "Plans";
    response.result = bootstrap.result["plans"];
    co_return toResponse(response);
}

Task<HttpResponsePtr> MagmaPortalController::zoneStatus(HttpRequestPtr req) {
    auto bootstrap = co_await magmaService().getPortalBootstrap(zoneCodeOf(req), queryValue(req, "business"));
    if (!bootstrap.success) {
        co_return toResponse(bootstrap, k404NotFound);
    }
    data_support::dto::BaseApiResponse response;
    response.success = true;
    response.message = "Zone status";
    response.result["zone"] = bootstrap.result["zone"];
    co_return toResponse(response);
}

Task<HttpResponsePtr> MagmaPortalController::checkout(HttpRequestPtr req) {
    const auto json = req->getJsonObject();
    if (!json) {
        co_return jsonGuard();
    }
    auto dto = data_support::dto::magma_net::CheckoutIntentDto::fromJson(*json);
    auto result = co_await magmaService().billing().createCheckoutIntent(dto);
    co_return toResponse(result, result.success ? k200OK : k400BadRequest);
}

Task<HttpResponsePtr> MagmaPortalController::checkoutStatus(HttpRequestPtr req, std::string intentId) {
    auto result = co_await magmaService().billing().getCheckoutStatus(intentId);
    co_return toResponse(result, result.success ? k200OK : k404NotFound);
}

Task<HttpResponsePtr> MagmaPortalController::login(HttpRequestPtr req, std::string intentId) {
    auto result = co_await magmaService().billing().loginClient(intentId);
    co_return toResponse(result, result.success ? k200OK : k409Conflict);
}

Task<HttpResponsePtr> MagmaPortalController::redeemVoucher(HttpRequestPtr req) {
    const auto json = req->getJsonObject();
    if (!json) {
        co_return jsonGuard();
    }
    auto dto = data_support::dto::magma_net::VoucherRedeemDto::fromJson(*json);
    auto result = co_await magmaService().vouchers().redeemVoucher(dto);
    co_return toResponse(result, result.success ? k200OK : k400BadRequest);
}

Task<HttpResponsePtr> MagmaPortalController::createTicket(HttpRequestPtr req) {
    const auto json = req->getJsonObject();
    if (!json) {
        co_return jsonGuard();
    }
    auto bootstrap = co_await magmaService().getPortalBootstrap(zoneCodeOf(req), "");
    if (!bootstrap.success) {
        co_return toResponse(bootstrap, k404NotFound);
    }
    auto body = *json;
    body["category"] = body.get("category", "CONNECTIVITY").asString();
    body["authorType"] = "CUSTOMER";
    auto result = co_await magmaService().ops().createTicket(
        bootstrap.result["business"]["id"].asString(), body);
    co_return toResponse(result, result.success ? k201Created : k400BadRequest);
}

Task<HttpResponsePtr> MagmaPortalController::agentLogin(HttpRequestPtr req) {
    const auto json = req->getJsonObject();
    if (!json) {
        co_return jsonGuard();
    }
    auto bootstrap = co_await magmaService().getPortalBootstrap(zoneCodeOf(req), queryValue(req, "business"));
    if (!bootstrap.success) {
        co_return toResponse(bootstrap, k404NotFound);
    }
    data_support::dto::magma_net::AgentAuthDto dto(*json);
    auto result = co_await magmaService().agents().agentLogin(
        bootstrap.result["business"]["id"].asString(), dto);
    co_return toResponse(result, result.success ? k200OK : k401Unauthorized);
}

Task<HttpResponsePtr> MagmaPortalController::agentPinResetRequest(HttpRequestPtr req) {
    const auto json = bodyJson(req);
    auto bootstrap = co_await magmaService().getPortalBootstrap(zoneCodeOf(req), queryValue(req, "business"));
    if (!bootstrap.success) {
        co_return toResponse(bootstrap, k404NotFound);
    }
    auto result = co_await magmaService().agents().requestAgentPinReset(
        bootstrap.result["business"]["id"].asString(),
        json.get("agentIdentifier", "").asString());
    co_return toResponse(result);
}

Task<HttpResponsePtr> MagmaPortalController::agentPinReset(HttpRequestPtr req) {
    const auto json = req->getJsonObject();
    if (!json) {
        co_return jsonGuard();
    }
    auto bootstrap = co_await magmaService().getPortalBootstrap(zoneCodeOf(req), queryValue(req, "business"));
    if (!bootstrap.success) {
        co_return toResponse(bootstrap, k404NotFound);
    }
    auto dto = data_support::dto::magma_net::AgentPinResetDto::fromJson(*json);
    auto result = co_await magmaService().agents().resetAgentPin(
        bootstrap.result["business"]["id"].asString(), dto);
    co_return toResponse(result, result.success ? k200OK : k400BadRequest);
}
