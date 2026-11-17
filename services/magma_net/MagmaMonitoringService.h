//
// MagmaNet — alerts, traffic series, SLA and platform health.
//

#ifndef DATASUPPORTAPI_MAGMAMONITORINGSERVICE_H
#define DATASUPPORTAPI_MAGMAMONITORINGSERVICE_H

#include <drogon/drogon.h>
#include <json/json.h>

#include "dto/BaseApiResponse.h"

namespace data_support::services {

    class MagmaMonitoringService {
    public:
        drogon::Task<dto::BaseApiResponse> listAlerts(const std::string &businessId,
                                                      bool openOnly, const std::string &severity,
                                                      int pageNo, int pageSize);
        drogon::Task<dto::BaseApiResponse> getAlertSummary(const std::string &businessId);
        drogon::Task<dto::BaseApiResponse> acknowledgeAlert(const std::string &businessId,
                                                            const std::string &alertId,
                                                            const std::string &staffId);
        drogon::Task<dto::BaseApiResponse> resolveAlert(const std::string &businessId,
                                                        const std::string &alertId);
        drogon::Task<dto::BaseApiResponse> createAlert(const std::string &businessId,
                                                       const std::string &zoneId,
                                                       const std::string &deviceId,
                                                       const std::string &alertType,
                                                       const std::string &severity,
                                                       const std::string &message);
        drogon::Task<dto::BaseApiResponse> getBandwidthSeries(const std::string &businessId,
                                                              const std::string &zoneId,
                                                              int hours);
        drogon::Task<dto::BaseApiResponse> getConcurrencySeries(const std::string &businessId,
                                                                int hours);
        drogon::Task<dto::BaseApiResponse> getSlaReport(const std::string &businessId,
                                                        int days);
        drogon::Task<dto::BaseApiResponse> getOutboxStatus(const std::string &businessId);
        drogon::Task<dto::BaseApiResponse> getSystemHealth(const std::string &businessId);
    };

} // namespace data_support::services

#endif //DATASUPPORTAPI_MAGMAMONITORINGSERVICE_H
