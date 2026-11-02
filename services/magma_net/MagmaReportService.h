//
// MagmaNet — revenue, subscriber and tax reporting.
//

#ifndef DATASUPPORTAPI_MAGMAREPORTSERVICE_H
#define DATASUPPORTAPI_MAGMAREPORTSERVICE_H

#include <drogon/drogon.h>
#include <json/json.h>

#include "dto/BaseApiResponse.h"

namespace data_support::services {

    class MagmaReportService {
    public:
        drogon::Task<dto::BaseApiResponse> getSalesReport(const std::string &businessId,
                                                          const std::string &from,
                                                          const std::string &to,
                                                          const std::string &groupBy);
        drogon::Task<dto::BaseApiResponse> getRevenueSummary(const std::string &businessId,
                                                             const std::string &from,
                                                             const std::string &to);
        drogon::Task<dto::BaseApiResponse> getSubscriberReport(const std::string &businessId,
                                                               int days);
        drogon::Task<dto::BaseApiResponse> getVoucherSalesReport(const std::string &businessId,
                                                                 const std::string &from,
                                                                 const std::string &to);
        drogon::Task<dto::BaseApiResponse> getAgentPerformanceReport(const std::string &businessId,
                                                                     const std::string &from,
                                                                     const std::string &to);
        drogon::Task<dto::BaseApiResponse> getTaxSummary(const std::string &businessId,
                                                         const std::string &from,
                                                         const std::string &to);
        drogon::Task<dto::BaseApiResponse> exportCsv(const std::string &businessId,
                                                     const std::string &reportType,
                                                     const std::string &from,
                                                     const std::string &to);
    };

} // namespace data_support::services

#endif //DATASUPPORTAPI_MAGMAREPORTSERVICE_H
