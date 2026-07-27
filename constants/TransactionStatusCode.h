//
// Created by Emmanuel Addo-Odame on 25/07/2026.
//

#ifndef DATASUPPORTAPI_TRANSACTIONSTATUSCODE_H
#define DATASUPPORTAPI_TRANSACTIONSTATUSCODE_H


namespace data_support::constants {

    namespace TransactionStatusCode {
        constexpr const char* HUBTEL_TRANSACTION_SUCCESS_CODE = "0000";  // Success indicator for hubtel transactions
        constexpr const char* HUBTEL_TRANSACTION_REQUEST_ACCEPTED = "0001";  // Request accepted
    }
}
#endif //DATASUPPORTAPI_TRANSACTIONSTATUSCODE_H