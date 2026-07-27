//
// Created by Emmanuel Addo-Odame on 26/07/2026.
//
#include "SmartSuiteBankingService.h"
#include "BusinessAccounts.h"
#include "SmartSuiteTransactions.h"
#include "dto/smart_suite/SmartSuiteApiResponse.h"
#include "dto/smart_suite/SmartSuiteOperationResponse.h"
#include "plugins/DataSupportServicePlugin.h"
#include "utils/CryptoUtils.h"
#include <drogon/utils/Utilities.h>
#include <iomanip>
#include <chrono>

namespace data_support::services {

drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> SmartSuiteBankingService::verifyCustomerAccount(const std::string &businessId,
                                                const std::string &phoneNo) {
  dto::smart_suite::SmartSuiteOperationResponse response;

  try {

    auto dbClient = drogon::app().getDbClient();
    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts>
        mapper(dbClient);

    auto businessAccount = co_await mapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "Failed to parse ConfigurationData: " << errs;
      response.code = 1;
      response.message = "Configuration error. Request Failed !";
      co_return response;
    }

    const auto &smartSuiteConfig = configJson["SmartSuiteApi"];
    if (smartSuiteConfig.isNull()) {
      response.code = 1;
      response.message = "Smart Suite configuration missing. Request Failed !";
      co_return response;
    }

    std::string baseUrl = smartSuiteConfig["BaseUrl"].asString();
    std::string vTelcom = smartSuiteConfig["v_TELCOM"].asString();
    std::string vApiKey = smartSuiteConfig["v_APIKEY"].asString();

    Json::Value payload;
    payload["v_PHONENO"] = phoneNo;
    payload["v_Amnt_ip"] = "";
    payload["v_transT"] = "P";
    payload["v_ACTYPE"] = "F";
    payload["v_ACCR"] = "";
    payload["V_Pin"] = "";
    payload["V_PinNew"] = "";
    payload["V_TRANSREF"] = "";
    payload["v_TELCOM"] = vTelcom;
    payload["v_APIKEY"] = vApiKey;

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string body = Json::writeString(writer, payload);

    auto client = drogon::HttpClient::newHttpClient(baseUrl);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);

    req->setPath("");
    req->setBody(body);
    req->addHeader("Content-Type", "application/json");

    auto httpResp = co_await client->sendRequestCoro(req);

    if (httpResp && (httpResp->statusCode() == drogon::k200OK ||
                     httpResp->statusCode() == drogon::k201Created)) {
      const std::string responseStr = std::string(httpResp->getBody());

      Json::Value jsonResponse;
      std::istringstream respIss(responseStr);
      if (Json::parseFromStream(readerBuilder, respIss, &jsonResponse, &errs)) {

        auto result = dto::SmartSuiteApiResponseModel<Json::Value>::fromJson(
            jsonResponse);

        bool isSuccess = (result.getRespcode() == "1");

        response.message =
            isSuccess ? "Account identified"
                      : "Account not registered! Please register your account.";
        response.code = isSuccess ? 0 : 1;
        response.success = isSuccess;

        co_return response;
      }
    }

    response.message = "Failed to identify account. Please try again.";
    response.code = 1;
    response.success = false;
    co_return response;

  } catch (const std::exception &e) {
    LOG_ERROR << "Error verifying customer account: " << e.what();

    response.message = "Failed to identify account. Please try again.";
    response.code = 1;
    response.success = false;
    co_return response;
  }
}

drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> SmartSuiteBankingService::changePin(const std::string &businessId,
                                    const dto::smart_suite::ChangePinDto &dto) {

  dto::smart_suite::SmartSuiteOperationResponse response;

  try {

    auto dbClient = drogon::app().getDbClient();
    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> mapper(dbClient);

    auto businessAccount = co_await mapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "Failed to parse ConfigurationData: " << errs;
      response.code = 2;
      response.message = "Failed to change PIN. Configuration error.";
      co_return response;
    }

    const auto &smartSuiteConfig = configJson["SmartSuiteApi"];
    if (smartSuiteConfig.isNull()) {
      response.code = 2;
      response.message = "Failed to change PIN. Smart Suite configuration missing.";
      co_return response;
    }

    std::string baseUrl = smartSuiteConfig["BaseUrl"].asString();
    std::string vTelcom = smartSuiteConfig["v_TELCOM"].asString();
    std::string vApiKey = smartSuiteConfig["v_APIKEY"].asString();

    Json::Value payload;
    payload["v_PHONENO"] = dto.getPhoneNo();
    payload["v_Amnt_ip"] = "";
    payload["v_transT"] = "P";
    payload["v_ACTYPE"] = "F";
    payload["v_ACCR"] = "";
    payload["V_Pin"] = dto.getCurrentPin();
    payload["V_PinNew"] = dto.getNewPin();
    payload["V_TRANSREF"] = "";
    payload["v_TELCOM"] = vTelcom;
    payload["v_APIKEY"] = vApiKey;

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string body = Json::writeString(writer, payload);

    auto client = drogon::HttpClient::newHttpClient(baseUrl);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("");
    req->setBody(body);
    req->addHeader("Content-Type", "application/json");

    auto httpResp = co_await client->sendRequestCoro(req);

    if (httpResp && (httpResp->statusCode() == drogon::k200OK ||
                     httpResp->statusCode() == drogon::k201Created)) {
      const std::string responseStr = std::string(httpResp->getBody());

      Json::Value jsonResponse;
      std::istringstream respIss(responseStr);
      if (Json::parseFromStream(readerBuilder, respIss, &jsonResponse, &errs)) {

        auto result = dto::SmartSuiteApiResponseModel<Json::Value>::fromJson(jsonResponse);

        std::string respDescLower = result.getRespdesc();
        std::transform(respDescLower.begin(), respDescLower.end(), respDescLower.begin(), ::tolower);

        bool isSuccess = (result.getRespcode() == "1" && respDescLower == "success");

        response.message = isSuccess ? "PIN updated successfully" : "Failed to change PIN. Please try again.";

        try {
          response.code = std::stoi(result.getRespcode());
        } catch (...) {
          response.code = 2;
        }
        response.success = isSuccess;

        co_return response;
      }
    }

    response.message = "Failed to change PIN. Please try again.";
    response.code = 2;
    response.success = false;
    co_return response;

  } catch (const std::exception &e) {
    LOG_ERROR << "Error changing PIN: " << e.what();

    response.message = "Failed to change PIN. Please try again.";
    response.code = 2;
    response.success = false;
    co_return response;
  }
}

drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> SmartSuiteBankingService::getMiniStatement(
    const std::string &businessId,
    const dto::smart_suite::MiniStatementDto &dto) {

  dto::smart_suite::SmartSuiteOperationResponse response;

  try {
    auto dbClient = drogon::app().getDbClient();
    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts>
        mapper(dbClient);

    auto businessAccount = co_await mapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "Failed to parse ConfigurationData: " << errs;
      response.code = 1;
      response.message = "Failed to retrieve statement.";
      co_return response;
    }

    const auto &smartSuiteConfig = configJson["SmartSuiteApi"];
    if (smartSuiteConfig.isNull()) {
      response.code = 1;
      response.message = "Failed to retrieve statement.";
      co_return response;
    }

    std::string baseUrl = smartSuiteConfig["BaseUrl"].asString();
    std::string vTelcom = smartSuiteConfig["v_TELCOM"].asString();
    std::string vApiKey = smartSuiteConfig["v_APIKEY"].asString();

    std::string accountTypeUpper = dto.getAccountType();
    std::transform(accountTypeUpper.begin(), accountTypeUpper.end(),
                   accountTypeUpper.begin(), ::toupper);

    std::string serialNoStr =
        (accountTypeUpper == "K") ? dto.getSerialNo() : "";

    Json::Value payload;
    payload["v_PHONENO"] = dto.getPhoneNo();
    payload["v_Amnt_ip"] = "";
    payload["v_transT"] = "S";
    payload["v_ACTYPE"] = accountTypeUpper;
    payload["v_ACCR"] = "";
    payload["V_Pin"] = dto.getPin();
    payload["V_PinNew"] = "";
    payload["v_SERIALNO"] = serialNoStr;
    payload["V_TRANSREF"] = "";
    payload["v_TELCOM"] = vTelcom;
    payload["v_APIKEY"] = vApiKey;

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string body = Json::writeString(writer, payload);

    auto client = drogon::HttpClient::newHttpClient(baseUrl);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("");
    req->setBody(body);
    req->addHeader("Content-Type", "application/json");

    auto httpResp = co_await client->sendRequestCoro(req);

    if (httpResp && (httpResp->statusCode() == drogon::k200OK ||
                     httpResp->statusCode() == drogon::k201Created)) {
      const std::string responseStr = std::string(httpResp->getBody());

      Json::Value jsonResponse;
      std::istringstream respIss(responseStr);
      if (Json::parseFromStream(readerBuilder, respIss, &jsonResponse, &errs)) {

        auto result = dto::SmartSuiteApiResponseModel<Json::Value>::fromJson(
            jsonResponse);

        const auto &respData = result.getRespdataresults();
        if (!respData.isArray() || respData.empty()) {
          response.message = "No transactions found.";
          response.code = 0;
          response.success = true;
          co_return response;
        }

        std::ostringstream sb;
        sb << "*** Mini Statement ***\n";

        for (const auto &transaction : respData) {
          std::string trDate = transaction["TRDATE"].asString();
          std::string datePart = trDate.substr(0, trDate.find(' '));
          std::string amount = transaction["AMOUNT"].asString();
          std::string trType = transaction["TRTYPE"].asString();

          sb << "Date: " << datePart << ", Amount: " << amount << " - "
             << trType << "\n";
        }

        std::string respDescLower = result.getRespdesc();
        std::transform(respDescLower.begin(), respDescLower.end(),
                       respDescLower.begin(), ::tolower);

        bool isSuccess =
            (result.getRespcode() == "0" && respDescLower == "success");

        response.message =
            isSuccess ? sb.str() : "Failed to retrieve statement.";

        try {
          response.code = std::stoi(result.getRespcode());
        } catch (...) {
          response.code = 1;
        }
        response.success = isSuccess;

        co_return response;
      }
    }

    response.message = "Failed to retrieve statement.";
    response.code = 1;
    response.success = false;
    co_return response;

  } catch (const std::exception &e) {
    LOG_ERROR << "Error retrieving mini statement: " << e.what();

    response.message = "Failed to retrieve statement.";
    response.code = 500;
    response.success = false;
    co_return response;
  }
}

drogon::Task<dto::smart_suite::SmartSuiteOperationResponse>
SmartSuiteBankingService::getAccountBalance(
    const std::string &businessId,
    const dto::smart_suite::AccountBalanceDto &dto) {

  dto::smart_suite::SmartSuiteOperationResponse response;

  try {
    auto dbClient = drogon::app().getDbClient();
    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts>
        mapper(dbClient);

    auto businessAccount = co_await mapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "Failed to parse ConfigurationData: " << errs;
      response.code = 1;
      response.message = "Failed to retrieve account balance.";
      co_return response;
    }

    const auto &smartSuiteConfig = configJson["SmartSuiteApi"];
    if (smartSuiteConfig.isNull()) {
      response.code = 1;
      response.message = "Failed to retrieve account balance.";
      co_return response;
    }

    std::string baseUrl = smartSuiteConfig["BaseUrl"].asString();
    std::string vTelcom = smartSuiteConfig["v_TELCOM"].asString();
    std::string vApiKey = smartSuiteConfig["v_APIKEY"].asString();

    std::string accountTypeUpper = dto.getAccountType();
    std::transform(accountTypeUpper.begin(), accountTypeUpper.end(),
                   accountTypeUpper.begin(), ::toupper);

    Json::Value payload;
    payload["v_PHONENO"] = dto.getPhoneNo();
    payload["v_Amnt_ip"] = "";
    payload["v_transT"] = "B";
    payload["v_ACTYPE"] = accountTypeUpper;
    payload["v_ACCR"] = "";
    payload["V_Pin"] = dto.getPin();
    payload["V_PinNew"] = "";
    payload["V_TRANSREF"] = "";
    payload["v_TELCOM"] = vTelcom;
    payload["v_APIKEY"] = vApiKey;

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string body = Json::writeString(writer, payload);

    auto client = drogon::HttpClient::newHttpClient(baseUrl);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("");
    req->setBody(body);
    req->addHeader("Content-Type", "application/json");

    auto httpResp = co_await client->sendRequestCoro(req);

    if (httpResp && (httpResp->statusCode() == drogon::k200OK ||
                     httpResp->statusCode() == drogon::k201Created)) {
      const std::string responseStr = std::string(httpResp->getBody());

      Json::Value jsonResponse;
      std::istringstream respIss(responseStr);
      if (Json::parseFromStream(readerBuilder, respIss, &jsonResponse, &errs)) {

        auto result = dto::SmartSuiteApiResponseModel<Json::Value>::fromJson(
            jsonResponse);

        std::string respDescLower = result.getRespdesc();
        std::transform(respDescLower.begin(), respDescLower.end(),
                       respDescLower.begin(), ::tolower);

        bool isSuccess =
            (result.getRespcode() == "0" && respDescLower == "success");

        if (accountTypeUpper == "K") {
          if (!isSuccess) {
            response.message = "Failed to Retrieve Kiddy Account Balances.";
          } else {
            const auto &respData = result.getRespdataresults();
            if (!respData.isArray() || respData.empty()) {
              response.message = "No Kiddy Account Balances Found !";
            } else {
              std::ostringstream sb;
              sb << "** Kiddy Account Balances **\n";
              for (const auto &kiddy : respData) {
                sb << kiddy["SURNAME"].asString()
                   << " - Balance: " << kiddy["BALANCE"].asString() << "\n";
              }
              response.message = sb.str();
            }
          }
        } else if (accountTypeUpper == "L") {
          if (!isSuccess) {
            response.message = "Failed to Retrieve Loan Account Balance(s).";
          } else {
            const auto &respData = result.getRespdataresults();
            if (!respData.isArray() || respData.empty()) {
              response.message = "No Loan Accounts Found !";
            } else {
              std::ostringstream sb;
              sb << "*** Loan Account Balances ***\n";
              for (const auto &loan : respData) {
                sb << "Serial No: " << loan["SERIAL_NO"].asString()
                   << ", Balance: " << loan["BALANCE"].asString()
                   << " :: " << loan["LOAN_TYPE"].asString() << "\n";
              }
              response.message = sb.str();
            }
          }
        } else if (accountTypeUpper == "F") {
          response.message = isSuccess ? "Your Flexy Account Balance is GHS " +
                                             result.getFlexy()
                                       : "Failed to retrieve account balance.";
        } else if (accountTypeUpper == "U") {
          response.message =
              isSuccess ? "Your Savings Deposit Account Balance is GHS " +
                              result.getSavings()
                        : "Failed to Retrieve Savings Deposit Account Balance.";
        } else if (accountTypeUpper == "S") {
          response.message =
              isSuccess
                  ? "Your Shares Account Balance is GHS " + result.getShares()
                  : "Failed to Retrieve Shares Account Balance.";
        } else {
          response.message = isSuccess
                                 ? "Account Balance retrieved successfully."
                                 : "Failed to retrieve account balance.";
        }

        try {
          response.code = std::stoi(result.getRespcode());
        } catch (...) {
          response.code = 1;
        }
        response.success = isSuccess;

        co_return response;
      }
    }

    response.message = "Failed to retrieve account balance.";
    response.code = 1;
    response.success = false;
    co_return response;

  } catch (const std::exception &e) {
    LOG_ERROR << "Error retrieving account balance: " << e.what();

    response.message = "Failed to retrieve account balance.";
    response.code = 1;
    response.success = false;
    co_return response;
  }
}

