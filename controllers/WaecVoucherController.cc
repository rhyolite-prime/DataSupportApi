//
// Created by Antigravity on 24/07/2026.
//

#include "WaecVoucherController.h"

namespace data_support::controllers {

    drogon::Task<void> WaecVoucherController::createMerchantAccount(drogon::HttpRequestPtr req, std::function<void(const drogon::HttpResponsePtr &)> callback, std::string businessId) {
        dto::WaecVoucherMerchantDto dto;
        // In a real implementation, you would parse the request body to populate dto
        auto res = co_await service_.createMerchantAccount(businessId, dto);
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k200OK);
        callback(resp);
    }

    drogon::Task<void> WaecVoucherController::individualVoucherSale(drogon::HttpRequestPtr req, std::function<void(const drogon::HttpResponsePtr &)> callback, std::string businessId) {
        dto::IndividualWaecVoucherSaleDto dto;
        auto res = co_await service_.individualVoucherSale(businessId, dto);
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k200OK);
        callback(resp);
    }

    drogon::Task<void> WaecVoucherController::handleBulkVoucherSale(drogon::HttpRequestPtr req, std::function<void(const drogon::HttpResponsePtr &)> callback, std::string businessId) {
        dto::BulkWaecVoucherSaleDto dto;
        auto res = co_await service_.handleBulkVoucherSale(businessId, dto);
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k200OK);
        callback(resp);
    }

    drogon::Task<void> WaecVoucherController::handleAffiliateVoucherSale(drogon::HttpRequestPtr req, std::function<void(const drogon::HttpResponsePtr &)> callback, std::string businessId) {
        dto::AffiliateWaecVoucherSaleDto dto;
        auto res = co_await service_.handleAffiliateVoucherSale(businessId, dto);
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k200OK);
        callback(resp);
    }

    drogon::Task<void> WaecVoucherController::getMerchantDashboardStats(drogon::HttpRequestPtr req, std::function<void(const drogon::HttpResponsePtr &)> callback, std::string businessId) {
        dto::WaecMerchantAuthDto dto;
        auto res = co_await service_.getMerchantDashboardStats(businessId, dto);
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k200OK);
        callback(resp);
    }

    drogon::Task<void> WaecVoucherController::getMerchantTransactions(drogon::HttpRequestPtr req, std::function<void(const drogon::HttpResponsePtr &)> callback, std::string businessId) {
        dto::WaecMerchantAuthDto dto;
        auto res = co_await service_.getMerchantTransactions(businessId, dto, 1, 10);
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k200OK);
        callback(resp);
    }

}
