#include "ThirdPartyController.h"

#include "dto/erp_api/SyncApiCredentialDto.h"
#include "dto/smart_suite/AccountBalanceDto.h"
#include "dto/smart_suite/ChangePinDto.h"
#include "dto/smart_suite/WebCheckoutDepositDto.h"
#include "dto/smart_suite/FlexyWithdrawalToBankDto.h"
#include "dto/smart_suite/FlexyWithdrawalToMomoDto.h"
#include "dto/smart_suite/InterAccountTransferDto.h"
#include "plugins/DataSupportServicePlugin.h"


Task<ThirdPartyController::AuthResult> ThirdPartyController::authenticateRequest(HttpRequestPtr req) {
    AuthResult result;
    auto clientId = req->getHeader("ClientId");
    auto clientSecret = req->getHeader("ClientSecret");

    if (clientId.empty() || clientSecret.empty()) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "ClientId and ClientSecret Headers are absent !";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        result.errorResponse = resp;
        co_return result;
    }

    auto plugin = drogon::app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &businessApiKeyService = plugin->getBusinessApiKeyService();

    auto businessIdentity = co_await businessApiKeyService.validateApiCredentials(clientId, clientSecret);

    if (!businessIdentity.isValid) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = businessIdentity.errorMessage.empty() ? "Invalid API credentials" : businessIdentity.errorMessage;
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k401Unauthorized);
        result.errorResponse = resp;
        co_return result;
    }

    result.isValid = true;
    result.businessId = businessIdentity.businessId;
    co_return result;
}

Task<HttpResponsePtr> ThirdPartyController::verifyCustomerAccount(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) {
        co_return authResult.errorResponse;
    }

    std::string phoneNo = req->getParameter("phoneNo");

    auto plugin = drogon::app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &smartSuiteBankingService = plugin->getSmartSuiteBankingService();

    auto result = co_await smartSuiteBankingService.verifyCustomerAccount(authResult.businessId, phoneNo);

    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());

    co_return resp;
}

Task<HttpResponsePtr> ThirdPartyController::changePin(HttpRequestPtr req)
{
    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) {
        co_return authResult.errorResponse;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::smart_suite::ChangePinDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &smartSuiteBankingService = plugin->getSmartSuiteBankingService();

    auto result = co_await smartSuiteBankingService.changePin(authResult.businessId, dto);

    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;
}

Task<HttpResponsePtr> ThirdPartyController::getMiniStatement(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) {
        co_return authResult.errorResponse;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::smart_suite::MiniStatementDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &smartSuiteBankingService = plugin->getSmartSuiteBankingService();

    auto result = co_await smartSuiteBankingService.getMiniStatement(authResult.businessId, dto);

    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;
}

Task<HttpResponsePtr> ThirdPartyController::getAccountBalance(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) {
        co_return authResult.errorResponse;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::smart_suite::AccountBalanceDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &smartSuiteBankingService = plugin->getSmartSuiteBankingService();

    auto result = co_await smartSuiteBankingService.getAccountBalance(authResult.businessId, dto);

    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}

Task<HttpResponsePtr> ThirdPartyController::initializeDepositViaUssd(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) {
        co_return authResult.errorResponse;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::smart_suite::UssdDepositDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &smartSuiteBankingService = plugin->getSmartSuiteBankingService();

    auto result = co_await smartSuiteBankingService.initializeDepositViaUssd(authResult.businessId, dto);

    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;
}

