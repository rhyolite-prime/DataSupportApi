//
// MagmaNet — RADIUS CoA / Disconnect client (RFC 5176).
//
// FreeRADIUS handles authentication and accounting; this client is only used
// to kick or re-rate an already-authenticated session the moment a plan is
// exhausted, expires, is suspended, or is upgraded.
//
// The methods are intentionally synchronous UDP calls with a short timeout.
// Call them from a detached worker (see fireAndForget) rather than from a
// Drogon IO thread so a dead NAS cannot stall the event loop.
//

#ifndef DATASUPPORTAPI_RADIUSCLIENT_H
#define DATASUPPORTAPI_RADIUSCLIENT_H

#include <cstdint>
#include <functional>
#include <string>

namespace data_support::utils {

    class RadiusClient {
    public:
        explicit RadiusClient(int timeoutMs = 400);

        /** Send a Disconnect-Request for one session. */
        bool disconnect(const std::string &nasIp,
                        int nasPort,
                        const std::string &secret,
                        const std::string &acctSessionId,
                        const std::string &username);

        /** Send a CoA-Request that reapplies a Mikrotik-Rate-Limit (e.g. fair-use throttle). */
        bool changeRateLimit(const std::string &nasIp,
                             int nasPort,
                             const std::string &secret,
                             const std::string &acctSessionId,
                             const std::string &username,
                             const std::string &rateLimit);

        /** Run a blocking RADIUS operation off the event loop, fire-and-forget. */
        static void fireAndForget(std::function<void()> work);

    private:
        int timeoutMs_;
    };

} // namespace data_support::utils

#endif //DATASUPPORTAPI_RADIUSCLIENT_H
