//
// Created by Emmanuel Addo-Odame on 09/09/2026.
//

#ifndef DATASUPPORTAPI_PAYSTACKAPI_H
#define DATASUPPORTAPI_PAYSTACKAPI_H
#include <drogon/drogon.h>
#include "dto/DirectReceiveMoneyDto.h"


namespace data_support::services {

    class PaystackApi {

    public:

        drogon::Task<dto::TransactionResponse> initializeWebCheckout(const std::string &businessId, const dto::PaystackWebCheckoutDto &dto);

        drogon::Task<dto::TransactionResponse> transactionStatusCheck(const std::string &businessId, const std::string &paymentReference);


    };

}
#endif //DATASUPPORTAPI_PAYSTACKAPI_H