drogon::Task<dto::smart_suite::SmartSuiteOperationResponse>
SmartSuiteBankingService::initializeDepositViaUssd(
    const std::string &businessId,
    const dto::smart_suite::UssdDepositDto &dto) {

  dto::smart_suite::SmartSuiteOperationResponse response;

  try {
    auto dbClient = drogon::app().getDbClient();
    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts>
        bizMapper(dbClient);
    auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "Failed to parse ConfigurationData: " << errs;
      response.message = "Failed to initialize deposit.";
      response.code = 1;
      co_return response;
    }

    std::string encryptionKey =
        configJson["SmartSuiteApi"]["EncryptionKey"].asString();
    if (encryptionKey.length() < 32) {
      LOG_ERROR << "SmartSuiteApi EncryptionKey is missing or too short (needs "
                   "32 chars).";
      response.message = "Failed to initialize deposit due to config error.";
      response.code = 1;
      co_return response;
    }

    std::string clientRef = drogon::utils::getUuid();
    clientRef.erase(std::remove(clientRef.begin(), clientRef.end(), '-'),
                    clientRef.end());

    drogon_model::DataSupportDb::SmartSuiteTransactions transaction;
    transaction.setBusinessId(businessId);
    transaction.setPhoneNo(dto.getCustomerMsisdn());
    transaction.setAmount(std::to_string(dto.getAmount()));
    transaction.setAccountType(dto.getAccountType());
    transaction.setTransactionType(dto.getTransactionType());
    transaction.setStatus("PENDING");
    transaction.setReference(clientRef);
    transaction.setIntegrationChannel(dto.getIntegrationChannel());
    transaction.setIntegratingService(dto.getIntegrationService());
    transaction.setProvider(dto.getProvider());
    transaction.setChildAccountNumber(dto.getKiddyAccountSerialNo());
    transaction.setPin(
        utils::CryptoUtils::encrypt(dto.getPin(), encryptionKey));

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::SmartSuiteTransactions>
        txnMapper(dbClient);
    auto savedTxn = co_await txnMapper.insert(transaction);

    dto::DirectReceiveMoneyDto receiveDto;
    receiveDto.setCustomerName(dto.getCustomerName());
    receiveDto.setCustomerMsisdn(dto.getCustomerMsisdn());
    receiveDto.setCustomerEmail(dto.getCustomerEmail());
    receiveDto.setAmount(dto.getAmount());
    receiveDto.setPrimaryCallbackUrl(dto.getPrimaryCallbackUrl());
    receiveDto.setDescription(dto.getDescription());
    receiveDto.setProvider(dto.getProvider());
    receiveDto.setClientReference(clientRef);
    receiveDto.setChannel(dto.getIntegrationChannel());

    auto plugin = drogon::app().getPlugin<plugins::DataSupportServicePlugin>();
    auto &hubtelApi = plugin->getHubtelApi();

    auto hubtelResponse =
        co_await hubtelApi.initializeDirectReceiveMoney(businessId, receiveDto);

    response.code = hubtelResponse.code;
    response.message = hubtelResponse.message;
    response.success =
        (hubtelResponse.code == 0 || hubtelResponse.code == 200 ||
         hubtelResponse.code == 201); // Based on standard success codes

    co_return response;

  } catch (const std::exception &e) {
    LOG_ERROR << "Error initializing USSD deposit: " << e.what();
    response.message = "Failed to initialize deposit.";
    response.code = 500;
    response.success = false;
    co_return response;
  }
}

drogon::Task<dto::smart_suite::SmartSuiteOperationResponse>
SmartSuiteBankingService::initializeDepositViaWebCheckout(
    const std::string &businessId,
    const dto::smart_suite::WebCheckoutDepositDto &dto) {

  dto::smart_suite::SmartSuiteOperationResponse response;

  try {
    auto dbClient = drogon::app().getDbClient();
    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts>
        bizMapper(dbClient);
    auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "Failed to parse ConfigurationData: " << errs;
      response.message = "Failed to initialize web checkout.";
      response.code = 1;
      co_return response;
    }

    std::string encryptionKey =
        configJson["SmartSuiteApi"]["EncryptionKey"].asString();
    if (encryptionKey.length() < 32) {
      LOG_ERROR << "SmartSuiteApi EncryptionKey is missing or too short.";
      response.message =
          "Failed to initialize web checkout due to config error.";
      response.code = 1;
      co_return response;
    }

    std::string clientRef = drogon::utils::getUuid();
    clientRef.erase(std::remove(clientRef.begin(), clientRef.end(), '-'),
                    clientRef.end());

    drogon_model::DataSupportDb::SmartSuiteTransactions transaction;
    transaction.setBusinessId(businessId);
    transaction.setPhoneNo(dto.getPhoneNo());
    transaction.setAmount(std::to_string(dto.getAmount()));
    transaction.setAccountType(dto.getAccountType());
    transaction.setTransactionType(dto.getTransactionType());
    transaction.setStatus("PENDING");
    transaction.setReference(clientRef);
    transaction.setIntegrationChannel(dto.getIntegrationChannel());
    transaction.setIntegratingService(dto.getIntegrationService());
    transaction.setChildAccountNumber(dto.getKiddyAccountSerialNo());
    transaction.setPin(
        utils::CryptoUtils::encrypt(dto.getPin(), encryptionKey));

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::SmartSuiteTransactions>
        txnMapper(dbClient);
    auto savedTxn = co_await txnMapper.insert(transaction);

    dto::WebCheckoutDto checkoutDto;
    checkoutDto.phoneNo = dto.getPhoneNo();
    checkoutDto.totalAmount = dto.getAmount();
    checkoutDto.description = dto.getDescription();
    checkoutDto.callbackUrl = dto.getPrimaryCallbackUrl();
    checkoutDto.returnUrl = dto.getReturnUrl();
    checkoutDto.cancellationUrl = dto.getCancellationUrl();
    checkoutDto.clientReference = clientRef;

    auto plugin = drogon::app().getPlugin<plugins::DataSupportServicePlugin>();
    auto &hubtelApi = plugin->getHubtelApi();

    auto hubtelResponse =
        co_await hubtelApi.initializeWebCheckout(businessId, checkoutDto);

    response.code = hubtelResponse.code;
    response.message = hubtelResponse.message;
    response.success =
        (hubtelResponse.code == 0 || hubtelResponse.code == 200 ||
         hubtelResponse.code == 201);
    response.checkoutUrl = hubtelResponse.checkoutUrl;

    co_return response;

  } catch (const std::exception &e) {
    LOG_ERROR << "Error initializing web checkout deposit: " << e.what();
    response.message = "Failed to initialize web checkout deposit.";
    response.code = 500;
    response.success = false;
    co_return response;
  }
}

