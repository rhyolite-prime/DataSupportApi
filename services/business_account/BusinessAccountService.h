//
// Created by Emmanuel Addo-Odame on 22/07/2026.
//

#ifndef DATASUPPORTAPI_BUSINESSACCOUNTSERVICE_H
#define DATASUPPORTAPI_BUSINESSACCOUNTSERVICE_H
#include <drogon/drogon.h>

#include "constants/StatusType.h"
#include "dto/BaseApiResponse.h"
#include "dto/BusinessAccountDto.h"

namespace data_support::services {

class BusinessAccountService {

public:
  drogon::Task<dto::BaseApiResponse> getAll(int pageNo, int pageSize,
                                            const std::string &query);

  drogon::Task<dto::BaseApiResponse> create(const dto::BusinessAccountDto &dto);

  drogon::Task<dto::BaseApiResponse> update(const dto::BusinessAccountDto &dto,
                                            const std::string &id);

  drogon::Task<dto::BaseApiResponse> updateAccountStatus(const std::string &id, constants::StatusType status);

  drogon::Task<dto::BaseApiResponse> deleteAccount(const std::string &id);
};
} // namespace data_support::services


#endif // DATASUPPORTAPI_BUSINESSACCOUNTSERVICE_H