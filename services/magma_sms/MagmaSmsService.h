//
// MagmaNet — transactional SMS (receipts, OTPs, outages, agent notices).
//

#ifndef DATASUPPORTAPI_MAGMASMSSERVICE_H
#define DATASUPPORTAPI_MAGMASMSSERVICE_H

#include <drogon/drogon.h>

#include <string>

namespace data_support::services {

    class MagmaSmsService {
    public:
        drogon::Task<bool> send(const std::string &businessId,
                                const std::string &msisdn,
                                const std::string &message);

        drogon::Task<bool> sendReceipt(const std::string &businessId,
                                       const std::string &msisdn,
                                       const std::string &planName,
                                       const std::string &allowance,
                                       double amount,
                                       const std::string &reference);

        drogon::Task<bool> sendOtp(const std::string &businessId,
                                   const std::string &msisdn,
                                   const std::string &code);

        drogon::Task<bool> sendOutageNotice(const std::string &businessId,
                                            const std::string &msisdn,
                                            const std::string &zoneName);
    };

} // namespace data_support::services

#endif //DATASUPPORTAPI_MAGMASMSSERVICE_H
