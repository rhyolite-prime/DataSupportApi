//
// MagmaNet — unified payment gateway facade.
//
// Wraps Hubtel (MoMo collection + payout) and Paystack (card) behind one
// interface so the billing service never talks to a provider directly.
// Credentials are read from business_accounts.configuration_data, matching
// the existing HubtelPaymentApi / PaystackApi blocks.
//

#ifndef DATASUPPORTAPI_MAGMAPAYSERVICE_H
#define DATASUPPORTAPI_MAGMAPAYSERVICE_H

#include <drogon/drogon.h>
#include <json/json.h>

#include <string>

namespace data_support::services {

    struct PaymentInitiation {
        bool success = false;
        std::string provider;            // HUBTEL | PAYSTACK | SANDBOX
        std::string providerReference;
        std::string internalReference;
        std::string instruction;
        std::string targetUrl;
        std::string message;
        double amount = 0;
    };

    struct PaymentVerification {
        bool paid = false;
        bool pending = false;
        std::string status;
        std::string providerReference;
        double amount = 0;
        std::string message;
        Json::Value raw = Json::Value(Json::objectValue);
    };

    class MagmaPayService {
    public:
        drogon::Task<PaymentInitiation> initiateMomo(const std::string &businessId,
                                                     const std::string &msisdn,
                                                     const std::string &customerName,
                                                     const std::string &paymentMethod,
                                                     double amount,
                                                     const std::string &internalReference,
                                                     const std::string &description,
                                                     const std::string &callbackUrl);

        drogon::Task<PaymentInitiation> initiateCard(const std::string &businessId,
                                                     const std::string &email,
                                                     double amount,
                                                     const std::string &internalReference,
                                                     const std::string &callbackUrl);

        drogon::Task<PaymentVerification> verify(const std::string &businessId,
                                                 const std::string &provider,
                                                 const std::string &providerReference,
                                                 const std::string &internalReference);

        drogon::Task<PaymentInitiation> payoutMomo(const std::string &businessId,
                                                   const std::string &msisdn,
                                                   const std::string &recipientName,
                                                   double amount,
                                                   const std::string &internalReference,
                                                   const std::string &description,
                                                   const std::string &callbackUrl,
                                                   const std::string &paymentMethod = "MOMO_MTN");

        static std::string normaliseChannel(const std::string &paymentMethod);
        static std::string providerFor(const std::string &paymentMethod);
    };

} // namespace data_support::services

#endif //DATASUPPORTAPI_MAGMAPAYSERVICE_H
