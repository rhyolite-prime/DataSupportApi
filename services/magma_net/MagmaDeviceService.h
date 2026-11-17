//
// MagmaNet — device enrolment, RouterOS configuration and health polling.
//

#ifndef DATASUPPORTAPI_MAGMADEVICESERVICE_H
#define DATASUPPORTAPI_MAGMADEVICESERVICE_H

#include <drogon/drogon.h>
#include <json/json.h>

#include "dto/BaseApiResponse.h"

namespace data_support::services {

    class MagmaRadiusService;

    class MagmaDeviceService {
    public:
        explicit MagmaDeviceService(MagmaRadiusService &radius);

        drogon::Task<dto::BaseApiResponse> registerDevice(const std::string &businessId,
                                                          const Json::Value &body);
        drogon::Task<dto::BaseApiResponse> listDevices(const std::string &businessId,
                                                       const std::string &zoneId);
        drogon::Task<dto::BaseApiResponse> getDevice(const std::string &businessId,
                                                     const std::string &deviceId);
        drogon::Task<dto::BaseApiResponse> decommissionDevice(const std::string &businessId,
                                                              const std::string &deviceId);
        drogon::Task<dto::BaseApiResponse> pushDeviceConfig(const std::string &businessId,
                                                            const std::string &deviceId);
        drogon::Task<dto::BaseApiResponse> rebootDevice(const std::string &businessId,
                                                        const std::string &deviceId);
        drogon::Task<dto::BaseApiResponse> runDeviceDiagnostics(const std::string &businessId,
                                                                const std::string &deviceId);
        drogon::Task<dto::BaseApiResponse> pollHealth(const std::string &businessId);
        drogon::Task<dto::BaseApiResponse> hello(const Json::Value &body);
        drogon::Task<dto::BaseApiResponse> heartbeat(const std::string &deviceId,
                                                     const Json::Value &body);
        drogon::Task<dto::BaseApiResponse> desiredConfig(const std::string &deviceId);

    private:
        MagmaRadiusService &radius_;
    };

} // namespace data_support::services

#endif //DATASUPPORTAPI_MAGMADEVICESERVICE_H
