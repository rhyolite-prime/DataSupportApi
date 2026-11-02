//
// MagmaNet — checkout, payment reconciliation, activation and invoicing.
//

#include "MagmaBillingService.h"

#include <cmath>
#include <iomanip>
#include <sstream>

#include "MagmaRadiusService.h"
#include "MagmaSettings.h"
#include "MagmaSmsService.h"
#include "MagmaSubscriberService.h"
#include "MagmaSupport.h"
#include "services/magma_pay/MagmaPayService.h"
#include "utils/DbUtils.h"
#include "utils/HmacUtils.h"
#include "utils/RouterOsClient.h"

using namespace drogon;
using namespace data_support::utils;

namespace data_support::services {

    namespace {
        struct TaxSplit {
            double total = 0;
            double net = 0;
            double tax = 0;
            Json::Value breakdown = Json::Value(Json::objectValue);
        };

        TaxSplit splitTax(double amount, bool taxInclusive) {
            const auto &settings = MagmaSettings::instance();
            const double rate = settings.vatRate + settings.nhilRate + settings.getFundRate;
            TaxSplit split;
            split.total = amount;
            if (taxInclusive) {
                split.net = amount / (1.0 + rate);
                split.tax = amount - split.net;
            } else {
                split.net = amount;
                split.tax = amount * rate;
                split.total = amount + split.tax;
            }
            split.breakdown["vat"] = settings.vatRate * (taxInclusive ? split.net : amount);
            split.breakdown["nhil"] = settings.nhilRate * (taxInclusive ? split.net : amount);
            split.breakdown["getFund"] = settings.getFundRate * (taxInclusive ? split.net : amount);
            split.breakdown["total_tax"] = split.tax;
            return split;
        }

        std::string reference(const std::string &prefix) {
            return prefix + "-" + HmacUtils::randomHex(6);
        }

        Task<Json::Value> loadIntent(const std::string &intentId) {
            Json::Value result;
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT i.*, z.zone_code, z.name AS zone_name, z.business_id, z.status AS zone_status, "
                "p.plan_code, p.name AS plan_name, p.plan_type, p.duration_minutes, p.data_cap_mb, "
                "p.down_kbps, p.up_kbps, p.device_limit, p.price, p.validity_days, p.tax_inclusive, "
                "host(d.tunnel_ip) AS tunnel_ip, d.rest_scheme, d.rest_port, d.api_username, d.api_secret_ciphertext "
                "FROM magma_checkout_intents i "
                "JOIN magma_zones z ON z.id=i.zone_id "
                "JOIN magma_data_plans p ON p.id=i.plan_id "
                "LEFT JOIN LATERAL (SELECT * FROM magma_devices WHERE zone_id=z.id AND role='GATEWAY' "
                "  ORDER BY (status='ONLINE') DESC NULLS LAST LIMIT 1) d ON true "
                "WHERE i.id=$1", intentId);
            if (!rows.empty()) {
                result = jsonRow(rows[0]);
            }
            co_return result;
        }

