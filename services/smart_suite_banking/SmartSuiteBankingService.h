//
// Created by Emmanuel Addo-Odame on 21/07/2026.
//

#ifndef DATASUPPORTAPI_SMARTSUITEBANKINGSERVICE_H
#define DATASUPPORTAPI_SMARTSUITEBANKINGSERVICE_H
#include <drogon/drogon.h>
#include "dto/BaseApiResponse.h"
#include "dto/smart_suite/AccountBalanceDto.h"
#include "dto/smart_suite/ChangePinDto.h"
#include "dto/smart_suite/FlexyWithdrawalToBankDto.h"
#include "dto/smart_suite/FlexyWithdrawalToMomoDto.h"
#include "dto/smart_suite/InterAccountTransferDto.h"
#include "dto/smart_suite/KiddyWithdrawalRequestDto.h"
#include "dto/smart_suite/MiniStatementDto.h"
#include "dto/smart_suite/SavingsWithdrawalRequestDto.h"
#include "dto/smart_suite/SmartSuiteOperationResponse.h"
#include "dto/smart_suite/UssdDepositDto.h"
#include "dto/smart_suite/WebCheckoutDepositDto.h"

namespace data_support::services {

    class SmartSuiteBankingService {

        public:

        drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> verifyCustomerAccount(const std::string &businessId, const std::string &phoneNo);

        drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> changePin(const std::string &businessId, const dto::smart_suite::ChangePinDto &dto);

        drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> getMiniStatement(const std::string &businessId, const dto::smart_suite::MiniStatementDto &dto);

        drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> getAccountBalance(const std::string &businessId, const dto::smart_suite::AccountBalanceDto &dto);

        drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> initializeDepositViaUssd(const std::string &businessId, const dto::smart_suite::UssdDepositDto &dto);

        drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> initializeDepositViaWebCheckout(const std::string &businessId, const dto::smart_suite::WebCheckoutDepositDto &dto);

        drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> initializeFlexyWithdrawalToBank(const std::string &businessId, const dto::smart_suite::FlexyWithdrawalToBankDto &dto);

        drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> initializeFlexyWithdrawalToMomo(const std::string &businessId, const dto::smart_suite::FlexyWithdrawalToMomoDto &dto);

        drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> completeFlexyWithdrawalToBank(const std::string &businessId, const std::string &reference, const std::string &recipientName, bool isSuccessful);

        drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> completeFlexyWithdrawalToMomo(const std::string &businessId, const std::string &reference, const std::string &recipientName, bool isSuccessful);

        drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> completeDepositViaWebCheckout(const std::string &businessId, const std::string &reference, bool isSuccessful);

        drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> completeDeposit(const std::string &businessId, const std::string &reference, const std::string &recipientName, bool isSuccessful);

        drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> reverseTransaction(const std::string &businessId, const std::string &reference);

        // withdrawal request...

        drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> kiddyWithdrawalRequest(const std::string &businessId, const dto::smart_suite::KiddyWithdrawalRequestDto &dto);

        drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> savingsWithdrawalRequest(const std::string &businessId, const dto::smart_suite::SavingsWithdrawalRequestDto &dto);

        drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> flexyToKiddyTransfer(const std::string &businessId, const dto::smart_suite::KiddyWithdrawalRequestDto &dto);

        drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> interAccountTransfer(const std::string &businessId, const dto::smart_suite::InterAccountTransferDto &dto);

        // audit ...

        //drogon::Task<dto::BaseApiResponse> performAudit(const std::string &businessId, const std::string &reference);

        //drogon::Task<dto::BaseApiResponse> performDailyAudit(const std::string &businessId);



    };
}
#endif //DATASUPPORTAPI_SMARTSUITEBANKINGSERVICE_H