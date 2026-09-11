//
// Created by Emmanuel Addo-Odame on 21/07/2026.
//

#ifndef DATASUPPORTAPI_WAECVOUCHERSERVICE_H
#define DATASUPPORTAPI_WAECVOUCHERSERVICE_H
#include <drogon/drogon.h>
#include "dto/BaseApiResponse.h"
#include "dto/waec_voucher/WaecMerchantAuthDto.h"

namespace data_support::services {

    class WaecVoucherService {

    public:
        drogon::Task<dto::BaseApiResponse> getAll(const std::string &businessId, int pageNo, int pageSize, const std::string &voucherType, const std::string &query);

        drogon::Task<dto::BaseApiResponse> importVouchers(const std::string &businessId, const dto::waec_voucher::WaecVoucherDto &dto);

        drogon::Task<dto::BaseApiResponse> initializeSingleVoucherSale(const std::string &businessId, const dto::waec_voucher::SingleWaecVoucherSaleDto &dto);

        drogon::Task<dto::BaseApiResponse> completeSingleVoucherSale(const std::string &accountId, const std::string &paymentReference);

        drogon::Task<dto::BaseApiResponse> initializeBulkVoucherSale(const std::string &businessId, const dto::waec_voucher::BulkWaecVoucherSaleDto &dto); // prepares a csv/pdf file of the pns and serial number. the header of the file will be the merchant information

        drogon::Task<dto::BaseApiResponse> completeBulkVoucherSale(const std::string &accountId, const std::string &paymentReference);

        drogon::Task<dto::BaseApiResponse> generateMerchantAffiliateSaleLink(const std::string &businessId, const std::string &merchantIdentifier, const std::string &pin); // merchantIdentifier can either be a phone number or email. the methods will use regex to deduce if its a phone numer or email

        drogon::Task<dto::BaseApiResponse> initializeAffiliateVoucherSale(const std::string &businessId, const dto::waec_voucher::AffiliateWaecVoucherSaleDto &dto);

        drogon::Task<dto::BaseApiResponse> completeAffiliateVoucherSale(const std::string &accountId, const std::string &paymentReference);

        drogon::Task<dto::BaseApiResponse> createMerchantAccount(const std::string &businessId, const dto::waec_voucher::WaecMerchantDto &dto);

        drogon::Task<dto::BaseApiResponse> initializeResetMerchantAccountPin(const std::string &businessId, const std::string &merchantIdentifier); // should probably go through an OTP flow

        drogon::Task<dto::BaseApiResponse> resetMerchantAccountPin(const std::string &businessId, const dto::waec_voucher::ResetMerchantPinDto &dto);

        drogon::Task<dto::BaseApiResponse> changeMerchantAccountPin(const std::string &businessId, const dto::waec_voucher::ChangeMerchantPinDto &dto);

        drogon::Task<dto::BaseApiResponse> updateMerchantAccountEmail(const std::string &businessId, const dto::waec_voucher::UpdateMerchantEmailDto &dto);

        drogon::Task<dto::BaseApiResponse> updateMerchantSettlementAccount(const std::string &businessId, const dto::waec_voucher::UpdateMerchantSettlementAccountDto &dto);

        // Extra Methods
        drogon::Task<dto::BaseApiResponse> getMerchantSummaryStats(const std::string &businessId, const dto::waec_voucher::WaecMerchantAuthDto &dto);

        drogon::Task<dto::BaseApiResponse> getMerchantTransactions(const std::string &businessId, const dto::waec_voucher::MerchantTransactionsRequestDto &dto);

        drogon::Task<dto::BaseApiResponse> getMerchantStatement(const std::string &businessId, const dto::waec_voucher::MerchantStatementRequestDto &dto);

        drogon::Task<dto::BaseApiResponse> initializeMerchantCommissionWithdrawal(const std::string &businessId, const dto::waec_voucher::CashoutMerchantCommissionDto &dto);

        drogon::Task<dto::BaseApiResponse> completeMerchantCommissionWithdrawal(const std::string &businessId, const std::string &reference);

        drogon::Task<dto::BaseApiResponse> getAvailableVoucherCount(const std::string &businessId);
    };

}
#endif //DATASUPPORTAPI_WAECVOUCHERSERVICE_H