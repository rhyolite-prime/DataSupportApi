//
// Created by Emmanuel Addo-Odame on 21/07/2026.
//

#ifndef DATASUPPORTAPI_WAECVOUCHERSERVICE_H
#define DATASUPPORTAPI_WAECVOUCHERSERVICE_H
#include <drogon/drogon.h>
#include "dto/BaseApiResponse.h"

namespace data_support::services {

    class WaecVoucherService {

    public:
        // drogon::Task<dto::BaseApiResponse> getAll(const std::string &businessId, int pageNo, int pageSize, const std::string &query);
        //
        // drogon::Task<dto::BaseApiResponse> importVouchers(const std::string &businessId, const dto::WaecVoucherDto &dto);
        //
        // drogon::Task<dto::BaseApiResponse> individualVoucherSale(const std::string &businessId, const dto::IndividualWaecVoucherSaleDto &dto);
        //
        // drogon::Task<dto::BaseApiResponse> handleBulkVoucherSale(const std::string &businessId, const dto::BulkWaecVoucherSaleDto &dto);
        //
        // drogon::Task<dto::BaseApiResponse> handleAffiliateVoucherSale(const std::string &businessId, const dto::AffiliateWaecVoucherSaleDto &dto);
        //
        // drogon::Task<dto::BaseApiResponse> createMerchantAccount(const std::string &businessId, const dto::WaecVoucherMerchantDto &dto);
        //
        // drogon::Task<dto::BaseApiResponse> resetMerchantAccountPin(const std::string &businessId, const dto::BuyVoucherDto &dto);
        //
        // drogon::Task<dto::BaseApiResponse> changeMerchantAccountPin(const std::string &businessId, const dto::WaecMerchantAccountDto &dto);
        //
        // drogon::Task<dto::BaseApiResponse> updateMerchantAccountEmail(const std::string &businessId, const dto::BuyVoucherDto &dto);
    };

}
#endif //DATASUPPORTAPI_WAECVOUCHERSERVICE_H