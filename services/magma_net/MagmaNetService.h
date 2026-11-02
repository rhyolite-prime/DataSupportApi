//
// MagmaNet — control-plane facade. Owns every MagmaNet service so a single
// instance (held by DataSupportServicePlugin) wires the whole subsystem.
//

#ifndef DATASUPPORTAPI_MAGMANETSERVICE_H
#define DATASUPPORTAPI_MAGMANETSERVICE_H

#include <drogon/drogon.h>
#include <json/json.h>

#include <string>

#include "dto/BaseApiResponse.h"
#include "services/quartz/QuartzApi.h"

#include "MagmaAgentService.h"
#include "MagmaBillingService.h"
#include "MagmaDeviceService.h"
#include "MagmaMonitoringService.h"
#include "MagmaOpsService.h"
#include "MagmaPlanService.h"
#include "MagmaRadiusService.h"
#include "MagmaReportService.h"
#include "MagmaSubscriberService.h"
#include "MagmaVoucherService.h"
#include "MagmaZoneService.h"
#include "services/magma_pay/MagmaPayService.h"
#include "services/magma_sms/MagmaSmsService.h"

namespace data_support::services {

    class MagmaNetService {
    public:
        MagmaNetService();

        MagmaZoneService &zones() { return zones_; }
        MagmaPlanService &plans() { return plans_; }
        MagmaSubscriberService &subscribers() { return subscribers_; }
        MagmaRadiusService &radius() { return radius_; }
        MagmaBillingService &billing() { return billing_; }
        MagmaVoucherService &vouchers() { return vouchers_; }
        MagmaAgentService &agents() { return agents_; }
        MagmaDeviceService &devices() { return devices_; }
        MagmaMonitoringService &monitoring() { return monitoring_; }
        MagmaReportService &reports() { return reports_; }
        MagmaOpsService &ops() { return ops_; }
        MagmaPayService &payments() { return payments_; }
        MagmaSmsService &sms() { return sms_; }

        // Public portal bootstrap: zone, branding, plans, service status.
        drogon::Task<dto::BaseApiResponse> getPortalBootstrap(const std::string &zoneCode,
                                                              const std::string &businessId);
        drogon::Task<dto::BaseApiResponse> getAdminOverview(const std::string &businessId);

        // Job entry points (invoked by the Quartz callbacks below).
        drogon::Task<dto::BaseApiResponse> runTenMinuteJobs();
        drogon::Task<dto::BaseApiResponse> runHourlyJobs();
        drogon::Task<dto::BaseApiResponse> runDailyJobs();
        drogon::Task<dto::BaseApiResponse> registerScheduledJobs(QuartzApi &quartz);

    private:
        MagmaSmsService sms_;
        MagmaPayService payments_;
        MagmaRadiusService radius_;
        MagmaSubscriberService subscribers_;
        MagmaBillingService billing_;
        MagmaZoneService zones_;
        MagmaPlanService plans_;
        MagmaVoucherService vouchers_;
        MagmaAgentService agents_;
        MagmaDeviceService devices_;
        MagmaMonitoringService monitoring_;
        MagmaReportService reports_;
        MagmaOpsService ops_;
    };

} // namespace data_support::services

#endif //DATASUPPORTAPI_MAGMANETSERVICE_H
