//
// MagmaNet — voucher batches, distribution and redemption.
//

#ifndef DATASUPPORTAPI_MAGMAVOUCHERSERVICE_H
#define DATASUPPORTAPI_MAGMAVOUCHERSERVICE_H

#include <drogon/drogon.h>
#include <json/json.h>

#include "dto/BaseApiResponse.h"
#include "dto/magma_net/CheckoutDto.h"
#include "dto/magma_net/VoucherDto.h"

namespace data_support::services {

    class MagmaSubscriberService;
    class MagmaBillingService;

    class MagmaVoucherService {
    public:
        MagmaVoucherService(MagmaSubscriberService &subscribers, MagmaBillingService &billing);

        drogon::Task<dto::BaseApiResponse> createVoucherBatch(
            const std::string &businessId, const dto::magma_net::VoucherBatchDto &dto);
        drogon::Task<dto::BaseApiResponse> listVoucherBatches(const std::string &businessId,
                                                              int pageNo, int pageSize);
        drogon::Task<dto::BaseApiResponse> getVoucherBatch(const std::string &businessId,
                                                           const std::string &batchId);
        drogon::Task<dto::BaseApiResponse> downloadVoucherBatch(const std::string &businessId,
                                                                const std::string &batchId);
        drogon::Task<dto::BaseApiResponse> assignBatchToAgent(const std::string &businessId,
                                                              const std::string &batchId,
                                                              const std::string &agentId);
        drogon::Task<dto::BaseApiResponse> voidVouchers(const std::string &businessId,
                                                        const std::string &batchId,
                                                        const std::vector<std::string> &voucherIds);
        drogon::Task<dto::BaseApiResponse> getVoucherInventory(const std::string &businessId,
                                                               const std::string &zoneId);
        drogon::Task<dto::BaseApiResponse> redeemVoucher(
            const dto::magma_net::VoucherRedeemDto &dto);

    private:
        MagmaSubscriberService &subscribers_;
        MagmaBillingService &billing_;
    };

} // namespace data_support::services

#endif //DATASUPPORTAPI_MAGMAVOUCHERSERVICE_H
