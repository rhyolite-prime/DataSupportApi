#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class ThirdPartyController : public HttpController<ThirdPartyController>
{
  public:
  static constexpr const char *PREFIX = "/api/v1/third-party/";

  METHOD_LIST_BEGIN

  //smart suite
  ADD_METHOD_TO(ThirdPartyController::verifyCustomerAccount,std::string(PREFIX) + "smart-suite/verify-customer-account", Get, Options);
  ADD_METHOD_TO(ThirdPartyController::changePin,std::string(PREFIX) + "smart-suite/change-pin", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::getMiniStatement,std::string(PREFIX) + "smart-suite/get-mini-statement", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::getAccountBalance,std::string(PREFIX) + "smart-suite/get-account-balance", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::initializeDepositViaUssd,std::string(PREFIX) + "smart-suite/initialize-deposit-via-ussd", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::initializeDepositViaWebCheckout,std::string(PREFIX) + "smart-suite/initialize-deposit-via-web-checkout", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::getBanks,std::string(PREFIX) + "smart-suite/get-banks", Get, Options);
  ADD_METHOD_TO(ThirdPartyController::initializeFlexyWithdrawalToBank,std::string(PREFIX) + "smart-suite/initialize-flexy-withdrawal-to-bank", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::initializeFlexyWithdrawalToMomo,std::string(PREFIX) + "smart-suite/initialize-flexy-withdrawal-to-momo", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::interAccountTransfer,std::string(PREFIX) + "smart-suite/inter-account-transfer", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::flexyToKiddyTransfer,std::string(PREFIX) + "smart-suite/flexy-to-kiddy-transfer", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::submitKiddyWithdrawalRequest,std::string(PREFIX) + "smart-suite/submit-kiddy-withdrawal-request", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::submitSavingsWithdrawalRequest,std::string(PREFIX) + "smart-suite/submit-savings-withdrawal-request", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::notifyLowPrepaidBalance, std::string(PREFIX) + "smart-suite/notify-low-prepaid-balance", Get, Options);

  //ERP-POS
  ADD_METHOD_TO(ThirdPartyController::getPosInventoryCategories, std::string(PREFIX) +"erp-pos/get-categories", Get, Options);
  ADD_METHOD_TO(ThirdPartyController::getPosInventoryItems, std::string(PREFIX) +"erp-pos/get-items", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::posCustomerSignup, std::string(PREFIX) +"erp-pos/customer-signup", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::posCustomerLogin, std::string(PREFIX) +"erp-pos/customer-login", Post, Options);

  ADD_METHOD_TO(ThirdPartyController::posAddItemToCart, std::string(PREFIX) +"erp-pos/add-to-cart", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::posRemoveItemFromCart, std::string(PREFIX) +"erp-pos/remove-from-cart", Get, Options);
  ADD_METHOD_TO(ThirdPartyController::getPosCartItemCount, std::string(PREFIX) +"erp-pos/get-cart-item-count", Get, Options);
  ADD_METHOD_TO(ThirdPartyController::getPosCartItems, std::string(PREFIX) +"erp-pos/get-cart-items", Get, Options);

  ADD_METHOD_TO(ThirdPartyController::posAddItemToWishlist, std::string(PREFIX) +"erp-pos/add-to-wishlist", Get, Options);
  ADD_METHOD_TO(ThirdPartyController::posRemoveItemFromWishlist, std::string(PREFIX) +"erp-pos/remove-from-wishlist", Get, Options);
  ADD_METHOD_TO(ThirdPartyController::getPosWishlistItemCount, std::string(PREFIX) +"erp-pos/get-wishlist-item-count", Get, Options);
  ADD_METHOD_TO(ThirdPartyController::getPosWishlistItems, std::string(PREFIX) +"erp-pos/get-wishlist-items", Get, Options);

  //ERP-SCHOOL MANAGER
  ADD_METHOD_TO(ThirdPartyController::createMerchantAccount, std::string(PREFIX) + "erp-sm/create-merchant", Post, Options);

  //ERP-HR
  ADD_METHOD_TO(ThirdPartyController::createMerchantAccount, std::string(PREFIX) + "erp-hr/create-merchant", Post, Options);


  //ERP-PAYROLL
  ADD_METHOD_TO(ThirdPartyController::createMerchantAccount, std::string(PREFIX) + "erp-pr/create-merchant", Post, Options);



  //WAEC eVoucher
  ADD_METHOD_TO(ThirdPartyController::createMerchantAccount, std::string(PREFIX) + "waec/create-merchant", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::individualVoucherSale, std::string(PREFIX) + "waec/voucher-sale", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::handleBulkVoucherSale, std::string(PREFIX) + "waec/bulk-voucher-sale", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::handleAffiliateVoucherSale, std::string(PREFIX) + "waec/affiliate-voucher-sale", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::getMerchantDashboardStats, std::string(PREFIX) + "waec/get-merchant-stats", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::getMerchantTransactions, std::string(PREFIX) + "waec/get-merchant-transactions", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::requestMerchantStatement, std::string(PREFIX) + "waec/request-merchant-statement", Post, Options);

  METHOD_LIST_END

  Task<HttpResponsePtr> verifyCustomerAccount(HttpRequestPtr req);
  Task<HttpResponsePtr> changePin(HttpRequestPtr req);
  Task<HttpResponsePtr> getMiniStatement(HttpRequestPtr req);
  Task<HttpResponsePtr> getAccountBalance(HttpRequestPtr req);
  Task<HttpResponsePtr> initializeDepositViaUssd(HttpRequestPtr req);
  Task<HttpResponsePtr> initializeDepositViaWebCheckout(HttpRequestPtr req);
  Task<HttpResponsePtr> getBanks(HttpRequestPtr req);
  Task<HttpResponsePtr> initializeFlexyWithdrawalToBank(HttpRequestPtr req);
  Task<HttpResponsePtr> initializeFlexyWithdrawalToMomo(HttpRequestPtr req);
  Task<HttpResponsePtr> interAccountTransfer(HttpRequestPtr req);
  Task<HttpResponsePtr> flexyToKiddyTransfer(HttpRequestPtr req);
  Task<HttpResponsePtr> submitKiddyWithdrawalRequest(HttpRequestPtr req);
  Task<HttpResponsePtr> submitSavingsWithdrawalRequest(HttpRequestPtr req);

  //ERP-SCHOOL MANAGER
  Task<HttpResponsePtr> getPosInventoryCategories(HttpRequestPtr req);
  Task<HttpResponsePtr> getPosInventoryItems(HttpRequestPtr req);
  Task<HttpResponsePtr> posCustomerSignup(HttpRequestPtr req);
  Task<HttpResponsePtr> posCustomerLogin(HttpRequestPtr req);

  Task<HttpResponsePtr> posAddItemToCart(HttpRequestPtr req);
  Task<HttpResponsePtr> posRemoveItemFromCart(HttpRequestPtr req);
  Task<HttpResponsePtr> getPosCartItemCount(HttpRequestPtr req);
  Task<HttpResponsePtr> getPosCartItems(HttpRequestPtr req);

  Task<HttpResponsePtr> posAddItemToWishlist(HttpRequestPtr req);
  Task<HttpResponsePtr> posRemoveItemFromWishlist(HttpRequestPtr req);
  Task<HttpResponsePtr> getPosWishlistItemCount(HttpRequestPtr req);
  Task<HttpResponsePtr> getPosWishlistItems(HttpRequestPtr req);





  //utils
  Task<HttpResponsePtr> notifyLowPrepaidBalance(HttpRequestPtr req);


    //waec vouchers
  Task<HttpResponsePtr> createMerchantAccount(HttpRequestPtr req);
  Task<HttpResponsePtr> individualVoucherSale(HttpRequestPtr req);
  Task<HttpResponsePtr> handleBulkVoucherSale(HttpRequestPtr req);
  Task<HttpResponsePtr> handleAffiliateVoucherSale(HttpRequestPtr req);
  Task<HttpResponsePtr> getMerchantDashboardStats(HttpRequestPtr req);
  Task<HttpResponsePtr> getMerchantTransactions(HttpRequestPtr req);
  Task<HttpResponsePtr> requestMerchantStatement(HttpRequestPtr req);


  private:
  struct AuthResult {
      bool isValid = false;
      std::string businessId;
      HttpResponsePtr errorResponse;
  };

  Task<AuthResult> authenticateRequest(HttpRequestPtr req);
};