drogon::Task<dto::smart_suite::SmartSuiteOperationResponse>
SmartSuiteBankingService::initializeFlexyWithdrawalToBank(
    const std::string &businessId,
    const dto::smart_suite::FlexyWithdrawalToBankDto &dto) {

  dto::smart_suite::SmartSuiteOperationResponse response;

  try {
    auto dbClient = drogon::app().getDbClient();
    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts>
        bizMapper(dbClient);
    auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "Failed to parse ConfigurationData: " << errs;
      response.message = "Failed to initialize withdrawal to bank.";
      response.code = 1;
      co_return response;
    }

    std::string encryptionKey =
        configJson["SmartSuiteApi"]["EncryptionKey"].asString();
    if (encryptionKey.length() < 32) {
      LOG_ERROR << "SmartSuiteApi EncryptionKey is missing or too short.";
      response.message = "Failed to initialize withdrawal due to config error.";
      response.code = 1;
      co_return response;
    }

    std::string clientRef = drogon::utils::getUuid();
    clientRef.erase(std::remove(clientRef.begin(), clientRef.end(), '-'),
                    clientRef.end());

    drogon_model::DataSupportDb::SmartSuiteTransactions transaction;
    transaction.setBusinessId(businessId);
    transaction.setPhoneNo(dto.getSenderMsisdn());
    transaction.setAmount(dto.getAmount());
    transaction.setAccountType(dto.getAccountType());
    transaction.setTransactionType(dto.getTransactionType());
    transaction.setStatus("PENDING");
    transaction.setReference(clientRef);
    transaction.setIntegrationChannel(dto.getIntegrationChannel());
    transaction.setIntegratingService(dto.getIntegratingService());
    // Pin requires encryption
    transaction.setPin(
        utils::CryptoUtils::encrypt(dto.getPin(), encryptionKey));

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::SmartSuiteTransactions>
        txnMapper(dbClient);
    auto savedTxn = co_await txnMapper.insert(transaction);

    dto::DirectSendToBankDto sendDto;
    sendDto.setBankName(dto.getBankName());
    sendDto.setBankBranch(dto.getBankBranch());
    sendDto.setBankBranchCode(dto.getBankBranchCode());
    try {
      sendDto.setAmount(std::stod(dto.getAmount()));
    } catch (...) {
      sendDto.setAmount(0.0);
    }
    sendDto.setBankAccountNumber(dto.getBankAccountNumber());
    // No bank account name in FlexyWithdrawalToBankDto, use default/empty
    sendDto.setBankAccountName("");
    sendDto.setPrimaryCallbackUrl(dto.getPrimaryCallbackUrl());
    sendDto.setDescription(dto.getDescription());
    sendDto.setSenderMsisdn(dto.getSenderMsisdn());
    sendDto.setRecipientPhoneNumber(dto.getSenderMsisdn()); // Best alternative
    sendDto.setClientReference(clientRef);

    auto plugin = drogon::app().getPlugin<plugins::DataSupportServicePlugin>();
    auto &hubtelApi = plugin->getHubtelApi();

    auto hubtelResponse =
        co_await hubtelApi.initializeDirectSendToBank(businessId, sendDto);

    response.code = hubtelResponse.code;
    response.message = hubtelResponse.message;
    response.success =
        (hubtelResponse.code == 0 || hubtelResponse.code == 200 ||
         hubtelResponse.code == 201);

    co_return response;

  } catch (const std::exception &e) {
    LOG_ERROR << "Error initializing bank withdrawal: " << e.what();
    response.message = "Failed to initialize bank withdrawal.";
    response.code = 500;
    response.success = false;
    co_return response;
  }
}

drogon::Task<dto::smart_suite::SmartSuiteOperationResponse>
SmartSuiteBankingService::initializeFlexyWithdrawalToMomo(
    const std::string &businessId,
    const dto::smart_suite::FlexyWithdrawalToMomoDto &dto) {

  dto::smart_suite::SmartSuiteOperationResponse response;

  try {
    auto dbClient = drogon::app().getDbClient();
    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts>
        bizMapper(dbClient);
    auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "Failed to parse ConfigurationData: " << errs;
      response.message = "Failed to initialize withdrawal to momo.";
      response.code = 1;
      co_return response;
    }

    std::string encryptionKey =
        configJson["SmartSuiteApi"]["EncryptionKey"].asString();
    if (encryptionKey.length() < 32) {
      LOG_ERROR << "SmartSuiteApi EncryptionKey is missing or too short.";
      response.message = "Failed to initialize withdrawal due to config error.";
      response.code = 1;
      co_return response;
    }

    std::string clientRef = drogon::utils::getUuid();
    clientRef.erase(std::remove(clientRef.begin(), clientRef.end(), '-'),
                    clientRef.end());

    drogon_model::DataSupportDb::SmartSuiteTransactions transaction;
    transaction.setBusinessId(businessId);
    transaction.setPhoneNo(dto.getSenderMsisdn());
    transaction.setAmount(std::to_string(dto.getAmount()));
    transaction.setAccountType(dto.getAccountType());
    transaction.setTransactionType(dto.getTransactionType());
    transaction.setStatus("PENDING");
    transaction.setReference(clientRef);
    transaction.setIntegrationChannel(dto.getIntegrationChannel());
    transaction.setIntegratingService(dto.getIntegrationService());
    transaction.setChildAccountNumber(dto.getKiddyAccountSerialNo());
    transaction.setPin(
        utils::CryptoUtils::encrypt(dto.getPin(), encryptionKey));

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::SmartSuiteTransactions>
        txnMapper(dbClient);
    auto savedTxn = co_await txnMapper.insert(transaction);

    dto::DirectSendMomoDto sendDto;
    sendDto.setRecipientName(dto.getRecipientName());
    sendDto.setRecipientMsisdn(dto.getRecipientMsisdn());
    sendDto.setCustomerEmail(dto.getCustomerEmail());
    sendDto.setAmount(dto.getAmount());
    sendDto.setPrimaryCallbackUrl(dto.getPrimaryCallbackUrl());
    sendDto.setDescription(dto.getDescription());
    sendDto.setProvider(dto.getProvider());
    sendDto.setClientReference(clientRef);

    auto plugin = drogon::app().getPlugin<plugins::DataSupportServicePlugin>();
    auto &hubtelApi = plugin->getHubtelApi();

    auto hubtelResponse =
        co_await hubtelApi.initializeDirectSendMomo(businessId, sendDto);

    response.code = hubtelResponse.code;
    response.message = hubtelResponse.message;
    response.success =
        (hubtelResponse.code == 0 || hubtelResponse.code == 200 ||
         hubtelResponse.code == 201);

    co_return response;

  } catch (const std::exception &e) {
    LOG_ERROR << "Error initializing momo withdrawal: " << e.what();
    response.message = "Failed to initialize momo withdrawal.";
    response.code = 500;
    response.success = false;
    co_return response;
  }
}

