//
// Created by Emmanuel Addo-Odame on 26/07/2026.
//

#ifndef DATASUPPORTAPI_HUBTELAPI_H
#define DATASUPPORTAPI_HUBTELAPI_H
#include <drogon/drogon.h>
#include "dto/BaseApiResponse.h"
#include "dto/DirectReceiveMoneyDto.h"

namespace data_support::services {

    class HubtelApi {

    public:

        drogon::Task<dto::TransactionResponse> initializeDirectReceiveMoney(const std::string &businessId, const dto::DirectReceiveMoneyDto &dto);

        drogon::Task<dto::TransactionResponse> initializeWebCheckout(const std::string &businessId, const dto::HubtelWebCheckoutDto &dto);

        drogon::Task<dto::TransactionResponse> transactionStatusCheck(const std::string &businessId, const std::string &paymentReference);

        drogon::Task<dto::TransactionResponse> initializeDirectSendToMomo(const std::string &businessId, const dto::DirectSendMomoDto &dto);

        drogon::Task<dto::TransactionResponse> initializeDirectSendToBank(const std::string &businessId, const dto::DirectSendToBankDto &dto);

        drogon::Task<dto::TransactionResponse> getAccountBalance(const std::string &businessId, const std::string &accountType);

        drogon::Task<bool> sendSms(const std::string &phoneNumber, const std::string &clientSecret, const std::string &clientId, const std::string &senderId, const std::string &messageContent);

    };

}

#endif //DATASUPPORTAPI_HUBTELAPI_H