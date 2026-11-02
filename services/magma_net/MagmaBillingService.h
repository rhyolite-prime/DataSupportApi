//
// MagmaNet — checkout, payment reconciliation, activation and invoicing.
//

#ifndef DATASUPPORTAPI_MAGMABILLINGSERVICE_H
#define DATASUPPORTAPI_MAGMABILLINGSERVICE_H

#include <drogon/drogon.h>
#include <json/json.h>

#include "dto/BaseApiResponse.h"
#include "dto/magma_net/CheckoutDto.h"

namespace data_support::services {

    class MagmaRadiusService;
    class MagmaSubscriberService;
    class MagmaPayService;
    class MagmaSmsService;

    class MagmaBillingService {
    public:
        MagmaBillingService(MagmaRadiusService &radius,
                            MagmaSubscriberService &subscribers,
                            MagmaPayService &payments,
                            MagmaSmsService &sms);

        // --- captive portal -------------------------------------------------
        drogon::Task<dto::BaseApiResponse> createCheckoutIntent(
            const dto::magma_net::CheckoutIntentDto &dto);
        drogon::Task<dto::BaseApiResponse> getCheckoutStatus(const std::string &intentId);
        drogon::Task<dto::BaseApiResponse> completeCheckout(const std::string &intentId,
                                                            const std::string &providerReference,
                                                            const Json::Value &rawCallback);
        drogon::Task<dto::BaseApiResponse> loginClient(const std::string &intentId);
        drogon::Task<dto::BaseApiResponse> loginDevice(const std::string &businessId,
                                                       const std::string &zoneId,
                                                       const std::string &subscriptionId,
                                                       const std::string &mac,
                                                       const std::string &ip);

        // --- provider callbacks --------------------------------------------
        drogon::Task<dto::BaseApiResponse> handleProviderCallback(const std::string &provider,
                                                                  const Json::Value &payload);
        drogon::Task<dto::BaseApiResponse> verifyPayment(const std::string &businessId,
                                                         const std::string &transactionId);
        drogon::Task<dto::BaseApiResponse> refundPayment(const std::string &businessId,
                                                         const std::string &transactionId,
                                                         const std::string &reason);

        // --- subscriptions --------------------------------------------------
        drogon::Task<dto::BaseApiResponse> activateSubscription(const std::string &businessId,
                                                                const std::string &subscriberId,
                                                                const std::string &planId,
                                                                const std::string &zoneId,
                                                                const std::string &paymentReference);
        drogon::Task<dto::BaseApiResponse> extendSubscription(const std::string &businessId,
                                                              const std::string &subscriptionId,
                                                              const std::string &planId);
        drogon::Task<dto::BaseApiResponse> terminateSubscription(const std::string &businessId,
                                                                 const std::string &subscriptionId,
                                                                 const std::string &reason);
        drogon::Task<dto::BaseApiResponse> getSubscription(const std::string &businessId,
                                                           const std::string &subscriptionId);

        // --- finance --------------------------------------------------------
        drogon::Task<dto::BaseApiResponse> listTransactions(const std::string &businessId,
                                                            int pageNo, int pageSize,
                                                            const std::string &status,
                                                            const std::string &from,
                                                            const std::string &to);
        drogon::Task<dto::BaseApiResponse> getRevenueSummary(const std::string &businessId,
                                                             const std::string &from,
                                                             const std::string &to);
        drogon::Task<dto::BaseApiResponse> closeDailyTill(const std::string &businessId,
                                                          const std::string &date);
        drogon::Task<dto::BaseApiResponse> issueInvoices(const std::string &businessId);
        drogon::Task<dto::BaseApiResponse> suspendOverdue(const std::string &businessId);
        drogon::Task<dto::BaseApiResponse> reconcilePendingPayments(const std::string &businessId);

        // --- helpers used by vouchers --------------------------------------
        drogon::Task<std::string> createPaidSubscription(const std::string &businessId,
                                                         const std::string &subscriberId,
                                                         const std::string &planId,
                                                         const std::string &zoneId,
                                                         const std::string &paymentId,
                                                         const std::string &voucherId,
                                                         const std::string &agentId,
                                                         double paidAmount);

    private:
        MagmaRadiusService &radius_;
        MagmaSubscriberService &subscribers_;
        MagmaPayService &payments_;
        MagmaSmsService &sms_;
    };

} // namespace data_support::services

#endif //DATASUPPORTAPI_MAGMABILLINGSERVICE_H
