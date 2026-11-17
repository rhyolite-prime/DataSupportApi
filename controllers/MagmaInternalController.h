//
// MagmaNet — internal job endpoints invoked by the Quartz scheduler.
//

#ifndef DATASUPPORTAPI_MAGMAINTERNALCONTROLLER_H
#define DATASUPPORTAPI_MAGMAINTERNALCONTROLLER_H

#include <drogon/HttpController.h>

using namespace drogon;

class MagmaInternalController : public HttpController<MagmaInternalController> {
public:
    static constexpr const char *PREFIX = "/api/v1/magma/internal/";

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(MagmaInternalController::tenMinute, std::string(PREFIX) + "jobs/ten-minute", Post, Get, Options);
    ADD_METHOD_TO(MagmaInternalController::hourly, std::string(PREFIX) + "jobs/hourly", Post, Get, Options);
    ADD_METHOD_TO(MagmaInternalController::daily, std::string(PREFIX) + "jobs/daily", Post, Get, Options);
    ADD_METHOD_TO(MagmaInternalController::registerSchedules, std::string(PREFIX) + "jobs/register-schedules", Post, Get, Options);
    METHOD_LIST_END

    Task<HttpResponsePtr> tenMinute(HttpRequestPtr req);
    Task<HttpResponsePtr> hourly(HttpRequestPtr req);
    Task<HttpResponsePtr> daily(HttpRequestPtr req);
    Task<HttpResponsePtr> registerSchedules(HttpRequestPtr req);
};

#endif //DATASUPPORTAPI_MAGMAINTERNALCONTROLLER_H
