//
// Created by Emmanuel Addo-Odame on 21/07/2026.
//

#ifndef DATASUPPORTAPI_WAECVOUCHERSERVICE_H
#define DATASUPPORTAPI_WAECVOUCHERSERVICE_H
#include <drogon/drogon.h>
#include "dto/BaseApiResponse.h"
#include "dto/waec_voucher/WaecVoucherDto.h"
#include "dto/waec_voucher/WaecMerchantAuthDto.h"

namespace data_support::services {

    class WaecVoucherService {

    public:
        drogon::Task<dto::BaseApiResponse> getAll(const std::string &businessId, int pageNo, int pageSize, const std::string &voucherType, const std::string &query);

        drogon::Task<dto::BaseApiResponse> importVouchers(const std::string &businessId, const dto::WaecVoucherDto &dto);

        drogon::Task<dto::BaseApiResponse> individualVoucherSale(const std::string &businessId, const dto::IndividualWaecVoucherSaleDto &dto);

        drogon::Task<dto::BaseApiResponse> handleBulkVoucherSale(const std::string &businessId, const dto::BulkWaecVoucherSaleDto &dto); // prepares a csv/pdf file of the pns and serial number. the header of the file will be the merchant information

        drogon::Task<dto::BaseApiResponse> generateMerchantAffiliateSaleLink(const std::string &businessId, const std::string &merchantIdentifier, const std::string &pin); // merchantIdentifier can either be a phone number or email. the methods will use regex to deduce if its a phone numer or email

        drogon::Task<dto::BaseApiResponse> handleAffiliateVoucherSale(const std::string &businessId, const dto::AffiliateWaecVoucherSaleDto &dto);

        drogon::Task<dto::BaseApiResponse> createMerchantAccount(const std::string &businessId, const dto::WaecVoucherMerchantDto &dto);

        drogon::Task<dto::BaseApiResponse> resetMerchantAccountPin(const std::string &businessId, const std::string &merchantIdentifier, const std::string &pin); // should probably go through an OTP flow

        drogon::Task<dto::BaseApiResponse> changeMerchantAccountPin(const std::string &businessId, const dto::ChangeMerchantPinDto &dto);

        drogon::Task<dto::BaseApiResponse> updateMerchantAccountEmail(const std::string &businessId, const dto::BuyVoucherDto &dto);

        // Extra Methods
        drogon::Task<dto::BaseApiResponse> getMerchantDashboardStats(const std::string &businessId, const dto::WaecMerchantAuthDto &dto);

        drogon::Task<dto::BaseApiResponse> getMerchantTransactions(const std::string &businessId, const dto::WaecMerchantAuthDto &dto, int pageNo, int pageSize);

        drogon::Task<dto::BaseApiResponse> getMerchantStatement(const std::string &businessId, const dto::WaecMerchantAuthDto &dto, const std::string &startDate, const std::string &endDate);

        drogon::Task<dto::BaseApiResponse> cashOutCommission(const std::string &businessId, const dto::WaecMerchantAuthDto &dto, double amount, const std::string &destinationAccount);

        drogon::Task<dto::BaseApiResponse> getAvailableVoucherCount(const std::string &businessId);
    };

}
#endif //DATASUPPORTAPI_WAECVOUCHERSERVICE_H