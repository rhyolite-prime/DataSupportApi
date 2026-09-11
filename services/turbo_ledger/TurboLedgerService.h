//
// Created by Emmanuel Addo-Odame on 11/08/2026.
//

#ifndef DATASUPPORTAPI_TURBOLEDGERSERVICE_H
#define DATASUPPORTAPI_TURBOLEDGERSERVICE_H
#include <drogon/drogon.h>
#include "dto/BaseApiResponse.h"
#include "dto/turbo_ledger/CreateIndividualAccountDto.h"
#include <drogon/utils/coroutine.h>

namespace data_support::services {

    class TurboLedgerService {

    public:
        drogon::Task<dto::BaseApiResponse> createIndividualAccount(const dto::CreateIndividualAccountDto &dto);


    };

}
#endif //DATASUPPORTAPI_TURBOLEDGERSERVICE_H