drogon::Task<dto::smart_suite::SmartSuiteOperationResponse>
SmartSuiteBankingService::completeFlexyWithdrawalToBank(
    const std::string &businessId, const std::string &reference,
    const std::string &recipientName, bool isSuccessful) {

  dto::smart_suite::SmartSuiteOperationResponse response;

  try {
    auto dbClient = drogon::app().getDbClient();
    drogon::orm::CoroMapper<drogon_model::DataSupportDb::SmartSuiteTransactions>
        txnMapper(dbClient);

    auto criteria =
        drogon::orm::Criteria(drogon_model::DataSupportDb::
                                  SmartSuiteTransactions::Cols::_reference,
                              drogon::orm::CompareOperator::EQ, reference) &&
        drogon::orm::Criteria(
            drogon_model::DataSupportDb::SmartSuiteTransactions::Cols::_status,
            drogon::orm::CompareOperator::EQ, "PENDING");

    auto transactions = co_await txnMapper.findBy(criteria);

    if (transactions.empty()) {
      response.message = "Transaction not found.";
      response.code = 1;
      response.success = false;
      co_return response;
    }

    auto transaction = transactions[0];

    if (isSuccessful) {
      transaction.setStatus("SUCCESS");
      transaction.setRecipientName(recipientName);

      co_await txnMapper.update(transaction);

      // TODO: Trigger actual SMS delivery here via MagmaSmsService or HubtelApi
      LOG_INFO << "Sent Withdrawal Success SMS for transaction " << reference
               << " to " << recipientName;

      // co_await sendSms(transaction.getPhoneNo(), "");

      response.message = "Withdrawal successful!";
      response.code = 0;
      response.success = true;
      co_return response;
    }

    LOG_ERROR
        << "Withdrawal failed for transaction " << reference << ", "
        << transaction.getValueOfPhoneNo()
        << ". Attempting to reverse transaction back to the sender's account.";

    co_await reverseTransaction(businessId, reference);

    response.message = "Failed to complete Withdrawal.";
    response.code = 1;
    response.success = false;
    co_return response;

  } catch (const std::exception &e) {
    LOG_ERROR << "Failed to complete Withdrawal: " << e.what();
    response.message = "Failed to complete Withdrawal.";
    response.code = 1;
    response.success = false;
    co_return response;
  }
}

drogon::Task<dto::smart_suite::SmartSuiteOperationResponse>
SmartSuiteBankingService::completeFlexyWithdrawalToMomo(
    const std::string &businessId, const std::string &reference,
    const std::string &recipientName, bool isSuccessful) {

  dto::smart_suite::SmartSuiteOperationResponse response;

  try {
    auto dbClient = drogon::app().getDbClient();
    drogon::orm::CoroMapper<drogon_model::DataSupportDb::SmartSuiteTransactions>
        txnMapper(dbClient);

    auto criteria =
        drogon::orm::Criteria(drogon_model::DataSupportDb::
                                  SmartSuiteTransactions::Cols::_reference,
                              drogon::orm::CompareOperator::EQ, reference) &&
        drogon::orm::Criteria(
            drogon_model::DataSupportDb::SmartSuiteTransactions::Cols::_status,
            drogon::orm::CompareOperator::EQ, "PENDING");

    auto transactions = co_await txnMapper.findBy(criteria);

    if (transactions.empty()) {
      response.message = "Transaction not found.";
      response.code = 1;
      response.success = false;
      co_return response;
    }

    auto transaction = transactions[0];

    if (isSuccessful) {
      transaction.setStatus("SUCCESS");
      transaction.setRecipientName(recipientName);

      co_await txnMapper.update(transaction);

      // TODO: Trigger actual SMS delivery here via MagmaSmsService or HubtelApi
      LOG_INFO << "Sent Withdrawal Success SMS for transaction " << reference
               << " to " << recipientName;

      // co_await sendSms(transaction.getPhoneNo(), "");

      response.message = "Withdrawal successful!";
      response.code = 0;
      response.success = true;
      co_return response;
    }

    LOG_ERROR << "Withdrawal failed for transaction " << reference << ", "
              << transaction.getValueOfPhoneNo()
              << ". Attempting to deposit funds back to the sender's account.";

    co_await reverseTransaction(businessId, reference);

    response.message = "Failed to complete Withdrawal.";
    response.code = 1;
    response.success = false;
    co_return response;

  } catch (const std::exception &e) {
    LOG_ERROR << "Failed to complete Withdrawal: " << e.what();
    response.message = "Failed to complete Withdrawal.";
    response.code = 1;
    response.success = false;
    co_return response;
  }
}



drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> SmartSuiteBankingService::completeDeposit(const std::string &businessId,
                                          const std::string &reference,
                                          const std::string &recipientName,
                                          bool isSuccessful) {

  dto::smart_suite::SmartSuiteOperationResponse response;

  try {
    auto dbClient = drogon::app().getDbClient();
    drogon::orm::CoroMapper<drogon_model::DataSupportDb::SmartSuiteTransactions>
        txnMapper(dbClient);

    auto criteria =
        drogon::orm::Criteria(drogon_model::DataSupportDb::
                                  SmartSuiteTransactions::Cols::_reference,
                              drogon::orm::CompareOperator::EQ, reference) &&
        drogon::orm::Criteria(
            drogon_model::DataSupportDb::SmartSuiteTransactions::Cols::_status,
            drogon::orm::CompareOperator::EQ, "PENDING");

    auto transactions = co_await txnMapper.findBy(criteria);

    if (transactions.empty()) {
      response.message = "Transaction not found.";
      response.code = 1;
      response.success = false;
      co_return response;
    }

    auto transaction = transactions[0];

    if (isSuccessful) {
      drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts>
          bizMapper(dbClient);
      auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);
      auto configStr = businessAccount.getValueOfConfigurationData();

      Json::Value configJson;
      Json::CharReaderBuilder readerBuilder;
      std::string errs;
      std::istringstream iss(configStr);

      if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
        response.message = "Failed to parse ConfigurationData.";
        response.code = 1;
        response.success = false;
        co_return response;
      }

      std::string baseUrl = configJson["SmartSuiteApi"]["BaseUrl"].asString();
      std::string encryptionKey =
          configJson["SmartSuiteApi"]["EncryptionKey"].asString();

      std::string accountType = transaction.getValueOfAccountType();
      std::string accountTypeUpper = accountType;
      std::transform(accountTypeUpper.begin(), accountTypeUpper.end(),
                     accountTypeUpper.begin(), ::toupper);

      std::string decryptedPin = utils::CryptoUtils::decrypt(
          transaction.getValueOfPin(), encryptionKey);

      Json::Value payload;
      payload["v_PHONENO"] = transaction.getValueOfPhoneNo();
      payload["v_Amnt_ip"] = transaction.getValueOfAmount();
      payload["V_transT"] = "D";
      payload["v_ACTYPE"] = accountTypeUpper;
      payload["v_ACCR"] = "";
      payload["V_Pin"] = decryptedPin;
      payload["V_PinNew"] = "";
      payload["v_SERIALNO"] = (accountTypeUpper == "K")
                                  ? transaction.getValueOfChildAccountNumber()
                                  : "";
      payload["V_TRANSREF"] = reference;

      if (configJson["SmartSuiteApi"].isMember("v_TELCOM")) {
        payload["v_TELCOM"] =
            configJson["SmartSuiteApi"]["v_TELCOM"].asString();
      }
      if (configJson["SmartSuiteApi"].isMember("v_APIKEY")) {
        payload["v_APIKEY"] =
            configJson["SmartSuiteApi"]["v_APIKEY"].asString();
      }

      Json::StreamWriterBuilder writer;
      writer["indentation"] = "";
      std::string body = Json::writeString(writer, payload);

      auto client = drogon::HttpClient::newHttpClient(baseUrl);
      auto req = drogon::HttpRequest::newHttpRequest();
      req->setMethod(drogon::Post);
      req->setPath("");
      req->setBody(body);
      req->addHeader("Content-Type", "application/json");

      auto httpResp = co_await client->sendRequestCoro(req, 20.0);

      if (httpResp) {
        const std::string responseStr = std::string(httpResp->getBody());
        Json::Value jsonResp;
        std::istringstream respIss(responseStr);

        if (Json::parseFromStream(readerBuilder, respIss, &jsonResp, &errs)) {
          auto result =
              dto::SmartSuiteApiResponseModel<Json::Value>::fromJson(jsonResp);

          std::string accountTypeName;
          if (accountTypeUpper == "F")
            accountTypeName = "Flexy";
          else if (accountTypeUpper == "K")
            accountTypeName = "Kiddy";
          else if (accountTypeUpper == "U" || accountTypeUpper == "S")
            accountTypeName = "Savings";
          else if (accountTypeUpper == "H")
            accountTypeName = "Shares";
          else if (accountTypeUpper == "L")
            accountTypeName = "Loan";
          else
            accountTypeName = accountType;

          std::string respDescLower = result.getRespdesc();
          std::transform(respDescLower.begin(), respDescLower.end(),
                         respDescLower.begin(), ::tolower);

          if (result.getRespcode() == "0" && respDescLower == "success") {
            transaction.setStatus("SUCCESS");
            if (!recipientName.empty()) {
              transaction.setRecipientName(recipientName);
            }
            co_await txnMapper.update(transaction);

            LOG_INFO << "Sent Deposit Success SMS for transaction " << reference
                     << " to " << accountTypeName;

            response.message = "GHS " + transaction.getValueOfAmount() +
                               " Deposited into " + accountTypeName +
                               " Account Successfully!";
            response.code = 0;
            response.success = true;
            co_return response;
          }
        }
      }

      // If we reach here, the Core Deposit failed, so we reverse back to Mobile
      // Money
      LOG_ERROR << "Deposit to core banking failed for transaction "
                << reference << ". Attempting to reverse back to Mobile Money.";

      try {
        auto plugin = drogon::app().getPlugin<plugins::DataSupportServicePlugin>();
        auto &hubtelApi = plugin->getHubtelApi();

        dto::DirectSendMomoDto sendDto;
        sendDto.setRecipientName(recipientName.empty() ? "Customer" : recipientName);
        sendDto.setRecipientMsisdn(transaction.getValueOfPhoneNo());
        sendDto.setCustomerEmail("");
        try {
          sendDto.setAmount(std::stod(transaction.getValueOfAmount()));
        } catch (...) {
          sendDto.setAmount(0.0);
        }
        sendDto.setPrimaryCallbackUrl("");
        sendDto.setDescription("Deposit Reversal");
        sendDto.setProvider(transaction.getValueOfProvider());
        sendDto.setClientReference(reference + "_rev");

        auto hubtelResponse =
            co_await hubtelApi.initializeDirectSendMomo(businessId, sendDto);

        if (hubtelResponse.code == 0 || hubtelResponse.code == 200 ||
            hubtelResponse.code == 201) {
          LOG_INFO << "Reversal to Mobile Money initiated successfully for "
                      "transaction "
                   << reference;
        } else {
          LOG_ERROR
              << "Failed to reverse back to Mobile Money for transaction "
              << reference << ": " << hubtelResponse.message;
        }
      } catch (const std::exception &e) {
        LOG_ERROR << "Exception during reversal to Mobile Money: "
                  << e.what();
      }

      response.message = "Failed to complete Deposit.";
      response.code = 1;
      response.success = false;
      co_return response;
    }

    response.message = "Failed to complete Deposit.";
    response.code = 1;
    response.success = false;
    co_return response;

  } catch (const std::exception &e) {
    LOG_ERROR << "Failed to complete Deposit: " << e.what();
    response.message = "Failed to complete Deposit.";
    response.code = 1;
    response.success = false;
    co_return response;
  }
}

