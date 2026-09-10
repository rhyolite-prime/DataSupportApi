//
// Created by Emmanuel Addo-Odame on 23/07/2026.
//

#ifndef DATASUPPORTAPI_STATUSTYPES_H
#define DATASUPPORTAPI_STATUSTYPES_H

namespace data_support::constants {

    enum StatusType {
        DRAFT = 0,
        PUBLISHED = 1,
        ARCHIVED = 2,
        DELETED = 3,
        ENABLED = 4,
        DISABLED = 5,
        UPCOMING = 6,
        ONGOING = 7,
        COMPLETED = 8,
        SCHEDULED = 10,
        ACTIVE = 11,
        INACTIVE = 12,
    };

    enum PaymentStatus
    {
        PENDING = 1,
        PROCESSING = 2,
        SUCCESS = 3,
        FAILED = 4,
        CANCELLED = 5,
        Refunded = 6,
        REFUNDED = 7,
        ONHOLD = 8,
        PARTIALLYPAID = 9,
        REVERSE = 10
    };

    enum WaecVoucherType
    {
        BECE = 1,
        WASSCE = 2,
    };

    enum WaecVoucherSaleType
    {
        INDIVIDUAL = 1,
        MERCHANT_BULK = 2,
        MERCHANT_AFFILIATE = 3,
    };

    enum TransactionType
    {
        DEPOSIT = 1,
        WITHDRAWAL = 2,
        ADJUSTMENT = 3,
        REVERSAL = 4,
    };

}


#endif //DATASUPPORTAPI_STATUSTYPES_H