Task<HttpResponsePtr> ThirdPartyController::initializeDepositViaWebCheckout(HttpRequestPtr req) {
    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::smart_suite::WebCheckoutDepositDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &smartSuiteBankingService = plugin->getSmartSuiteBankingService();
    auto result = co_await smartSuiteBankingService.initializeDepositViaWebCheckout(authResult.businessId, dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;
}

Task<HttpResponsePtr> ThirdPartyController::getBanks(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;


    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &smartSuiteBankingService = plugin->getSmartSuiteBankingService();

    auto result = co_await smartSuiteBankingService.getAllBanks(authResult.businessId);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}

Task<HttpResponsePtr> ThirdPartyController::initializeFlexyWithdrawalToBank(HttpRequestPtr req) {
    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::smart_suite::FlexyWithdrawalToBankDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &smartSuiteBankingService = plugin->getSmartSuiteBankingService();
    auto result = co_await smartSuiteBankingService.initializeFlexyWithdrawalToBank(authResult.businessId, dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;
}

Task<HttpResponsePtr> ThirdPartyController::initializeFlexyWithdrawalToMomo(HttpRequestPtr req) {
    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::smart_suite::FlexyWithdrawalToMomoDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &smartSuiteBankingService = plugin->getSmartSuiteBankingService();
    auto result = co_await smartSuiteBankingService.initializeFlexyWithdrawalToMomo(authResult.businessId, dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;
}

Task<HttpResponsePtr> ThirdPartyController::interAccountTransfer(HttpRequestPtr req) {
    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::smart_suite::InterAccountTransferDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &smartSuiteBankingService = plugin->getSmartSuiteBankingService();
    auto result = co_await smartSuiteBankingService.interAccountTransfer(authResult.businessId, dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;
}

Task<HttpResponsePtr> ThirdPartyController::flexyToKiddyTransfer(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::smart_suite::KiddyWithdrawalRequestDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &smartSuiteBankingService = plugin->getSmartSuiteBankingService();
    auto result = co_await smartSuiteBankingService.flexyToKiddyTransfer(authResult.businessId, dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}

Task<HttpResponsePtr> ThirdPartyController::submitKiddyWithdrawalRequest(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::smart_suite::KiddyWithdrawalRequestDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &smartSuiteBankingService = plugin->getSmartSuiteBankingService();
    auto result = co_await smartSuiteBankingService.kiddyWithdrawalRequest(authResult.businessId, dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;
}

Task<HttpResponsePtr> ThirdPartyController::submitSavingsWithdrawalRequest(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::smart_suite::SavingsWithdrawalRequestDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &smartSuiteBankingService = plugin->getSmartSuiteBankingService();
    auto result = co_await smartSuiteBankingService.savingsWithdrawalRequest(authResult.businessId, dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}

//ERP-POS

Task<HttpResponsePtr> ThirdPartyController::getPosInventoryCategories(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    int pageNo = 1;
    int pageSize = 10;

    auto pageNoStr = req->getParameter("pageNo");
    if (!pageNoStr.empty()) {
        pageNo = std::stoi(pageNoStr);
    }

    auto pageSizeStr = req->getParameter("pageSize");
    if (!pageSizeStr.empty()) {
        pageSize = std::stoi(pageSizeStr);
    }

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.getCategories(authResult.businessId, pageNo, pageSize );
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}


Task<HttpResponsePtr> ThirdPartyController::getPosInventoryItems(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::erp_api::InventoryItemFilterDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.getInventoryItems(authResult.businessId, dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}

Task<HttpResponsePtr> ThirdPartyController::posCustomerSignup(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::erp_api::PosCustomerSignupDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.posCustomerSignup(authResult.businessId, dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}


Task<HttpResponsePtr> ThirdPartyController::posCustomerLogin(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::erp_api::PosCustomerLoginDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.posCustomerLogin(authResult.businessId, dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}



Task<HttpResponsePtr> ThirdPartyController::posAddItemToCart(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto customerHeader = req->getHeader("X-Customer");

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::erp_api::PosShopCartDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.addItemToCart(authResult.businessId, customerHeader, dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}

Task<HttpResponsePtr> ThirdPartyController::posRemoveItemFromCart(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto customerHeader = req->getHeader("X-Customer");

    auto itemId = req->getParameter("itemId");

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.removeItemFromCart(authResult.businessId, customerHeader, itemId);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}


Task<HttpResponsePtr> ThirdPartyController::getPosCartItemCount(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto customerHeader = req->getHeader("X-Customer");

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.getCartItemCount(authResult.businessId, customerHeader);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}

Task<HttpResponsePtr> ThirdPartyController::getPosCartItems(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto customerHeader = req->getHeader("X-Customer");

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.getCartItems(authResult.businessId, customerHeader);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}


Task<HttpResponsePtr> ThirdPartyController::posAddItemToWishlist(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto customerHeader = req->getHeader("X-Customer");

    auto itemId = req->getParameter("itemId");

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.addItemToWishlist(authResult.businessId, customerHeader, itemId);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}


Task<HttpResponsePtr> ThirdPartyController::posRemoveItemFromWishlist(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto customerHeader = req->getHeader("X-Customer");

    auto itemId = req->getParameter("itemId");

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.removeItemFromWishlist(authResult.businessId, customerHeader, itemId);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}


Task<HttpResponsePtr> ThirdPartyController::getPosWishlistItemCount(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto customerHeader = req->getHeader("X-Customer");

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.getWishlistItemCount(authResult.businessId, customerHeader);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;
}


Task<HttpResponsePtr> ThirdPartyController::getPosWishlistItems(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto customerHeader = req->getHeader("X-Customer");

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &erpApi = plugin->getErpApi();

    auto result = co_await erpApi.getWishlistItems(authResult.businessId, customerHeader);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;
}



//utils

Task<HttpResponsePtr> ThirdPartyController::notifyLowPrepaidBalance(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &smartSuiteBankingService = plugin->getSmartSuiteBankingService();
    auto result = co_await smartSuiteBankingService.sendLowPrepaidBalanceNotification(authResult.businessId);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}

//WAEC eVoucher

Task<HttpResponsePtr> ThirdPartyController::createMerchantAccount(HttpRequestPtr req) {

    //creates a merchant and a turbo ledger personal account

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::waec_voucher::WaecMerchantDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &waecVoucherService = plugin->getWaecVoucherService();

    auto result = co_await waecVoucherService.createMerchantAccount(authResult.businessId, dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}

Task<HttpResponsePtr> ThirdPartyController::individualVoucherSale(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::waec_voucher::SingleWaecVoucherSaleDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &waecVoucherService = plugin->getWaecVoucherService();

    auto result = co_await waecVoucherService.initializeSingleVoucherSale(authResult.businessId, dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}

Task<HttpResponsePtr> ThirdPartyController::handleBulkVoucherSale(HttpRequestPtr req) {

    //creates a transaction in turbo ledger in the name of the merchant account
    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::waec_voucher::BulkWaecVoucherSaleDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &waecVoucherService = plugin->getWaecVoucherService();

    auto result = co_await waecVoucherService.initializeBulkVoucherSale(authResult.businessId, dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;


}

Task<HttpResponsePtr> ThirdPartyController::handleAffiliateVoucherSale(HttpRequestPtr req) {

    // creates a transaction in turbo ledger and credits the merchant wallet
    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::waec_voucher::AffiliateWaecVoucherSaleDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &waecVoucherService = plugin->getWaecVoucherService();

    auto result = co_await waecVoucherService.initializeAffiliateVoucherSale(authResult.businessId, dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}

Task<HttpResponsePtr> ThirdPartyController::getMerchantDashboardStats(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::waec_voucher::WaecMerchantAuthDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &waecVoucherService = plugin->getWaecVoucherService();

    auto result = co_await waecVoucherService.getMerchantSummaryStats(authResult.businessId, dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;


}

Task<HttpResponsePtr> ThirdPartyController::getMerchantTransactions(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::waec_voucher::MerchantTransactionsRequestDto dto;
    dto.fromJson(*jsonPtr);


    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &waecVoucherService = plugin->getWaecVoucherService();

    auto result = co_await waecVoucherService.getMerchantTransactions(authResult.businessId, dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;


}

Task<HttpResponsePtr> ThirdPartyController::requestMerchantStatement(HttpRequestPtr req) {

    auto authResult = co_await authenticateRequest(req);
    if (!authResult.isValid) co_return authResult.errorResponse;

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        data_support::dto::BaseApiResponse response;
        response.success = false;
        response.error["message"] = "Invalid JSON body";
        auto resp = HttpResponse::newHttpJsonResponse(response.toJson());
        resp->setStatusCode(k400BadRequest);
        co_return resp;
    }

    data_support::dto::waec_voucher::MerchantStatementRequestDto dto;
    dto.fromJson(*jsonPtr);

    auto plugin = app().getPlugin<data_support::plugins::DataSupportServicePlugin>();
    auto &waecVoucherService = plugin->getWaecVoucherService();

    auto result = co_await waecVoucherService.getMerchantStatement(authResult.businessId, dto);
    auto resp = HttpResponse::newHttpJsonResponse(result.toJson());
    co_return resp;

}