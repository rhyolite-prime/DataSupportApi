//
// Created by Emmanuel Addo-Odame on 11/08/2026.
//

#include <drogon/utils/coroutine.h>
#include "TurboLedgerService.h"
#include "dto/BaseApiResponse.h"
#include "dto/turbo_ledger/CreateIndividualAccountDto.h"

namespace data_support::services {

    drogon::Task<dto::BaseApiResponse> TurboLedgerService::createIndividualAccount(const dto::CreateIndividualAccountDto &dto) {
        dto::BaseApiResponse res;
        co_return res;

    }

}
