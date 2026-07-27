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
  ADD_METHOD_TO(ThirdPartyController::submitWithdrawalRequest,std::string(PREFIX) + "smart-suite/submit-withdrawal-request", Post, Options);

  //WAEC eVoucher
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
  Task<HttpResponsePtr> submitWithdrawalRequest(HttpRequestPtr req);

  private:
  struct AuthResult {
      bool isValid = false;
      std::string businessId;
      HttpResponsePtr errorResponse;
  };

  Task<AuthResult> authenticateRequest(HttpRequestPtr req);
};