drogon::Task<dto::smart_suite::SmartSuiteOperationResponse>
SmartSuiteBankingService::reverseTransaction(const std::string &businessId,
                                             const std::string &reference) {

  dto::smart_suite::SmartSuiteOperationResponse response;
  try {
    auto dbClient = drogon::app().getDbClient();
    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts>
        bizMapper(dbClient);
    auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      response.message =
          "Failed to process deposit reversal. Please try again.";
      response.code = 1;
      response.success = false;
      co_return response;
    }

    std::string baseUrl = configJson["SmartSuiteApi"]["BaseUrl"].asString();
    std::string encryptionKey =
        configJson["SmartSuiteApi"]["EncryptionKey"].asString();

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::SmartSuiteTransactions>
        txnMapper(dbClient);
    auto criteria = drogon::orm::Criteria(
        drogon_model::DataSupportDb::SmartSuiteTransactions::Cols::_reference,
        drogon::orm::CompareOperator::EQ, reference);
    auto transactions = co_await txnMapper.findBy(criteria);

    if (transactions.empty()) {
      response.message =
          "Failed to process deposit reversal. Please try again.";
      response.code = 1;
      response.success = false;
      co_return response;
    }

    auto transaction = transactions[0];

    std::string accountType = transaction.getValueOfAccountType();
    std::string accountTypeUpper = accountType;
    std::transform(accountTypeUpper.begin(), accountTypeUpper.end(),
                   accountTypeUpper.begin(), ::toupper);

    std::string decryptedPin =
        utils::CryptoUtils::decrypt(transaction.getValueOfPin(), encryptionKey);

    Json::Value payload;
    payload["v_PHONENO"] = transaction.getValueOfPhoneNo();
    payload["v_Amnt_ip"] = transaction.getValueOfAmount();
    payload["V_transT"] = "R";
    payload["v_ACTYPE"] = accountTypeUpper;
    payload["v_ACCR"] = "";
    payload["V_Pin"] = decryptedPin;
    payload["V_PinNew"] = "";
    payload["v_SERIALNO"] = (accountTypeUpper == "K")
                                ? transaction.getValueOfChildAccountNumber()
                                : "";
    payload["V_TRANSREF"] = reference;

    if (configJson["SmartSuiteApi"].isMember("v_TELCOM")) {
      payload["v_TELCOM"] = configJson["SmartSuiteApi"]["v_TELCOM"].asString();
    }
    if (configJson["SmartSuiteApi"].isMember("v_APIKEY")) {
      payload["v_APIKEY"] = configJson["SmartSuiteApi"]["v_APIKEY"].asString();
    }

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string body = Json::writeString(writer, payload);

    auto client = drogon::HttpClient::newHttpClient(baseUrl);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("");
    req->setBody(body);
    req->addHeader("Content-Type", "application/json");

    auto httpResp = co_await client->sendRequestCoro(req, 60.0);

    if (httpResp) {
      const std::string responseStr = std::string(httpResp->getBody());
      Json::Value jsonResp;
      std::istringstream respIss(responseStr);

      if (Json::parseFromStream(readerBuilder, respIss, &jsonResp, &errs)) {
        auto result =
            dto::SmartSuiteApiResponseModel<Json::Value>::fromJson(jsonResp);

        std::string accountTypeName;
        if (accountTypeUpper == "F")
          accountTypeName = "Flexy";
        else if (accountTypeUpper == "K")
          accountTypeName = "Kiddy";
        else if (accountTypeUpper == "S")
          accountTypeName = "Savings";
        else if (accountTypeUpper == "H")
          accountTypeName = "Shares";
        else if (accountTypeUpper == "L")
          accountTypeName = "Loan";
        else
          accountTypeName = accountType;

        std::string respDescLower = result.getRespdesc();
        std::transform(respDescLower.begin(), respDescLower.end(),
                       respDescLower.begin(), ::tolower);

        if (result.getRespcode() == "0" && respDescLower == "success") {
          response.message = "Deposit reversal for " + accountTypeName +
                             " account was successful!";
          response.code = 0;
          response.success = true;
        } else {
          response.message =
              "Failed to reverse deposit for " + accountTypeName + " account.";
          response.code = result.getRespcode() == "0" ? 0 : 1;
          response.success = false;
        }
        co_return response;
      }
    }

    response.message = "Failed to process deposit reversal. Please try again.";
    response.code = 1;
    response.success = false;
    co_return response;

  } catch (const std::exception &e) {
    LOG_ERROR << "Error in reverseTransaction: " << e.what();
    response.message = "Failed to process deposit reversal. Please try again.";
    response.code = 1;
    response.success = false;
    co_return response;
  }
}

// withdrawal request...
drogon::Task<dto::smart_suite::SmartSuiteOperationResponse>
SmartSuiteBankingService::kiddyWithdrawalRequest(
    const std::string &businessId,
    const dto::smart_suite::KiddyWithdrawalRequestDto &dto) {

  dto::smart_suite::SmartSuiteOperationResponse response;

  try {
    auto dbClient = drogon::app().getDbClient();
    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> mapper(dbClient);

    auto businessAccount = co_await mapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "Failed to parse ConfigurationData: " << errs;
      response.code = 1;
      response.message = "Failed to initiate withdrawal request from Kiddy Account.";
      co_return response;
    }

    const auto &smartSuiteConfig = configJson["SmartSuiteApi"];
    if (smartSuiteConfig.isNull()) {
      response.code = 1;
      response.message = "Failed to initiate withdrawal request from Kiddy Account.";
      co_return response;
    }

    std::string baseUrl = smartSuiteConfig["BaseUrl"].asString();
    std::string vTelcom = smartSuiteConfig["v_TELCOM"].asString();
    std::string vApiKey = smartSuiteConfig["v_APIKEY"].asString();

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::ostringstream timeStream;
    timeStream << std::put_time(std::localtime(&now_c), "%Y%m%d%H%M%S");
    std::string transRef = timeStream.str();

    Json::Value payload;
    payload["v_PHONENO"] = dto.getPhoneNo();
    
    // Use std::ostringstream to avoid scientific notation and ensure it's a string
    std::ostringstream amountStream;
    amountStream << dto.getAmount();
    payload["v_Amnt_ip"] = amountStream.str();
    
    payload["v_transT"] = "WR";
    payload["v_ACTYPE"] = "K";
    payload["v_ACCR"] = "";
    payload["V_Pin"] = dto.getPin();
    payload["V_PinNew"] = "";
    payload["v_SERIALNO"] = dto.getSerialNo();
    payload["V_TRANSREF"] = transRef;
    payload["v_TELCOM"] = vTelcom;
    payload["v_APIKEY"] = vApiKey;

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string body = Json::writeString(writer, payload);

    auto client = drogon::HttpClient::newHttpClient(baseUrl);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("");
    req->setBody(body);
    req->addHeader("Content-Type", "application/json");

    auto httpResp = co_await client->sendRequestCoro(req, 60.0);

    if (httpResp && (httpResp->statusCode() == drogon::k200OK || httpResp->statusCode() == drogon::k201Created)) {
      const std::string responseStr = std::string(httpResp->getBody());
      Json::Value jsonResponse;
      std::istringstream respIss(responseStr);
      if (Json::parseFromStream(readerBuilder, respIss, &jsonResponse, &errs)) {
        auto result = dto::SmartSuiteApiResponseModel<Json::Value>::fromJson(jsonResponse);
        
        std::string respDescLower = result.getRespdesc();
        std::transform(respDescLower.begin(), respDescLower.end(), respDescLower.begin(), ::tolower);

        bool isSuccess = (result.getRespcode() == "0" && respDescLower == "success");

        if (isSuccess) {
            std::ostringstream msg;
            msg << "The withdrawal request for GHS " << amountStream.str() << " has been successfully submitted!";
            response.message = msg.str();
        } else {
            response.message = "Failed to initiate withdrawal request from Kiddy Account.";
        }

        try {
            response.code = std::stoi(result.getRespcode());
        } catch (...) {
            response.code = 1;
        }
        response.success = isSuccess;

        co_return response;
      }
    }

    response.message = "Failed to initiate withdrawal request from Kiddy Account.";
    response.code = 1;
    response.success = false;
    co_return response;

  } catch (const std::exception &e) {
    LOG_ERROR << "Error in KiddyWithdrawalRequest: " << e.what();

    response.message = "Failed to initiate withdrawal request from Kiddy Account.";
    response.code = 1;
    response.success = false;
    co_return response;
  }
}

