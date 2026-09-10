//
// Created by Emmanuel Addo-Odame on 23/08/2026.
//

#ifndef DATASUPPORTAPI_EMAILAPI_H
#define DATASUPPORTAPI_EMAILAPI_H
#include "dto/BaseApiResponse.h"
#include "dto/SendEmailDto.h"
#include <drogon/drogon.h>

namespace data_support::services {

    class EmailApi {

    public:

        drogon::Task<dto::BaseApiResponse> sendEmailAsync(const std::string &businessId, const dto::SendEmailDto &dto);

    };

}
#endif //DATASUPPORTAPI_EMAILAPI_H
