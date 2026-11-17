//
// MagmaNet — subscribers and their client devices.
//

#ifndef DATASUPPORTAPI_MAGMASUBSCRIBERSERVICE_H
#define DATASUPPORTAPI_MAGMASUBSCRIBERSERVICE_H

#include <drogon/drogon.h>

#include "dto/BaseApiResponse.h"
#include "dto/magma_net/SubscriberDto.h"

namespace data_support::services {

    class MagmaSubscriberService {
    public:
        drogon::Task<dto::BaseApiResponse> createSubscriber(const std::string &businessId,
                                                            const dto::magma_net::SubscriberDto &dto);
        drogon::Task<dto::BaseApiResponse> updateSubscriber(const std::string &businessId,
                                                            const std::string &subscriberId,
                                                            const dto::magma_net::SubscriberDto &dto);
        drogon::Task<dto::BaseApiResponse> getSubscriber(const std::string &businessId,
                                                         const std::string &subscriberId);
        drogon::Task<dto::BaseApiResponse> listSubscribers(const std::string &businessId,
                                                           int pageNo, int pageSize,
                                                           const std::string &query,
                                                           const std::string &status);
        drogon::Task<dto::BaseApiResponse> listClientDevices(const std::string &businessId,
                                                             const std::string &subscriberId);
        drogon::Task<dto::BaseApiResponse> blockClientDevice(const std::string &businessId,
                                                             const std::string &mac,
                                                             const std::string &reason);
        drogon::Task<dto::BaseApiResponse> unblockClientDevice(const std::string &businessId,
                                                               const std::string &mac);
        drogon::Task<dto::BaseApiResponse> getSubscriberUsage(const std::string &businessId,
                                                              const std::string &subscriberId);
        drogon::Task<dto::BaseApiResponse> getSubscriberSessions(const std::string &businessId,
                                                                 const std::string &subscriberId,
                                                                 int pageNo, int pageSize);
        drogon::Task<dto::BaseApiResponse> suspendSubscriber(const std::string &businessId,
                                                             const std::string &subscriberId,
                                                             const std::string &reason);
        drogon::Task<dto::BaseApiResponse> resumeSubscriber(const std::string &businessId,
                                                            const std::string &subscriberId);
        drogon::Task<dto::BaseApiResponse> churnSubscriber(const std::string &businessId,
                                                           const std::string &subscriberId);

        // Internal helpers used by billing, vouchers and the portal.
        drogon::Task<std::string> ensureSubscriber(const std::string &businessId,
                                                   const std::string &msisdn,
                                                   const std::string &fullName,
                                                   const std::string &email,
                                                   const std::string &zoneId);
        drogon::Task<std::string> findSubscriberIdByMsisdn(const std::string &businessId,
                                                           const std::string &msisdn);
        drogon::Task<std::string> findSubscriberIdByMac(const std::string &businessId,
                                                        const std::string &mac);
        drogon::Task<> bindDevice(const std::string &businessId,
                                  const std::string &subscriberId,
                                  const std::string &mac);
        drogon::Task<bool> isDeviceBlocked(const std::string &businessId, const std::string &mac);
    };

} // namespace data_support::services

#endif //DATASUPPORTAPI_MAGMASUBSCRIBERSERVICE_H