        Task<std::string> ensureSubscribedTransaction(const std::string &transactionId,
                                                       const std::string &subscriptionId) {
            try {
                auto db = app().getDbClient();
                co_await db->execSqlCoro(
                    "UPDATE magma_transactions SET status='SUCCESS', paid_at=COALESCE(paid_at, now()), "
                    "subscription_id=$2, updated_at=now() WHERE id=$1", transactionId, subscriptionId);
            } catch (const std::exception &e) {
                LOG_ERROR << "[MagmaNet] transaction finalise error: " << e.what();
            }
            co_return subscriptionId;
        }
    }

    MagmaBillingService::MagmaBillingService(MagmaRadiusService &radius,
                                             MagmaSubscriberService &subscribers,
                                             MagmaPayService &payments,
                                             MagmaSmsService &sms)
        : radius_(radius), subscribers_(subscribers), payments_(payments), sms_(sms) {}

    Task<std::string> MagmaBillingService::createPaidSubscription(
        const std::string &businessId,
        const std::string &subscriberId,
        const std::string &planId,
        const std::string &zoneId,
        const std::string &paymentId,
        const std::string &voucherId,
        const std::string &agentId,
        double paidAmount) {
        try {
            auto db = app().getDbClient();
            auto planRows = co_await db->execSqlCoro(
                "SELECT * FROM magma_data_plans WHERE id=$1 AND business_id=$2", planId, businessId);
            if (planRows.empty()) {
                co_return "";
            }
            const auto &plan = planRows[0];
            const std::string planType = plan["plan_type"].as<std::string>();
            const int durationMinutes = plan["duration_minutes"].isNull()
                                            ? 0 : plan["duration_minutes"].as<int>();
            const int64_t capMb = plan["data_cap_mb"].isNull()
                                      ? 0 : plan["data_cap_mb"].as<int64_t>();
            const int validityDays = plan["validity_days"].as<int>();

            Json::Value snapshot;
            snapshot["plan_id"] = planId;
            snapshot["plan_code"] = plan["plan_code"].as<std::string>();
            snapshot["name"] = plan["name"].as<std::string>();
            snapshot["plan_type"] = planType;
            snapshot["duration_minutes"] = durationMinutes;
            snapshot["data_cap_mb"] = static_cast<int64_t>(capMb);
            snapshot["down_kbps"] = plan["down_kbps"].as<int>();
            snapshot["up_kbps"] = plan["up_kbps"].as<int>();
            snapshot["price"] = plan["price"].as<std::string>();
            snapshot["paid_amount"] = paidAmount;
            snapshot["validity_days"] = validityDays;

            const std::string subscriptionId = newId();
            const std::string radiusUsername = "mg:" + subscriptionId;
            const std::string radiusSecret = HmacUtils::randomHex(24);
            const std::string secretCipher = magma::encryptSecret(radiusSecret);

            auto inserted = co_await db->execSqlCoro(
                "INSERT INTO magma_subscriptions (id, business_id, subscriber_id, plan_id, zone_id, "
                "radius_username, radius_secret_ct, status, purchased_minutes, purchased_mb, "
                "starts_at, expires_at, activated_at, plan_snapshot, payment_id, voucher_id, agent_id) "
                "VALUES ($1,$2,$3,$4,$5,$6, $7, 'ACTIVE', $8,$9, now(), "
                "now() + CASE WHEN $10='TIME' THEN ($11 * interval '1 minute') "
                "             ELSE ($12 * interval '1 day') END, now(), $13, "
                "NULLIF($14,'')::uuid, NULLIF($15,'')::uuid, NULLIF($16,'')::uuid) "
                "RETURNING id, radius_username, expires_at",
                subscriptionId, businessId, subscriberId, planId, zoneId, radiusUsername,
                secretCipher, durationMinutes, capMb, planType, durationMinutes, validityDays,
                toJsonString(snapshot), paymentId, voucherId, agentId);

            co_await db->execSqlCoro(
                "UPDATE magma_subscribers SET last_seen_at=now() WHERE id=$1", subscriberId);

            auto projection = co_await radius_.projectSubscription(businessId, subscriptionId);
            if (!projection.success) {
                Json::Value payload;
                payload["subscription_id"] = subscriptionId;
                co_await magma::enqueueOutbox("RADIUS_PROVISION", subscriptionId, payload);
                LOG_WARN << "[MagmaNet] deferred RADIUS projection for " << subscriptionId;
            }

            int64_t allowanceMinutes = durationMinutes;
            int64_t allowanceMb = capMb;
            (void)allowanceMinutes;
            (void)allowanceMb;

            co_return subscriptionId;
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] createPaidSubscription error: " << e.what();
            co_return "";
        }
    }

    Task<dto::BaseApiResponse> MagmaBillingService::createCheckoutIntent(
        const dto::magma_net::CheckoutIntentDto &dto) {
        if (dto.zoneCode.empty() || dto.planCode.empty() || dto.mac.empty()) {
            co_return errorResponse("zoneCode, planCode and mac are required");
        }
        if (dto.msisdn.empty() && lowerCopy(dto.paymentMethod) != "card") {
            co_return errorResponse("msisdn is required for mobile money payments");
        }

        const bool allowed = co_await magma::rateLimit(
            "checkout:" + dto.mac, 10, 600);
        if (!allowed) {
            co_return errorResponse("Too many checkout attempts. Please wait a few minutes.");
        }

        try {
            auto db = app().getDbClient();

            auto zoneRows = co_await db->execSqlCoro(
                "SELECT id, business_id, zone_code, name, status FROM magma_zones "
                "WHERE zone_code=$1 ORDER BY created_at LIMIT 1", dto.zoneCode);
            if (zoneRows.empty()) {
                co_return errorResponse("Unknown zone");
            }
            const std::string zoneId = zoneRows[0]["id"].as<std::string>();
            const std::string businessId = zoneRows[0]["business_id"].as<std::string>();
            const std::string zoneStatus = zoneRows[0]["status"].as<std::string>();
            if (zoneStatus != "LIVE") {
                co_return errorResponse("This zone is not selling plans right now");
            }

            auto planRows = co_await db->execSqlCoro(
                "SELECT p.* FROM magma_data_plans p "
                "WHERE p.business_id=$1 AND p.plan_code=$2 AND p.is_active AND p.is_public "
                "AND (NOT EXISTS (SELECT 1 FROM magma_plan_zones pz WHERE pz.plan_id=p.id) "
                "     OR EXISTS (SELECT 1 FROM magma_plan_zones pz WHERE pz.plan_id=p.id AND pz.zone_id=$3))",
                businessId, dto.planCode, zoneId);
            if (planRows.empty()) {
                co_return errorResponse("Plan is not available in this zone");
            }
            const std::string planId = planRows[0]["id"].as<std::string>();
            const double price = std::stod(planRows[0]["price"].as<std::string>());

            if (co_await subscribers_.isDeviceBlocked(businessId, dto.mac)) {
                co_return errorResponse("This device has been blocked. Contact support.");
            }

            const std::string subscriberId = co_await subscribers_.ensureSubscriber(
                businessId, dto.msisdn, dto.customerName, dto.email, zoneId);
            if (subscriberId.empty()) {
                co_return errorResponse("Unable to create the customer record");
            }

            const std::string intentId = newId();
            const std::string transactionId = newId();
            const std::string internalReference = reference("MG");
            const std::string provider = MagmaPayService::providerFor(dto.paymentMethod);

            co_await db->execSqlCoro(
                "INSERT INTO magma_checkout_intents (id, business_id, zone_id, plan_id, mac_address, "
                "ip_address, msisdn, subscriber_id, transaction_id, status, expires_at) "
                "VALUES ($1,$2,$3,$4,$5::macaddr, NULLIF($6,'')::inet, NULLIF($7,''), $8,$9,'NEW', "
                "now() + ($10 * interval '1 second'))",
                intentId, businessId, zoneId, planId, dto.mac, dto.ip, dto.msisdn, subscriberId,
                transactionId, MagmaSettings::instance().checkoutTtlSeconds);

            const bool taxInclusive = planRows[0]["tax_inclusive"].isNull()
                                          ? true
                                          : planRows[0]["tax_inclusive"].as<bool>();
            const TaxSplit tax = splitTax(price, taxInclusive);
            co_await db->execSqlCoro(
                "INSERT INTO magma_transactions (id, business_id, subscriber_id, purpose, amount, "
                "tax_amount, net_amount, tax_breakdown, payment_method, provider, internal_reference, "
                "status, payer_msisdn) VALUES ($1,$2,$3,'BUNDLE',$4,$5,$6,$7,$8,$9,$10,'PENDING',$11)",
                transactionId, businessId, subscriberId, price, tax.tax, tax.net,
                toJsonString(tax.breakdown), dto.paymentMethod, provider, internalReference,
                dto.msisdn);

            const std::string callbackUrl =
                MagmaSettings::instance().portalBaseUrl + "/api/v1/magma/webhooks/" +
                lowerCopy(provider) + "/checkout";

            PaymentInitiation payment;
            if (lowerCopy(dto.paymentMethod) == "card") {
                payment = co_await payments_.initiateCard(businessId, dto.email, price,
                                                          internalReference, callbackUrl);
            } else {
                payment = co_await payments_.initiateMomo(businessId, dto.msisdn, dto.customerName,
                                                          dto.paymentMethod, price,
                                                          internalReference, "MagmaNet " +
                                                              planRows[0]["name"].as<std::string>(),
                                                          callbackUrl);
            }

            if (!payment.success) {
                co_await db->execSqlCoro(
                    "UPDATE magma_transactions SET status='FAILED', failure_reason=$2, updated_at=now() "
                    "WHERE id=$1", transactionId, payment.message);
                co_await db->execSqlCoro(
                    "UPDATE magma_checkout_intents SET status='FAILED', updated_at=now() WHERE id=$1",
                    intentId);
                co_return errorResponse(payment.message.empty()
                                            ? "Unable to start the payment"
                                            : payment.message);
            }

            co_await db->execSqlCoro(
                "UPDATE magma_transactions SET provider_reference=$2, updated_at=now() WHERE id=$1",
                transactionId,
                payment.providerReference.empty() ? internalReference : payment.providerReference);
            co_await db->execSqlCoro(
                "UPDATE magma_checkout_intents SET status='AWAITING_PAYMENT', updated_at=now() "
                "WHERE id=$1", intentId);

            Json::Value result;
            result["intentId"] = intentId;
            result["transactionId"] = transactionId;
            result["internalReference"] = internalReference;
            result["amount"] = price;
            result["currency"] = MagmaSettings::instance().defaultCurrency;
            result["plan"] = planRows[0]["plan_code"].as<std::string>();
            result["provider"] = payment.provider;
            result["providerReference"] = payment.providerReference;
            result["instruction"] = payment.instruction;
            result["targetUrl"] = payment.targetUrl;
            result["sandbox"] = MagmaSettings::instance().sandboxPayments;

            // Sandbox: settle immediately so the portal flow can be exercised end to end.
            if (payment.provider == "SANDBOX") {
                auto completed = co_await completeCheckout(intentId, payment.providerReference,
                                                           Json::Value(Json::objectValue));
                if (!completed.success) {
                    co_return completed;
                }
                result["sandbox"] = true;
                result["status"] = "PROVISIONED";
                result["subscription"] = completed.result;
                co_return okResponse("Sandbox payment approved and plan activated", result);
            }

            co_return okResponse("Payment initiated", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] createCheckoutIntent error: " << e.what();
            co_return errorResponse("Unable to start checkout");
        }
    }

    Task<dto::BaseApiResponse> MagmaBillingService::getCheckoutStatus(const std::string &intentId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT i.id, i.status, i.expires_at, i.mac_address, i.login_attempts, "
                "t.status AS payment_status, t.amount, t.provider, t.internal_reference, "
                "s.id AS subscription_id, s.status AS subscription_status, s.expires_at AS subscription_expires, "
                "s.used_mb, s.purchased_mb, s.used_seconds, s.purchased_minutes, p.name AS plan_name, "
                "p.plan_type, z.zone_code "
                "FROM magma_checkout_intents i "
                "LEFT JOIN magma_transactions t ON t.id=i.transaction_id "
                "LEFT JOIN magma_subscriptions s ON s.id=(SELECT id FROM magma_subscriptions "
                "   WHERE subscriber_id=i.subscriber_id AND zone_id=i.zone_id "
                "   ORDER BY created_at DESC LIMIT 1) "
                "JOIN magma_data_plans p ON p.id=i.plan_id "
                "JOIN magma_zones z ON z.id=i.zone_id "
                "WHERE i.id=$1", intentId);
            if (rows.empty()) {
                co_return errorResponse("Checkout not found");
            }
            Json::Value result = jsonRow(rows[0]);
            const std::string status = result.get("status", "").asString();
            result["ready_to_login"] = status == "PROVISIONED";
            co_return okResponse("Checkout status", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getCheckoutStatus error: " << e.what();
            co_return errorResponse("Unable to read checkout status");
        }
    }

    Task<dto::BaseApiResponse> MagmaBillingService::completeCheckout(
        const std::string &intentId, const std::string &providerReference,
        const Json::Value &rawCallback) {
        try {
            auto db = app().getDbClient();

            auto claimed = co_await db->execSqlCoro(
                "UPDATE magma_checkout_intents SET status='PAID', updated_at=now() "
                "WHERE id=$1 AND status IN ('NEW','AWAITING_PAYMENT') RETURNING *", intentId);
            if (claimed.empty()) {
                auto existing = co_await db->execSqlCoro(
                    "SELECT status FROM magma_checkout_intents WHERE id=$1", intentId);
                if (!existing.empty() &&
                    existing[0]["status"].as<std::string>() == "PROVISIONED") {
                    auto status = co_await getCheckoutStatus(intentId);
                    co_return okResponse("Checkout already provisioned", status.result);
                }
                co_return errorResponse("Checkout is not payable in its current state");
            }

            const auto &intent = claimed[0];
            const std::string businessId = intent["business_id"].as<std::string>();
            const std::string zoneId = intent["zone_id"].as<std::string>();
            const std::string planId = intent["plan_id"].as<std::string>();
            const std::string subscriberId = intent["subscriber_id"].as<std::string>();
            const std::string transactionId = intent["transaction_id"].as<std::string>();
            const std::string mac = intent["mac_address"].as<std::string>();

            auto txRows = co_await db->execSqlCoro(
                "SELECT amount FROM magma_transactions WHERE id=$1", transactionId);
            const double amount = txRows.empty()
                                      ? 0
                                      : std::stod(txRows[0]["amount"].as<std::string>());

            const std::string subscriptionId = co_await createPaidSubscription(
                businessId, subscriberId, planId, zoneId, transactionId, "", "", amount);
            if (subscriptionId.empty()) {
                // Keep the intent retryable so a provider callback or the
                // reconciliation job can provision once the fault clears.
                co_await db->execSqlCoro(
                    "UPDATE magma_checkout_intents SET status='AWAITING_PAYMENT', updated_at=now() "
                    "WHERE id=$1", intentId);
                co_return errorResponse("Unable to activate the plan");
            }

            co_await db->execSqlCoro(
                "UPDATE magma_checkout_intents SET status='PROVISIONED', updated_at=now() WHERE id=$1",
                intentId);

            co_await ensureSubscribedTransaction(transactionId, subscriptionId);

            if (!mac.empty()) {
                co_await subscribers_.bindDevice(businessId, subscriberId, mac);
            }

            if (rawCallback.isObject() && !rawCallback.empty()) {
                co_await db->execSqlCoro(
                    "UPDATE magma_transactions SET raw_callback=$2, provider_reference=COALESCE(NULLIF($3,''), provider_reference), "
                    "updated_at=now() WHERE id=$1", transactionId, toJsonString(rawCallback),
                    providerReference);
            }

            auto status = co_await getCheckoutStatus(intentId);
            Json::Value result = status.result;
            result["subscriptionId"] = subscriptionId;

            Json::Value smsPayload;
            smsPayload["subscription_id"] = subscriptionId;
            smsPayload["amount"] = amount;
            smsPayload["msisdn"] = intent["msisdn"].isNull() ? "" : intent["msisdn"].as<std::string>();
            co_await magma::enqueueOutbox("SMS_SEND", subscriptionId, smsPayload);

            co_return okResponse("Payment confirmed and plan activated", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] completeCheckout error: " << e.what();
            co_return errorResponse("Unable to complete checkout");
        }
    }

    Task<dto::BaseApiResponse> MagmaBillingService::loginClient(const std::string &intentId) {
        try {
            auto intent = co_await loadIntent(intentId);
            if (intent.empty()) {
                co_return errorResponse("Checkout not found");
            }
            if (intent.get("status", "").asString() != "PROVISIONED") {
                co_return errorResponse("Payment has not been confirmed yet");
            }
            if (intent["tunnel_ip"].isNull()) {
                co_return errorResponse("No online gateway for this zone");
            }

            auto db = app().getDbClient();
            auto subscription = co_await db->execSqlCoro(
                "SELECT s.id, s.radius_username, s.radius_secret_ct, s.status "
                "FROM magma_subscriptions s WHERE s.subscriber_id=$1 AND s.zone_id=$2 "
                "ORDER BY s.created_at DESC LIMIT 1",
                intent["subscriber_id"].as<std::string>(), intent["zone_id"].as<std::string>());
            if (subscription.empty()) {
                co_return errorResponse("No active subscription found");
            }
            if (subscription[0]["status"].as<std::string>() != "ACTIVE") {
                co_return errorResponse("Subscription is not active");
            }

            RouterOsClient client(
                intent["rest_scheme"].isNull() ? "http" : intent["rest_scheme"].as<std::string>(),
                intent["tunnel_ip"].as<std::string>(),
                intent["rest_port"].isNull() ? 80 : intent["rest_port"].as<int>(),
                intent["api_username"].isNull() ? "" : intent["api_username"].as<std::string>(),
                magma::decryptSecret(intent["api_secret_ciphertext"].isNull()
                                         ? "" : intent["api_secret_ciphertext"].as<std::string>()),
                MagmaSettings::instance().routerOsTimeoutMs / 1000.0);

            const std::string mac = intent["mac_address"].as<std::string>();
            const std::string ip = intent["ip_address"].isNull() ? "" : intent["ip_address"].as<std::string>();

            const bool visible = co_await client.hostTableContains(mac, ip);
            if (!visible) {
                co_await db->execSqlCoro(
                    "UPDATE magma_checkout_intents SET login_attempts=login_attempts+1, updated_at=now() "
                    "WHERE id=$1", intentId);
                co_return errorResponse(
                    "This device is not visible on the network yet. Reconnect to the WiFi and try again.");
            }

            const std::string radiusSecret = magma::decryptSecret(
                subscription[0]["radius_secret_ct"].as<std::string>());
            const std::string username = subscription[0]["radius_username"].as<std::string>();

            auto login = co_await client.loginHotspotClient(ip, mac, username, radiusSecret);
            if (!login.ok) {
                co_return errorResponse("The gateway rejected the login: " + login.error);
            }

            Json::Value result;
            result["username"] = username;
            result["zone"] = intent["zone_code"];
            result["online"] = true;
            co_return okResponse("Client logged in", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] loginClient error: " << e.what();
            co_return errorResponse("Unable to log the client in");
        }
    }

    Task<dto::BaseApiResponse> MagmaBillingService::loginDevice(const std::string &businessId,
                                                                 const std::string &zoneId,
                                                                 const std::string &subscriptionId,
                                                                 const std::string &mac,
                                                                 const std::string &ip) {
        try {
            auto db = app().getDbClient();
            auto gateway = co_await db->execSqlCoro(
                "SELECT host(tunnel_ip) AS tunnel_ip, rest_scheme, rest_port, api_username, api_secret_ciphertext "
                "FROM magma_devices WHERE zone_id=$1 AND role='GATEWAY' "
                "ORDER BY (status='ONLINE') DESC NULLS LAST LIMIT 1", zoneId);
            if (gateway.empty() || gateway[0]["tunnel_ip"].isNull()) {
                co_return errorResponse("No online gateway for this zone");
            }

            auto sub = co_await db->execSqlCoro(
                "SELECT radius_username, radius_secret_ct, status FROM magma_subscriptions "
                "WHERE id=$1 AND business_id=$2", subscriptionId, businessId);
            if (sub.empty() || sub[0]["status"].as<std::string>() != "ACTIVE") {
                co_return errorResponse("Subscription is not active");
            }

            RouterOsClient client(
                gateway[0]["rest_scheme"].isNull() ? "http" : gateway[0]["rest_scheme"].as<std::string>(),
                gateway[0]["tunnel_ip"].as<std::string>(),
                gateway[0]["rest_port"].isNull() ? 80 : gateway[0]["rest_port"].as<int>(),
                gateway[0]["api_username"].isNull() ? "" : gateway[0]["api_username"].as<std::string>(),
                magma::decryptSecret(gateway[0]["api_secret_ciphertext"].isNull()
                                         ? "" : gateway[0]["api_secret_ciphertext"].as<std::string>()),
                MagmaSettings::instance().routerOsTimeoutMs / 1000.0);

            if (!(co_await client.hostTableContains(mac, ip))) {
                co_return errorResponse(
                    "This device is not visible on the network yet. Reconnect to the WiFi and try again.");
            }

            const std::string username = sub[0]["radius_username"].as<std::string>();
            const std::string secret = magma::decryptSecret(sub[0]["radius_secret_ct"].as<std::string>());
            auto login = co_await client.loginHotspotClient(ip, mac, username, secret);
            if (!login.ok) {
                co_return errorResponse("The gateway rejected the login: " + login.error);
            }

            Json::Value result;
            result["username"] = username;
            result["mac"] = mac;
            result["online"] = true;
            co_return okResponse("Client logged in", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] loginDevice error: " << e.what();
            co_return errorResponse("Unable to log the client in");
        }
    }

    Task<dto::BaseApiResponse> MagmaBillingService::handleProviderCallback(
        const std::string &provider, const Json::Value &payload) {
        try {
            std::string referenceValue;
            const char *referenceFields[] = {"clientReference", "ClientReference", "reference",
                                             "Reference", "Data"};
            for (const char *field : referenceFields) {
                const Json::Value candidate = payload.get(field, Json::Value());
                if (candidate.isString()) {
                    referenceValue = candidate.asString();
                    break;
                }
                if (candidate.isObject()) {
                    const Json::Value nested = candidate.get("ClientReference", Json::Value());
                    if (nested.isString()) {
                        referenceValue = nested.asString();
                        break;
                    }
                }
            }
            if (referenceValue.empty()) {
                co_return errorResponse("Callback has no reference");
            }

            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT t.*, i.id AS intent_id FROM magma_transactions t "
                "LEFT JOIN magma_checkout_intents i ON i.transaction_id=t.id "
                "WHERE t.internal_reference=$1 OR t.provider_reference=$1 "
                "ORDER BY t.created_at DESC LIMIT 1", referenceValue);
            if (rows.empty()) {
                co_return errorResponse("Unknown transaction reference");
            }

            const auto &tx = rows[0];
            const std::string transactionId = tx["id"].as<std::string>();
            const std::string businessId = tx["business_id"].as<std::string>();
            const std::string intentId = tx["intent_id"].isNull() ? "" : tx["intent_id"].as<std::string>();
            const std::string internalReference = tx["internal_reference"].as<std::string>();
            const std::string providerReference =
                tx["provider_reference"].isNull() ? internalReference : tx["provider_reference"].as<std::string>();
            const std::string paymentProvider = provider.empty()
                                                    ? (tx["provider"].isNull() ? "HUBTEL" : tx["provider"].as<std::string>())
                                                    : provider;

            if (tx["status"].as<std::string>() == "SUCCESS") {
                co_return okResponse("Transaction already processed", jsonRow(tx));
            }
            if (intentId.empty()) {
                co_return errorResponse("Transaction has no checkout intent");
            }

            auto verification = co_await payments_.verify(businessId, paymentProvider,
                                                          providerReference, internalReference);
            if (!verification.paid) {
                if (!verification.pending) {
                    co_await db->execSqlCoro(
                        "UPDATE magma_transactions SET status='FAILED', failure_reason=$2, "
                        "raw_callback=$3, updated_at=now() WHERE id=$1",
                        transactionId, verification.status, toJsonString(payload));
                }
                co_return errorResponse("Payment is not confirmed yet (" + verification.status + ")");
            }

            const double expected = std::stod(tx["amount"].as<std::string>());
            if (verification.amount > 0 && std::fabs(verification.amount - expected) > 0.5) {
                co_await db->execSqlCoro(
                    "INSERT INTO magma_alerts (business_id, alert_type, severity, message, payload) "
                    "VALUES ($1,'PAYMENT_MISMATCH','CRITICAL',$2,$3)",
                    businessId,
                    "Payment amount mismatch for transaction " + transactionId,
                    toJsonString(payload));
                co_return errorResponse("Payment amount does not match the expected amount");
            }

            co_return co_await completeCheckout(intentId, providerReference, payload);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] handleProviderCallback error: " << e.what();
            co_return errorResponse("Unable to process the payment callback");
        }
    }

    Task<dto::BaseApiResponse> MagmaBillingService::verifyPayment(const std::string &businessId,
                                                                  const std::string &transactionId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT t.*, i.id AS intent_id FROM magma_transactions t "
                "LEFT JOIN magma_checkout_intents i ON i.transaction_id=t.id "
                "WHERE t.id=$1 AND t.business_id=$2", transactionId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Transaction not found");
            }
            const auto &tx = rows[0];
            const std::string provider = tx["provider"].isNull() ? "HUBTEL" : tx["provider"].as<std::string>();
            const std::string providerReference =
                tx["provider_reference"].isNull() ? tx["internal_reference"].as<std::string>()
                                                  : tx["provider_reference"].as<std::string>();

            auto verification = co_await payments_.verify(
                businessId, provider, providerReference, tx["internal_reference"].as<std::string>());

            if (verification.paid && tx["status"].as<std::string>() != "SUCCESS") {
                const std::string intentId = tx["intent_id"].isNull() ? "" : tx["intent_id"].as<std::string>();
                if (!intentId.empty()) {
                    co_await completeCheckout(intentId, providerReference, Json::Value(Json::objectValue));
                }
            }

            Json::Value result;
            result["transaction"] = jsonRow(tx);
            result["verification"] = verification.raw;
            result["paid"] = verification.paid;
            result["status"] = verification.status;
            co_return okResponse("Payment verified", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] verifyPayment error: " << e.what();
            co_return errorResponse("Unable to verify payment");
        }
    }

    Task<dto::BaseApiResponse> MagmaBillingService::refundPayment(const std::string &businessId,
                                                                  const std::string &transactionId,
                                                                  const std::string &reason) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "UPDATE magma_transactions SET status='REVERSED', failure_reason=$3, updated_at=now() "
                "WHERE id=$1 AND business_id=$2 AND status='SUCCESS' RETURNING *",
                transactionId, businessId, reason);
            if (rows.empty()) {
                co_return errorResponse("Refundable transaction not found");
            }

            const std::string subscriptionId = rows[0]["subscription_id"].isNull()
                                                   ? ""
                                                   : rows[0]["subscription_id"].as<std::string>();
            if (!subscriptionId.empty()) {
                co_await db->execSqlCoro(
                    "UPDATE magma_subscriptions SET status='CANCELLED', ended_at=now(), updated_at=now() "
                    "WHERE id=$1", subscriptionId);
                co_await radius_.removeSubscription(businessId, subscriptionId);
            }

            co_await magma::auditLog(businessId, "STAFF", "", "PAYMENT_REFUNDED",
                                     "magma_transactions", transactionId,
                                     Json::Value(reason));

            Json::Value result = jsonRow(rows[0]);
            result["note"] = "Refund recorded and access revoked. Execute the actual reversal in the "
                             "Hubtel/Paystack dashboard if the provider does not auto-reverse.";
            co_return okResponse("Refund recorded", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] refundPayment error: " << e.what();
            co_return errorResponse("Unable to record refund");
        }
    }

    Task<dto::BaseApiResponse> MagmaBillingService::activateSubscription(
        const std::string &businessId, const std::string &subscriberId, const std::string &planId,
        const std::string &zoneId, const std::string &paymentReference) {
        const std::string subscriptionId = co_await createPaidSubscription(
            businessId, subscriberId, planId, zoneId, "", "", "", 0);
        if (subscriptionId.empty()) {
            co_return errorResponse("Unable to activate subscription");
        }
        Json::Value result;
        result["subscriptionId"] = subscriptionId;
        result["paymentReference"] = paymentReference;
        co_return okResponse("Subscription activated", result);
    }

    Task<dto::BaseApiResponse> MagmaBillingService::extendSubscription(
        const std::string &businessId, const std::string &subscriptionId,
        const std::string &planId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT subscriber_id, zone_id, plan_id FROM magma_subscriptions "
                "WHERE id=$1 AND business_id=$2", subscriptionId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Subscription not found");
            }
            const std::string targetPlan = planId.empty()
                                               ? rows[0]["plan_id"].as<std::string>()
                                               : planId;
            const std::string newSubscription = co_await createPaidSubscription(
                businessId, rows[0]["subscriber_id"].as<std::string>(), targetPlan,
                rows[0]["zone_id"].as<std::string>(), "", "", "", 0);

            if (newSubscription.empty()) {
                co_return errorResponse("Unable to extend subscription");
            }
            co_await db->execSqlCoro(
                "UPDATE magma_subscriptions SET parent_id=$2 WHERE id=$1",
                newSubscription, subscriptionId);

            Json::Value result;
            result["subscriptionId"] = newSubscription;
            result["parentSubscriptionId"] = subscriptionId;
            co_return okResponse("Subscription extended", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] extendSubscription error: " << e.what();
            co_return errorResponse("Unable to extend subscription");
        }
    }

    Task<dto::BaseApiResponse> MagmaBillingService::terminateSubscription(
        const std::string &businessId, const std::string &subscriptionId,
        const std::string &reason) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "UPDATE magma_subscriptions SET status='CANCELLED', ended_at=now(), updated_at=now() "
                "WHERE id=$1 AND business_id=$2 AND status IN ('ACTIVE','SUSPENDED','PENDING') "
                "RETURNING id, radius_username", subscriptionId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Active subscription not found");
            }
            co_await radius_.removeSubscription(businessId, subscriptionId);
            co_await magma::auditLog(businessId, "STAFF", "", "SUBSCRIPTION_TERMINATED",
                                     "magma_subscriptions", subscriptionId, Json::Value(reason));
            co_return okResponse("Subscription terminated", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] terminateSubscription error: " << e.what();
            co_return errorResponse("Unable to terminate subscription");
        }
    }

    Task<dto::BaseApiResponse> MagmaBillingService::getSubscription(
        const std::string &businessId, const std::string &subscriptionId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT s.*, p.name AS plan_name, p.plan_type, z.zone_code, sub.msisdn "
                "FROM magma_subscriptions s JOIN magma_data_plans p ON p.id=s.plan_id "
                "JOIN magma_zones z ON z.id=s.zone_id JOIN magma_subscribers sub ON sub.id=s.subscriber_id "
                "WHERE s.id=$1 AND s.business_id=$2", subscriptionId, businessId);
            if (rows.empty()) {
                co_return errorResponse("Subscription not found");
            }
            co_return okResponse("Subscription", jsonRow(rows[0]));
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getSubscription error: " << e.what();
            co_return errorResponse("Unable to load subscription");
        }
    }

    Task<dto::BaseApiResponse> MagmaBillingService::listTransactions(
        const std::string &businessId, int pageNo, int pageSize, const std::string &status,
        const std::string &from, const std::string &to) {
        try {
            auto db = app().getDbClient();
            if (pageNo < 1) pageNo = 1;
            if (pageSize < 1 || pageSize > 200) pageSize = 50;

            auto rows = co_await db->execSqlCoro(
                "SELECT t.id, t.purpose, t.amount, t.tax_amount, t.currency, t.payment_method, "
                "t.provider, t.provider_reference, t.internal_reference, t.status, t.payer_msisdn, "
                "t.initiated_at, t.paid_at, sub.msisdn, z.zone_code, p.name AS plan_name "
                "FROM magma_transactions t "
                "LEFT JOIN magma_subscriptions s ON s.id=t.subscription_id "
                "LEFT JOIN magma_subscribers sub ON sub.id=t.subscriber_id "
                "LEFT JOIN magma_zones z ON z.id=s.zone_id "
                "LEFT JOIN magma_data_plans p ON p.id=s.plan_id "
                "WHERE t.business_id=$1 AND ($2='' OR t.status=$2) "
                "AND t.created_at >= COALESCE(NULLIF($3,'')::timestamptz, now() - interval '90 days') "
                "AND t.created_at <= COALESCE(NULLIF($4,'')::timestamptz, now()) "
                "ORDER BY t.created_at DESC LIMIT $5 OFFSET $6",
                businessId, status, from, to, pageSize, (pageNo - 1) * pageSize);

            auto total = co_await db->execSqlCoro(
                "SELECT COUNT(*) AS count FROM magma_transactions WHERE business_id=$1 "
                "AND ($2='' OR status=$2)", businessId, status);

            Json::Value result;
            result["transactions"] = rowsToJson(rows);
            result["pageNo"] = pageNo;
            result["pageSize"] = pageSize;
            result["total"] = total.empty() ? 0 : total[0]["count"].as<int64_t>();
            co_return okResponse("Transactions", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] listTransactions error: " << e.what();
            co_return errorResponse("Unable to list transactions");
        }
    }

    Task<dto::BaseApiResponse> MagmaBillingService::getRevenueSummary(
        const std::string &businessId, const std::string &from, const std::string &to) {
        try {
            auto db = app().getDbClient();
            auto totals = co_await db->execSqlCoro(
                "SELECT "
                "COALESCE(SUM(amount) FILTER (WHERE created_at >= date_trunc('day', now())),0) AS today, "
                "COALESCE(SUM(amount) FILTER (WHERE created_at >= date_trunc('month', now())),0) AS month, "
                "COUNT(*) FILTER (WHERE status='SUCCESS') AS successful, "
                "COUNT(*) FILTER (WHERE status='FAILED') AS failed, "
                "COALESCE(SUM(amount) FILTER (WHERE status='SUCCESS'),0) AS gross "
                "FROM magma_transactions WHERE business_id=$1 "
                "AND created_at >= COALESCE(NULLIF($2,'')::timestamptz, now() - interval '30 days') "
                "AND created_at <= COALESCE(NULLIF($3,'')::timestamptz, now())", businessId, from, to);

            auto byProvider = co_await db->execSqlCoro(
                "SELECT provider, COUNT(*) AS count, COALESCE(SUM(amount),0) AS total "
                "FROM magma_transactions WHERE business_id=$1 AND status='SUCCESS' "
                "AND created_at >= COALESCE(NULLIF($2,'')::timestamptz, now() - interval '30 days') "
                "GROUP BY provider ORDER BY total DESC", businessId, from);

            auto byZone = co_await db->execSqlCoro(
                "SELECT z.zone_code, COUNT(*) AS count, COALESCE(SUM(t.amount),0) AS total "
                "FROM magma_transactions t JOIN magma_subscriptions s ON s.id=t.subscription_id "
                "JOIN magma_zones z ON z.id=s.zone_id "
                "WHERE t.business_id=$1 AND t.status='SUCCESS' "
                "AND t.created_at >= COALESCE(NULLIF($2,'')::timestamptz, now() - interval '30 days') "
                "GROUP BY z.zone_code ORDER BY total DESC", businessId, from);

            auto daily = co_await db->execSqlCoro(
                "SELECT date_trunc('day', created_at)::date AS day, COUNT(*) AS count, "
                "COALESCE(SUM(amount),0) AS total FROM magma_transactions "
                "WHERE business_id=$1 AND status='SUCCESS' "
                "AND created_at >= COALESCE(NULLIF($2,'')::timestamptz, now() - interval '30 days') "
                "GROUP BY 1 ORDER BY 1", businessId, from);

            Json::Value result;
            result["totals"] = totals.empty() ? Json::Value(Json::objectValue) : jsonRow(totals[0]);
            result["by_provider"] = rowsToJson(byProvider);
            result["by_zone"] = rowsToJson(byZone);
            result["daily"] = rowsToJson(daily);
            co_return okResponse("Revenue summary", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] getRevenueSummary error: " << e.what();
            co_return errorResponse("Unable to build revenue summary");
        }
    }

    Task<dto::BaseApiResponse> MagmaBillingService::closeDailyTill(
        const std::string &businessId, const std::string &date) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT payment_method, provider, status, COUNT(*) AS count, "
                "COALESCE(SUM(amount),0) AS total "
                "FROM magma_transactions WHERE business_id=$1 "
                "AND created_at::date = COALESCE(NULLIF($2,'')::date, now()::date) "
                "GROUP BY payment_method, provider, status ORDER BY payment_method",
                businessId, date);

            auto vouchers = co_await db->execSqlCoro(
                "SELECT COUNT(*) AS count, COALESCE(SUM(price),0) AS total "
                "FROM magma_vouchers WHERE business_id=$1 AND sold_at IS NOT NULL "
                "AND sold_at::date = COALESCE(NULLIF($2,'')::date, now()::date)",
                businessId, date);

            Json::Value result;
            result["date"] = date.empty() ? "today" : date;
            result["by_method"] = rowsToJson(rows);
            result["vouchers"] = vouchers.empty() ? Json::Value(Json::objectValue) : jsonRow(vouchers[0]);
            co_return okResponse("Daily till", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] closeDailyTill error: " << e.what();
            co_return errorResponse("Unable to close the till");
        }
    }

    Task<dto::BaseApiResponse> MagmaBillingService::issueInvoices(const std::string &businessId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT s.id AS subscription_id, s.subscriber_id, sub.msisdn, p.price, p.name, "
                "p.tax_inclusive FROM magma_subscriptions s "
                "JOIN magma_subscribers sub ON sub.id=s.subscriber_id "
                "JOIN magma_data_plans p ON p.id=s.plan_id "
                "WHERE s.business_id=$1 AND s.status='ACTIVE' AND sub.auth_type='PPPOE' "
                "AND NOT EXISTS (SELECT 1 FROM magma_invoices i WHERE i.subscriber_id=s.subscriber_id "
                "  AND i.period_start = date_trunc('month', now())::date)", businessId);

            int created = 0;
            for (const auto &row : rows) {
                const double price = std::stod(row["price"].as<std::string>());
                const bool inclusive = row["tax_inclusive"].as<bool>();
                const TaxSplit tax = splitTax(price, inclusive);

                const std::string invoiceId = newId();
                const std::string invoiceNumber =
                    "INV-" + HmacUtils::randomHex(4) + "-" + std::to_string(++created);

                co_await db->execSqlCoro(
                    "INSERT INTO magma_invoices (id, business_id, subscriber_id, subscription_id, "
                    "invoice_number, period_start, period_end, subtotal, tax_amount, total, status, "
                    "due_at, issued_at) VALUES ($1,$2,$3,$4,$5, date_trunc('month', now())::date, "
                    "(date_trunc('month', now()) + interval '1 month - 1 day')::date, $6,$7,$8,"
                    "'ISSUED', now() + interval '7 days', now())",
                    invoiceId, businessId, row["subscriber_id"].as<std::string>(),
                    row["subscription_id"].as<std::string>(), invoiceNumber,
                    tax.net, tax.tax, tax.total);

                Json::Value smsPayload;
                smsPayload["msisdn"] = row["msisdn"].isNull() ? "" : row["msisdn"].as<std::string>();
                std::ostringstream invoiceMessage;
                invoiceMessage << "MagmaNet invoice " << invoiceNumber << " for GHS "
                               << std::fixed << std::setprecision(2) << tax.total
                               << " is due in 7 days.";
                smsPayload["message"] = invoiceMessage.str();
                co_await magma::enqueueOutbox("SMS_RAW", invoiceId, smsPayload);
            }

            Json::Value result;
            result["invoices_created"] = created;
            co_return okResponse("Invoice run complete", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] issueInvoices error: " << e.what();
            co_return errorResponse("Invoice run failed");
        }
    }

    Task<dto::BaseApiResponse> MagmaBillingService::suspendOverdue(const std::string &businessId) {
        try {
            auto db = app().getDbClient();
            co_await db->execSqlCoro(
                "UPDATE magma_invoices SET status='OVERDUE' "
                "WHERE business_id=$1 AND status='ISSUED' AND due_at < now()::date", businessId);

            auto rows = co_await db->execSqlCoro(
                "SELECT DISTINCT s.id, s.subscriber_id, s.radius_username FROM magma_subscriptions s "
                "JOIN magma_invoices i ON i.subscriber_id=s.subscriber_id "
                "WHERE s.business_id=$1 AND i.status='OVERDUE' AND s.status='ACTIVE'", businessId);

            int suspended = 0;
            for (const auto &row : rows) {
                co_await db->execSqlCoro(
                    "UPDATE magma_subscriptions SET status='SUSPENDED', updated_at=now() WHERE id=$1",
                    row["id"].as<std::string>());
                co_await radius_.removeSubscription(businessId, row["id"].as<std::string>());
                ++suspended;
            }

            Json::Value result;
            result["suspended"] = suspended;
            co_return okResponse("Overdue sweep complete", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] suspendOverdue error: " << e.what();
            co_return errorResponse("Overdue sweep failed");
        }
    }

    Task<dto::BaseApiResponse> MagmaBillingService::reconcilePendingPayments(
        const std::string &businessId) {
        try {
            auto db = app().getDbClient();
            auto rows = co_await db->execSqlCoro(
                "SELECT t.id FROM magma_transactions t "
                "WHERE t.business_id=$1 AND t.status='PENDING' "
                "AND t.initiated_at < now() - interval '2 minutes' "
                "AND t.initiated_at > now() - interval '2 hours' LIMIT 100", businessId);

            int confirmed = 0;
            for (const auto &row : rows) {
                auto result = co_await verifyPayment(businessId, row["id"].as<std::string>());
                if (result.success) {
                    ++confirmed;
                }
            }

            Json::Value result;
            result["checked"] = static_cast<int64_t>(rows.size());
            result["confirmed"] = confirmed;
            co_return okResponse("Pending payments reconciled", result);
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaNet] reconcilePendingPayments error: " << e.what();
            co_return errorResponse("Unable to reconcile payments");
        }
    }

} // namespace data_support::services
