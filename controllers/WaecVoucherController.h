//
// Created by Antigravity on 24/07/2026.
//

#ifndef DATASUPPORTAPI_WAECVOUCHERCONTROLLER_H
#define DATASUPPORTAPI_WAECVOUCHERCONTROLLER_H
#include <drogon/HttpController.h>
#include "services/waec_evoucher/WaecVoucherService.h"

namespace data_support::controllers {

    class WaecVoucherController : public drogon::HttpController<WaecVoucherController> {
    public:
        METHOD_LIST_BEGIN
            ADD_METHOD_TO(WaecVoucherController::createMerchantAccount, "/api/v1/business/{1}/waec/merchants", drogon::Post);
            ADD_METHOD_TO(WaecVoucherController::individualVoucherSale, "/api/v1/business/{1}/waec/vouchers/sale/individual", drogon::Post);
            ADD_METHOD_TO(WaecVoucherController::handleBulkVoucherSale, "/api/v1/business/{1}/waec/vouchers/sale/bulk", drogon::Post);
            ADD_METHOD_TO(WaecVoucherController::handleAffiliateVoucherSale, "/api/v1/business/{1}/waec/vouchers/sale/affiliate", drogon::Post);
            ADD_METHOD_TO(WaecVoucherController::getMerchantDashboardStats, "/api/v1/business/{1}/waec/merchants/dashboard", drogon::Post);
            ADD_METHOD_TO(WaecVoucherController::getMerchantTransactions, "/api/v1/business/{1}/waec/merchants/transactions", drogon::Post);
        METHOD_LIST_END

        drogon::Task<void> createMerchantAccount(drogon::HttpRequestPtr req, std::function<void(const drogon::HttpResponsePtr &)> callback, std::string businessId);
        drogon::Task<void> individualVoucherSale(drogon::HttpRequestPtr req, std::function<void(const drogon::HttpResponsePtr &)> callback, std::string businessId);
        drogon::Task<void> handleBulkVoucherSale(drogon::HttpRequestPtr req, std::function<void(const drogon::HttpResponsePtr &)> callback, std::string businessId);
        drogon::Task<void> handleAffiliateVoucherSale(drogon::HttpRequestPtr req, std::function<void(const drogon::HttpResponsePtr &)> callback, std::string businessId);
        drogon::Task<void> getMerchantDashboardStats(drogon::HttpRequestPtr req, std::function<void(const drogon::HttpResponsePtr &)> callback, std::string businessId);
        drogon::Task<void> getMerchantTransactions(drogon::HttpRequestPtr req, std::function<void(const drogon::HttpResponsePtr &)> callback, std::string businessId);

    private:
        services::WaecVoucherService service_;
    };

}
#endif //DATASUPPORTAPI_WAECVOUCHERCONTROLLER_H
