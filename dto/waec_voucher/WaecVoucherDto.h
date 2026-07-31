//
// Created by Antigravity on 23/07/2026.
//

#ifndef DATASUPPORTAPI_WAECVOUCHERDTO_H
#define DATASUPPORTAPI_WAECVOUCHERDTO_H

#include "dto/waec_voucher/WaecMerchantAuthDto.h"
#include <string>
#include <vector>

namespace data_support::dto {

    class WaecVoucherDto {
    public:
        std::string batchId;
        std::vector<std::string> serialNumbers;
        std::vector<std::string> pins;
        double retailPrice{0.0};
        double wholesalePrice{0.0};
    };

    class IndividualWaecVoucherSaleDto {
    public:
        std::string buyerPhone;
        std::string buyerEmail;
        int quantity{1};
        std::string paymentReference; // e.g. momo ref
        double amountPaid{0.0};
    };

    class BulkWaecVoucherSaleDto : public WaecMerchantAuthDto {
    public:
        int quantity{0};
        std::string paymentReference;
        double amountPaid{0.0};
    };

    class AffiliateWaecVoucherSaleDto : public WaecMerchantAuthDto {
    public:
        std::string buyerPhone;
        std::string buyerEmail;
        int quantity{1};
        std::string paymentReference;
        double amountPaid{0.0};
    };

    class WaecVoucherMerchantDto {
    public:
        std::string merchantName;
        std::string phoneNumber;
        std::string email;
        std::string pin;
        bool isAffiliate{false};
    };

    class ChangeMerchantPinDto : public WaecMerchantAuthDto {
    public:
        std::string newPin;
    };

    class BuyVoucherDto : public WaecMerchantAuthDto {
    public:
        std::string newEmail; // used for updateMerchantAccountEmail
    };

}
#endif //DATASUPPORTAPI_WAECVOUCHERDTO_H