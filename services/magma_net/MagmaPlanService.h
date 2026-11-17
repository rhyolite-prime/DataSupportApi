//
// MagmaNet — plan catalogue and availability.
//

#ifndef DATASUPPORTAPI_MAGMAPLANSERVICE_H
#define DATASUPPORTAPI_MAGMAPLANSERVICE_H

#include <drogon/drogon.h>

#include "dto/BaseApiResponse.h"
#include "dto/magma_net/DataPlanDto.h"

namespace data_support::services {

    class MagmaPlanService {
    public:
        drogon::Task<dto::BaseApiResponse> createDataPlan(const std::string &businessId,
                                                          const dto::magma_net::DataPlanDto &dto);
        drogon::Task<dto::BaseApiResponse> updateDataPlan(const std::string &businessId,
                                                          const std::string &planId,
                                                          const dto::magma_net::DataPlanDto &dto);
        drogon::Task<dto::BaseApiResponse> deactivateDataPlan(const std::string &businessId,
                                                              const std::string &planId);
        drogon::Task<dto::BaseApiResponse> listDataPlans(const std::string &businessId,
                                                         bool includeInactive);
        drogon::Task<dto::BaseApiResponse> getPublicPlansForZone(const std::string &zoneCode,
                                                                 const std::string &businessId);
        drogon::Task<dto::BaseApiResponse> getPlanAvailability(const std::string &businessId,
                                                               const std::string &planId);
        drogon::Task<dto::BaseApiResponse> simulatePlanChange(const std::string &businessId,
                                                              const std::string &planId,
                                                              const dto::magma_net::DataPlanDto &dto);
    };

} // namespace data_support::services

#endif //DATASUPPORTAPI_MAGMAPLANSERVICE_H
