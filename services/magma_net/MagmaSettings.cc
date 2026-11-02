//
// MagmaNet — runtime settings.
//

#include "MagmaSettings.h"

#include <drogon/drogon.h>

#include <cstdlib>

#include "utils/HmacUtils.h"

namespace data_support::services {

    namespace {
        std::string envOr(const char *name, const std::string &fallback) {
            const char *value = std::getenv(name);
            if (value == nullptr || *value == '\0') {
                return fallback;
            }
            return std::string(value);
        }

        std::string stringField(const Json::Value &json, const char *key,
                                const std::string &fallback = "") {
            if (json.isObject() && json.isMember(key) && json[key].isString()) {
                return json[key].asString();
            }
            return fallback;
        }

        int intField(const Json::Value &json, const char *key, int fallback) {
            if (json.isObject() && json.isMember(key) && json[key].isInt()) {
                return json[key].asInt();
            }
            return fallback;
        }

        bool boolField(const Json::Value &json, const char *key, bool fallback) {
            if (json.isObject() && json.isMember(key) && json[key].isBool()) {
                return json[key].asBool();
            }
            return fallback;
        }

        double doubleField(const Json::Value &json, const char *key, double fallback) {
            if (json.isObject() && json.isMember(key) && json[key].isNumeric()) {
                return json[key].asDouble();
            }
            return fallback;
        }
    }

    MagmaSettings &MagmaSettings::instance() {
        static MagmaSettings settings;
        static bool loaded = false;
        if (!loaded) {
            settings.reload();
            loaded = true;
        }
        return settings;
    }

    void MagmaSettings::reload() {
        const Json::Value &config = drogon::app().getCustomConfig()["magma_net"];
        const Json::Value &radius = config["radius"];
        const Json::Value &routeros = config["routeros"];
        const Json::Value &jobs = config["jobs"];

        portalBaseUrl = stringField(config, "portalBaseUrl", "https://portal.magmanet.gh");
        portalOrigin = stringField(config, "portalOrigin", portalBaseUrl);

        radiusHost = stringField(radius, "host", "127.0.0.1");
        radiusAuthPort = intField(radius, "authPort", 1812);
        radiusAcctPort = intField(radius, "acctPort", 1813);
        radiusCoaPort = intField(radius, "coaPort", 3799);
        radiusSchema = stringField(radius, "schema", "radius");
        radiusSecret = envOr("MAGMA_RADIUS_SECRET", stringField(radius, "secret", ""));

        secretsKey = envOr("MAGMA_SECRETS_KEY", stringField(config, "secretsKey", ""));
        hmacKey = envOr("MAGMA_HMAC_KEY", stringField(config, "hmacKey", ""));
        voucherPepper = envOr("MAGMA_VOUCHER_PEPPER", stringField(config, "voucherPepper", ""));
        internalSecret = envOr("MAGMA_INTERNAL_SECRET", stringField(config, "internalSecret", ""));

        if (secretsKey.size() < 32) {
            secretsKey = utils::HmacUtils::randomHex(24);
            LOG_WARN << "[MagmaNet] MAGMA_SECRETS_KEY not configured; using an ephemeral key. "
                        "Encrypted secrets will not survive a restart.";
        }
        if (hmacKey.empty()) {
            hmacKey = utils::HmacUtils::randomHex(32);
            LOG_WARN << "[MagmaNet] MAGMA_HMAC_KEY not configured; using an ephemeral key.";
        }
        if (voucherPepper.empty()) {
            voucherPepper = hmacKey;
        }
        if (internalSecret.empty()) {
            internalSecret = utils::HmacUtils::randomHex(24);
        }

        routerOsScheme = stringField(routeros, "scheme", "http");
        routerOsPort = intField(routeros, "port", routerOsScheme == "https" ? 443 : 80);
        routerOsTimeoutMs = intField(routeros, "timeoutMs", 5000);

        usageEnforcementSec = intField(jobs, "usageEnforcementSec", 60);
        healthSweepSec = intField(jobs, "healthSweepSec", 120);
        reconcilePaymentsSec = intField(jobs, "reconcilePaymentsSec", 90);
        invoiceHour = intField(jobs, "invoiceHour", 2);
        jobsEnabled = boolField(jobs, "enabled", true);

        sandboxPayments = envOr("MAGMA_SANDBOX_PAYMENTS", "") == "true" ||
                          boolField(config, "sandboxPayments", false);
        allowLocalVoucherCache = boolField(config, "allowLocalVoucherCache", false);
        checkoutTtlSeconds = intField(config, "checkoutTtlSeconds", 1800);
        portalTokenTtlSeconds = intField(config, "portalTokenTtlSeconds", 900);
        otpTtlSeconds = intField(config, "otpTtlSeconds", 300);
        maxVoucherBatch = intField(config, "maxVoucherBatch", 10000);
        defaultCurrency = stringField(config, "currency", "GHS");

        const Json::Value &tax = config["tax"];
        vatRate = doubleField(tax, "vat", 0.15);
        nhilRate = doubleField(tax, "nhil", 0.025);
        getFundRate = doubleField(tax, "getFund", doubleField(tax, "getfund", 0.025));
    }

} // namespace data_support::services
