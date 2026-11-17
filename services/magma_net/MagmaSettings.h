//
// MagmaNet — runtime settings.
//
// Values come from config.json -> custom_config.magma_net, with secrets
// overridable (and preferably supplied) through environment variables.
//

#ifndef DATASUPPORTAPI_MAGMASETTINGS_H
#define DATASUPPORTAPI_MAGMASETTINGS_H

#include <string>

namespace data_support::services {

    struct MagmaSettings {
        // Portal / public URLs
        std::string portalBaseUrl;
        std::string portalOrigin;

        // RADIUS (FreeRADIUS on the VPS, reached over the WireGuard subnet)
        std::string radiusHost;
        int radiusAuthPort = 1812;
        int radiusAcctPort = 1813;
        int radiusCoaPort = 3799;
        std::string radiusSecret;
        std::string radiusSchema = "radius";

        // Secrets
        std::string secretsKey;      // AES-256 key for encrypted columns
        std::string hmacKey;         // HMAC key for voucher hashes and tokens
        std::string voucherPepper;   // extra pepper for voucher code hashes
        std::string internalSecret;  // job / device endpoints

        // RouterOS defaults
        std::string routerOsScheme = "http";
        int routerOsPort = 80;
        int routerOsTimeoutMs = 5000;

        // Jobs
        int usageEnforcementSec = 60;
        int healthSweepSec = 120;
        int reconcilePaymentsSec = 90;
        int invoiceHour = 2;
        bool jobsEnabled = true;

        // Behaviour
        bool sandboxPayments = false;
        bool allowLocalVoucherCache = false;
        int checkoutTtlSeconds = 1800;
        int portalTokenTtlSeconds = 900;
        int otpTtlSeconds = 300;
        int maxVoucherBatch = 10000;
        std::string defaultCurrency = "GHS";
        double vatRate = 0.15;
        double nhilRate = 0.025;
        double getFundRate = 0.025;

        static MagmaSettings &instance();
        void reload();

    private:
        MagmaSettings() = default;
    };

} // namespace data_support::services

#endif //DATASUPPORTAPI_MAGMASETTINGS_H
