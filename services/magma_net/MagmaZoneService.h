//
// MagmaNet — zones, backhaul records, capacity and zone P&L.
//

#ifndef DATASUPPORTAPI_MAGMAZONESERVICE_H
#define DATASUPPORTAPI_MAGMAZONESERVICE_H

#include <drogon/drogon.h>

#include "dto/BaseApiResponse.h"
#include "dto/magma_net/OpsDto.h"
#include "dto/magma_net/ZoneDto.h"

namespace data_support::services {

    class MagmaZoneService {
    public:
        drogon::Task<dto::BaseApiResponse> createZone(const std::string &businessId,
                                                      const dto::magma_net::ZoneDto &dto);
        drogon::Task<dto::BaseApiResponse> updateZone(const std::string &businessId,
                                                      const std::string &zoneId,
                                                      const dto::magma_net::ZoneDto &dto);
        drogon::Task<dto::BaseApiResponse> getZone(const std::string &businessId,
                                                   const std::string &zoneId);
        drogon::Task<dto::BaseApiResponse> listZones(const std::string &businessId,
                                                     int pageNo, int pageSize,
                                                     const std::string &status);
        drogon::Task<dto::BaseApiResponse> getZoneHealth(const std::string &businessId,
                                                         const std::string &zoneId);
        drogon::Task<dto::BaseApiResponse> recordZoneCost(const std::string &businessId,
                                                          const dto::magma_net::ZoneCostDto &dto);
        drogon::Task<dto::BaseApiResponse> listZoneCosts(const std::string &businessId,
                                                         const std::string &zoneId,
                                                         const std::string &from,
                                                         const std::string &to);
        drogon::Task<dto::BaseApiResponse> getZonePnl(const std::string &businessId,
                                                      const std::string &zoneId,
                                                      const std::string &from,
                                                      const std::string &to);
        drogon::Task<dto::BaseApiResponse> getCapacityReport(const std::string &businessId);
        drogon::Task<dto::BaseApiResponse> getNetworkOverview(const std::string &businessId);
    };

} // namespace data_support::services

#endif //DATASUPPORTAPI_MAGMAZONESERVICE_H