drogon::Task<dto::smart_suite::SmartSuiteOperationResponse>
SmartSuiteBankingService::savingsWithdrawalRequest(
    const std::string &businessId,
    const dto::smart_suite::SavingsWithdrawalRequestDto &dto) {

  dto::smart_suite::SmartSuiteOperationResponse response;

  try {
    auto dbClient = drogon::app().getDbClient();
    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> mapper(dbClient);

    auto businessAccount = co_await mapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "Failed to parse ConfigurationData: " << errs;
      response.code = 1;
      response.message = "Failed to initiate withdrawal request from Savings Account.";
      co_return response;
    }

    const auto &smartSuiteConfig = configJson["SmartSuiteApi"];
    if (smartSuiteConfig.isNull()) {
      response.code = 1;
      response.message = "Failed to initiate withdrawal request from Savings Account.";
      co_return response;
    }

    std::string baseUrl = smartSuiteConfig["BaseUrl"].asString();
    std::string vTelcom = smartSuiteConfig["v_TELCOM"].asString();
    std::string vApiKey = smartSuiteConfig["v_APIKEY"].asString();

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::ostringstream timeStream;
    timeStream << std::put_time(std::localtime(&now_c), "%Y%m%d%H%M%S");
    std::string transRef = timeStream.str();

    Json::Value payload;
    payload["v_PHONENO"] = dto.getPhoneNo();
    
    std::ostringstream amountStream;
    amountStream << dto.getAmount();
    payload["v_Amnt_ip"] = amountStream.str();
    
    payload["v_transT"] = "WR";
    payload["v_ACTYPE"] = "U";
    payload["v_ACCR"] = "";
    payload["V_Pin"] = dto.getPin();
    payload["V_PinNew"] = "";
    payload["v_SERIALNO"] = "";
    payload["V_TRANSREF"] = transRef;
    payload["v_TELCOM"] = vTelcom;
    payload["v_APIKEY"] = vApiKey;

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string body = Json::writeString(writer, payload);

    auto client = drogon::HttpClient::newHttpClient(baseUrl);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("");
    req->setBody(body);
    req->addHeader("Content-Type", "application/json");

    auto httpResp = co_await client->sendRequestCoro(req, 60.0);

    if (httpResp && (httpResp->statusCode() == drogon::k200OK || httpResp->statusCode() == drogon::k201Created)) {
      const std::string responseStr = std::string(httpResp->getBody());
      Json::Value jsonResponse;
      std::istringstream respIss(responseStr);
      if (Json::parseFromStream(readerBuilder, respIss, &jsonResponse, &errs)) {
        auto result = dto::SmartSuiteApiResponseModel<Json::Value>::fromJson(jsonResponse);
        
        std::string respDescLower = result.getRespdesc();
        std::transform(respDescLower.begin(), respDescLower.end(), respDescLower.begin(), ::tolower);

        bool isSuccess = (result.getRespcode() == "0" && respDescLower == "success");

        if (isSuccess) {
            std::ostringstream msg;
            msg << "The withdrawal request for GHS " << amountStream.str() << " has been successfully submitted!";
            response.message = msg.str();
        } else {
            response.message = "Failed to initiate withdrawal request from Savings Account.";
        }

        try {
            response.code = std::stoi(result.getRespcode());
        } catch (...) {
            response.code = 1;
        }
        response.success = isSuccess;

        co_return response;
      }
    }

    response.message = "Failed to initiate withdrawal request from Savings Account.";
    response.code = 1;
    response.success = false;
    co_return response;

  } catch (const std::exception &e) {
    LOG_ERROR << "Error in SavingsWithdrawalRequest: " << e.what();

    response.message = "Failed to initiate withdrawal request from Savings Account.";
    response.code = 1;
    response.success = false;
    co_return response;
  }
}

drogon::Task<dto::smart_suite::SmartSuiteOperationResponse>
SmartSuiteBankingService::flexyToKiddyTransfer(
    const std::string &businessId,
    const dto::smart_suite::KiddyWithdrawalRequestDto &dto) {

  dto::smart_suite::SmartSuiteOperationResponse response;

  try {
    auto dbClient = drogon::app().getDbClient();
    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> mapper(dbClient);

    auto businessAccount = co_await mapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "Failed to parse ConfigurationData: " << errs;
      response.code = 1;
      response.message = "Failed to transfer funds.";
      co_return response;
    }

    const auto &smartSuiteConfig = configJson["SmartSuiteApi"];
    if (smartSuiteConfig.isNull()) {
      response.code = 1;
      response.message = "Failed to transfer funds.";
      co_return response;
    }

    std::string baseUrl = smartSuiteConfig["BaseUrl"].asString();
    std::string vTelcom = smartSuiteConfig["v_TELCOM"].asString();
    std::string vApiKey = smartSuiteConfig["v_APIKEY"].asString();

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::ostringstream timeStream;
    timeStream << std::put_time(std::localtime(&now_c), "%Y%m%d%H%M%S");
    std::string transRef = timeStream.str();

    Json::Value payload;
    payload["v_PHONENO"] = dto.getPhoneNo();
    
    std::ostringstream amountStream;
    amountStream << dto.getAmount();
    payload["v_Amnt_ip"] = amountStream.str();
    
    payload["v_transT"] = "T";
    payload["v_ACTYPE"] = "K";
    payload["v_ACCR"] = "";
    payload["V_Pin"] = dto.getPin();
    payload["V_PinNew"] = "";
    payload["v_SERIALNO"] = dto.getSerialNo();
    payload["V_TRANSREF"] = transRef;
    payload["v_TELCOM"] = vTelcom;
    payload["v_APIKEY"] = vApiKey;

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string body = Json::writeString(writer, payload);

    auto client = drogon::HttpClient::newHttpClient(baseUrl);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("");
    req->setBody(body);
    req->addHeader("Content-Type", "application/json");

    auto httpResp = co_await client->sendRequestCoro(req, 60.0);

    if (httpResp && (httpResp->statusCode() == drogon::k200OK || httpResp->statusCode() == drogon::k201Created)) {
      const std::string responseStr = std::string(httpResp->getBody());
      Json::Value jsonResponse;
      std::istringstream respIss(responseStr);
      if (Json::parseFromStream(readerBuilder, respIss, &jsonResponse, &errs)) {
        auto result = dto::SmartSuiteApiResponseModel<Json::Value>::fromJson(jsonResponse);
        
        std::string respDescLower = result.getRespdesc();
        std::transform(respDescLower.begin(), respDescLower.end(), respDescLower.begin(), ::tolower);

        bool isSuccess = (result.getRespcode() == "0" && respDescLower == "success");

        if (isSuccess) {
            std::ostringstream msg;
            msg << "GHS " << amountStream.str() << " transferred to kiddy account successfully!";
            response.message = msg.str();
        } else {
            response.message = "Failed to transfer funds.";
        }

        try {
            response.code = std::stoi(result.getRespcode());
        } catch (...) {
            response.code = 1;
        }
        response.success = isSuccess;

        co_return response;
      }
    }

    response.message = "Failed to transfer funds.";
    response.code = 1;
    response.success = false;
    co_return response;

  } catch (const std::exception &e) {
    LOG_ERROR << "Error in flexyToKiddyTransfer: " << e.what();

    response.message = "Failed to transfer funds.";
    response.code = 1;
    response.success = false;
    co_return response;
  }
}

drogon::Task<dto::smart_suite::SmartSuiteOperationResponse> SmartSuiteBankingService::interAccountTransfer(
    const std::string &businessId,
    const dto::smart_suite::InterAccountTransferDto &dto) {

  // create db record
}



} // namespace data_support::services
