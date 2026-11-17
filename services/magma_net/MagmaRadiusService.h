//
// MagmaNet — RADIUS projection, accounting ingestion and enforcement.
//

#ifndef DATASUPPORTAPI_MAGMARADIUSSERVICE_H
#define DATASUPPORTAPI_MAGMARADIUSSERVICE_H

#include <drogon/drogon.h>

#include "dto/BaseApiResponse.h"

namespace data_support::services {

    class MagmaRadiusService {
    public:
        /** Write (or refresh) radcheck/radreply/radusergroup for one subscription. */
        drogon::Task<dto::BaseApiResponse> projectSubscription(const std::string &businessId,
                                                               const std::string &subscriptionId);

        /** Remove a user's credentials (expiry, suspension, refund). */
        drogon::Task<dto::BaseApiResponse> removeSubscription(const std::string &businessId,
                                                              const std::string &subscriptionId);

        /** Pull radacct into magma_sessions and recompute usage totals. */
        drogon::Task<dto::BaseApiResponse> syncUsage();

        /** Expire/deplete plans, apply fair-use throttles, disconnect offenders. */
        drogon::Task<dto::BaseApiResponse> enforceLimits();

        /** Disconnect every live session for a subscription. */
        drogon::Task<int> disconnectSubscription(const std::string &businessId,
                                                 const std::string &subscriptionId);

        /** Register/refresh the NAS row for a zone gateway. */
        drogon::Task<dto::BaseApiResponse> syncNas(const std::string &deviceId);

        drogon::Task<dto::BaseApiResponse> getActiveSessions(const std::string &businessId,
                                                             const std::string &zoneId);
        drogon::Task<dto::BaseApiResponse> getAccountingSummary(const std::string &businessId,
                                                                const std::string &from,
                                                                const std::string &to);
    };

} // namespace data_support::services

#endif //DATASUPPORTAPI_MAGMARADIUSSERVICE_H
