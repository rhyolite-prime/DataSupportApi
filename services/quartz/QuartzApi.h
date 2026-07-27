//
// Created by Emmanuel Addo-Odame on 23/07/2026.
//

#ifndef DATASUPPORTAPI_QUARTZAPI_H
#define DATASUPPORTAPI_QUARTZAPI_H
#include "dto/QuartzJobDto.h"
#include <drogon/drogon.h>

namespace data_support::services {

    class QuartzApi {
    public:
        drogon::Task<bool> scheduleJob(const dto::QuartzJobDto &dto);
    };

}
#endif //DATASUPPORTAPI_QUARTZAPI_H