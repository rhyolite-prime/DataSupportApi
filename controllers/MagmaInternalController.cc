//
// MagmaNet — internal job endpoints invoked by the Quartz scheduler.
//

#include "MagmaInternalController.h"

#include "MagmaControllerSupport.h"
#include "plugins/DataSupportServicePlugin.h"
#include "services/magma_net/MagmaSettings.h"
#include "utils/DbUtils.h"

using namespace drogon;

namespace {
    data_support::services::MagmaNetService &magmaService() {
        return app().getPlugin<data_support::plugins::DataSupportServicePlugin>()->getMagmaNetService();
    }

    bool internalCaller(const HttpRequestPtr &req) {
        const std::string expected = data_support::services::MagmaSettings::instance().internalSecret;
        if (expected.empty()) {
            return false;
        }
        const std::string supplied = req->getHeader("X-Magma-Internal-Secret");
        return !supplied.empty() && supplied == expected;
    }

    HttpResponsePtr denied() {
        return data_support::magma::toResponse(
            data_support::utils::errorResponse("Invalid internal secret"), k401Unauthorized);
    }
}

Task<HttpResponsePtr> MagmaInternalController::tenMinute(HttpRequestPtr req) {
    if (!internalCaller(req)) co_return denied();
    co_return data_support::magma::toResponse(co_await magmaService().runTenMinuteJobs());
}

Task<HttpResponsePtr> MagmaInternalController::hourly(HttpRequestPtr req) {
    if (!internalCaller(req)) co_return denied();
    co_return data_support::magma::toResponse(co_await magmaService().runHourlyJobs());
}

Task<HttpResponsePtr> MagmaInternalController::daily(HttpRequestPtr req) {
    if (!internalCaller(req)) co_return denied();
    co_return data_support::magma::toResponse(co_await magmaService().runDailyJobs());
}

Task<HttpResponsePtr> MagmaInternalController::registerSchedules(HttpRequestPtr req) {
    if (!internalCaller(req)) co_return denied();
    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    co_return data_support::magma::toResponse(
        co_await magmaService().registerScheduledJobs(plugin->getQuartzApi()));
}
