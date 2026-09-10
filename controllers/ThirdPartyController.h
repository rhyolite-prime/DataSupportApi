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
  ADD_METHOD_TO(ThirdPartyController::initializeFlexyWithdrawalToBank,std::string(PREFIX) + "smart-suite/initialize-flexy-withdrawal-to-bank", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::initializeFlexyWithdrawalToMomo,std::string(PREFIX) + "smart-suite/initialize-flexy-withdrawal-to-momo", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::interAccountTransfer,std::string(PREFIX) + "smart-suite/inter-account-transfer", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::flexyToKiddyTransfer,std::string(PREFIX) + "smart-suite/flexy-to-kiddy-transfer", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::submitKiddyWithdrawalRequest,std::string(PREFIX) + "smart-suite/submit-kiddy-withdrawal-request", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::submitSavingsWithdrawalRequest,std::string(PREFIX) + "smart-suite/submit-savings-withdrawal-request", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::notifyLowPrepaidBalance,std::string(PREFIX) + "smart-suite/notify-low-prepaid-balance", Get, Options);

  //ERP-POS
  ADD_METHOD_TO(ThirdPartyController::createMerchantAccount, "erp-pos/create-merchant", Post, Options);

  //ERP-SCHOOL MANAGER
  ADD_METHOD_TO(ThirdPartyController::createMerchantAccount, "erp-sm/create-merchant", Post, Options);

  //ERP-HR
  ADD_METHOD_TO(ThirdPartyController::createMerchantAccount, "erp-hr/create-merchant", Post, Options);


  //ERP-PAYROLL
  ADD_METHOD_TO(ThirdPartyController::createMerchantAccount, "erp-pr/create-merchant", Post, Options);



  //WAEC eVoucher
  ADD_METHOD_TO(ThirdPartyController::createMerchantAccount, "waec/create-merchant", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::individualVoucherSale, "waec/voucher-sale", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::handleBulkVoucherSale, "waec/bulk-voucher-sale", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::handleAffiliateVoucherSale, "waec/affiliate-voucher-sale", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::getMerchantDashboardStats, "waec/get-merchant-stats", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::getMerchantTransactions, "waec/get-merchant-transactions", Post, Options);
  ADD_METHOD_TO(ThirdPartyController::requestMerchantStatement, "waec/request-merchant-statement", Post, Options);

  METHOD_LIST_END

  Task<HttpResponsePtr> verifyCustomerAccount(HttpRequestPtr req);
  Task<HttpResponsePtr> changePin(HttpRequestPtr req);
  Task<HttpResponsePtr> getMiniStatement(HttpRequestPtr req);
  Task<HttpResponsePtr> getAccountBalance(HttpRequestPtr req);
  Task<HttpResponsePtr> initializeDepositViaUssd(HttpRequestPtr req);
  Task<HttpResponsePtr> initializeDepositViaWebCheckout(HttpRequestPtr req);
  Task<HttpResponsePtr> initializeFlexyWithdrawalToBank(HttpRequestPtr req);
  Task<HttpResponsePtr> initializeFlexyWithdrawalToMomo(HttpRequestPtr req);
  Task<HttpResponsePtr> interAccountTransfer(HttpRequestPtr req);
  Task<HttpResponsePtr> flexyToKiddyTransfer(HttpRequestPtr req);
  Task<HttpResponsePtr> submitKiddyWithdrawalRequest(HttpRequestPtr req);
  Task<HttpResponsePtr> submitSavingsWithdrawalRequest(HttpRequestPtr req);

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
