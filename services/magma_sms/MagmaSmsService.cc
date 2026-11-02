//
// MagmaNet — transactional SMS.
//

#include "MagmaSmsService.h"

#include <iomanip>
#include <sstream>

#include "services/magma_net/MagmaSettings.h"
#include "services/magma_net/MagmaSupport.h"

using namespace drogon;
using namespace data_support::utils;

namespace data_support::services {

    Task<bool> MagmaSmsService::send(const std::string &businessId,
                                     const std::string &msisdn,
                                     const std::string &message) {
        if (msisdn.empty() || message.empty()) {
            co_return false;
        }

        if (MagmaSettings::instance().sandboxPayments) {
            LOG_INFO << "[MagmaSms][sandbox] to=" << msisdn << " body=" << message;
            co_return true;
        }

        try {
            auto config = co_await magma::businessConfig(businessId);
            const auto &sms = config["HubtelSmsApi"];
            if (sms.isNull()) {
                LOG_WARN << "[MagmaSms] HubtelSmsApi is not configured for business " << businessId;
                co_return false;
            }
            const std::string clientId = sms.get("ClientId", sms.get("ApiIdOrUsername", "")).asString();
            const std::string clientSecret =
                sms.get("ClientSecret", sms.get("ApiKeyOrPassword", "")).asString();
            const std::string senderId = sms.get("SenderId", "MagmaNet").asString();
            if (clientId.empty() || clientSecret.empty() || senderId.empty()) {
                co_return false;
            }

            auto client = HttpClient::newHttpClient("https://sms.hubtel.com");
            auto req = HttpRequest::newHttpRequest();
            req->setMethod(Get);
            req->setPath("/v1/messages/send");
            req->setParameter("clientsecret", clientSecret);
            req->setParameter("clientid", clientId);
            req->setParameter("from", senderId);
            req->setParameter("to", msisdn);
            req->setParameter("content", message);

            auto response = co_await client->sendRequestCoro(req, 15.0);
            if (response == nullptr) {
                co_return false;
            }
            if (response->getStatusCode() != k201Created &&
                response->getStatusCode() != k200OK) {
                LOG_WARN << "[MagmaSms] send failed: " << response->getStatusCode()
                         << " " << std::string(response->getBody());
                co_return false;
            }
            co_return true;
        } catch (const std::exception &e) {
            LOG_ERROR << "[MagmaSms] send error: " << e.what();
            co_return false;
        }
    }

    Task<bool> MagmaSmsService::sendReceipt(const std::string &businessId,
                                            const std::string &msisdn,
                                            const std::string &planName,
                                            const std::string &allowance,
                                            double amount,
                                            const std::string &reference) {
        std::ostringstream message;
        message << "MagmaNet: " << planName << " (" << allowance << ") activated. GHS "
                << std::fixed << std::setprecision(2) << amount
                << ". Ref " << reference << ". Dial the portal for help.";
        co_return co_await send(businessId, msisdn, message.str());
    }

    Task<bool> MagmaSmsService::sendOtp(const std::string &businessId,
                                        const std::string &msisdn,
                                        const std::string &code) {
        const std::string message = "MagmaNet: your verification code is " + code +
                                    ". It expires in 5 minutes. Never share it.";
        co_return co_await send(businessId, msisdn, message);
    }

    Task<bool> MagmaSmsService::sendOutageNotice(const std::string &businessId,
                                                 const std::string &msisdn,
                                                 const std::string &zoneName) {
        const std::string message = "MagmaNet: we are aware of a service disruption at " +
                                    zoneName + ". Our team is working on it.";
        co_return co_await send(businessId, msisdn, message);
    }

} // namespace data_support::services
