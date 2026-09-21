//
// Created by Emmanuel Addo-Odame on 26/07/2026.
//

#include "HubtelApi.h"

#include <drogon/orm/CoroMapper.h>

#include "BusinessAccounts.h"
#include "constants/TransactionStatusCode.h"
#include "plugins/DataSupportServicePlugin.h"

namespace data_support::services {


drogon::Task<dto::TransactionResponse> HubtelApi::initializeDirectReceiveMoney(const std::string &businessId, const dto::DirectReceiveMoneyDto &dto) {

  dto::TransactionResponse response;

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
      response.message = "Configuration error. Payment Failed !";
      co_return response;
    }

    const auto &hubtelConfig = configJson["HubtelPaymentApi"];
    if (hubtelConfig.isNull()) {
      response.code = 1;
      response.message = "Hubtel configuration missing. Payment Failed !";
      co_return response;
    }

    std::string username = hubtelConfig["ApiIdOrUsername"].asString();
    std::string password = hubtelConfig["ApiKeyOrPassword"].asString();
    std::string posSalesAccountId = hubtelConfig["PosSalesAccountId"].asString();

    std::string provider = dto.getProvider();
    std::transform(provider.begin(), provider.end(), provider.begin(), ::tolower);
    
    if (provider.find("mtn") != std::string::npos) {
      provider = "mtn-gh";
    } else if (provider.find("vodafone") != std::string::npos) {
      provider = "vodafone-gh";
    } else if (provider.find("airtel") != std::string::npos || provider.find("tigo") != std::string::npos) {
      provider = "tigo-gh";
    }

    std::string promptMessage = "Authorize Payment by entering PIN on your mobile device. ";
    if (provider == "mtn-gh") {
      promptMessage += "If no bill prompt appears, dial *170# to resume.";
    } else if (provider == "vodafone-gh" || provider == "tigo-gh") {
      promptMessage += "If no bill prompt appears, dial *110# to resume.";
    }

    Json::Value payload;
    payload["CustomerName"] = dto.getCustomerName();
    payload["CustomerMsisdn"] = dto.getCustomerMsisdn();
    payload["CustomerEmail"] = dto.getCustomerEmail();
    payload["Channel"] = provider;
    payload["Amount"] = dto.getAmount();
    
    payload["PrimaryCallbackUrl"] = dto.getPrimaryCallbackUrl();
    payload["Description"] = dto.getDescription();
    payload["ClientReference"] = dto.getClientReference();

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string body = Json::writeString(writer, payload);

    std::string auth = username + ":" + password;
    std::string authHeader = "Basic " + drogon::utils::base64Encode((const unsigned char*)auth.data(), auth.length());

      // Fire-and-forget: detached async task that owns all its data
      drogon::async_run([posSalesAccountId, body, authHeader]() -> drogon::Task<> {
          // This runs independently — no connection to the original coroutine
          co_await drogon::sleepCoro(drogon::app().getLoop(), 5.0);

          auto client = drogon::HttpClient::newHttpClient("https://rmp.hubtel.com");
          auto req = drogon::HttpRequest::newHttpRequest();
          req->setMethod(drogon::Post);
          req->setPath("/merchantaccount/merchants/" + posSalesAccountId + "/receive/mobilemoney");
          req->setContentTypeCode(drogon::CT_APPLICATION_JSON);
          req->setBody(body);
          req->addHeader("Authorization", authHeader);

        try {
            auto httpResp = co_await client->sendRequestCoro(req, 10.0);

            if (httpResp == nullptr) {
                LOG_ERROR << "Delayed Hubtel API call failed. Null response.";
                co_return;
            }

            if (httpResp->getStatusCode() == drogon::k200OK ||
                httpResp->getStatusCode() == drogon::k201Created) {
                LOG_INFO << "Delayed Hubtel API call success: "
                         << httpResp->getBody();
            } else {
                LOG_ERROR << "Delayed Hubtel API call failed. Status: "
                          << httpResp->getStatusCode()
                          << ", body: " << httpResp->getBody();
            }
        } catch (const std::exception &e) {
            LOG_ERROR << "Delayed Hubtel API call failed with exception: " << e.what();
        }

       co_return;

      });

    response.code = 0;
    response.message = promptMessage;
    response.clientReference = dto.getClientReference();
    co_return response;

  } catch (const std::exception &e) {
    LOG_ERROR << "Error initializing Hubtel payment: " << e.what();
    response.clientReference = "";
    response.code = 1;
    response.message = "Failed to initiate transaction. Please try again.";
    co_return response;
  }
}




drogon::Task<dto::TransactionResponse> HubtelApi::initializeWebCheckout(const std::string &businessId, const dto::HubtelWebCheckoutDto &dto) {

    dto::TransactionResponse response;
    
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
            response.clientReference = dto.getClientReference();
            response.code = 1;
            response.message = "Failed to initiate web_checkout. Configuration error.";
            co_return response;
        }

        const auto &hubtelConfig = configJson["HubtelPaymentApi"];
        if (hubtelConfig.isNull()) {
            response.clientReference = dto.getClientReference();
            response.code = 1;
            response.message = "Failed to initiate web_checkout. Hubtel configuration missing.";
            co_return response;
        }

        std::string username = hubtelConfig["ApiIdOrUsername"].asString();
        std::string password = hubtelConfig["ApiKeyOrPassword"].asString();
        std::string merchantAccountNumber = hubtelConfig["MerchantAccountNumber"].asString();

        Json::Value payload;
        payload["totalAmount"] = dto.getTotalAmount();
        payload["returnUrl"] = dto.getReturnUrl();
        payload["merchantAccountNumber"] = merchantAccountNumber;

        payload["callbackUrl"] = dto.getCallbackUrl();
        payload["cancellationUrl"] = dto.getCancellationUrl();
        payload["description"] = dto.getDescription();
        payload["clientReference"] = dto.getClientReference();

        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        std::string body = Json::writeString(writer, payload);

        std::string auth = username + ":" + password;
        std::string authHeader = "Basic " + drogon::utils::base64Encode((const unsigned char*)auth.data(), auth.length());

        auto client = drogon::HttpClient::newHttpClient("https://payproxyapi.hubtel.com");
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Post);
        req->setPath("/items/initiate");
        req->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        req->setBody(body);
        req->addHeader("Authorization", authHeader);

        auto httpResp = co_await client->sendRequestCoro(req);

        if (httpResp && (httpResp->statusCode() == drogon::k200OK || httpResp->statusCode() == drogon::k201Created)) {
            const std::string responseStr = std::string(httpResp->getBody());
            
            Json::Value jsonResponse;
            std::istringstream respIss(responseStr);
            if (Json::parseFromStream(readerBuilder, respIss, &jsonResponse, &errs)) {
                auto result = dto::HubtelWebCheckoutResponse::fromJson(jsonResponse);
                
                LOG_INFO << "InitializeCheckout response => " << responseStr << ", reference: " << dto.getClientReference();

                response.clientReference = dto.getClientReference();
                response.code = (result.responseCode == constants::TransactionStatusCode::HUBTEL_TRANSACTION_SUCCESS_CODE) ? 0 : 1;
                response.checkoutUrl = result.data.checkoutUrl;
                response.message = (result.responseCode == constants::TransactionStatusCode::HUBTEL_TRANSACTION_SUCCESS_CODE) ? "Web Checkout initialized successfully." : "Transaction failed.";
                co_return response;
            }
        }
        
        response.clientReference = dto.getClientReference();
        response.code = 1;
        response.checkoutUrl = "";
        response.message = "Failed to initiate web_checkout check. Please try again.";
        co_return response;

    } catch (const std::exception &e) {
        LOG_ERROR << "Error! Failed to initiate Hubtel web_checkout. " << e.what();
        response.clientReference = "";
        response.code = 1;
        response.checkoutUrl = "";
        response.message = "Failed to initiate Hubtel web_checkout. Please try again.";
        co_return response;
    }
}


drogon::Task<dto::TransactionResponse> HubtelApi::transactionStatusCheck(const std::string &businessId, const std::string &paymentReference) {

    dto::TransactionResponse response;
    response.clientReference = paymentReference;
    
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
            response.message = "Failed to initiate transaction status check. Configuration error.";
            co_return response;
        }

        const auto &hubtelConfig = configJson["HubtelPaymentApi"];
        if (hubtelConfig.isNull()) {
            response.code = 1;
            response.message = "Failed to initiate transaction status check. Hubtel configuration missing.";
            co_return response;
        }

        std::string username = hubtelConfig["ApiIdOrUsername"].asString();
        std::string password = hubtelConfig["ApiKeyOrPassword"].asString();
        std::string posSalesAccountId = hubtelConfig["PosSalesAccountId"].asString();

        std::string auth = username + ":" + password;
        std::string authHeader = "Basic " + drogon::utils::base64Encode((const unsigned char*)auth.data(), auth.length());

        auto client = drogon::HttpClient::newHttpClient("https://api-txnstatus.hubtel.com");
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Get);
        req->setPath("/transactions/" + posSalesAccountId + "/status");
        req->setParameter("clientReference", paymentReference);
        req->addHeader("Content-Type", "application/json");
        req->addHeader("Authorization", authHeader);

        auto httpResp = co_await client->sendRequestCoro(req);

        if (httpResp && (httpResp->statusCode() == drogon::k200OK || httpResp->statusCode() == drogon::k201Created)) {
            const std::string responseStr = std::string(httpResp->getBody());
            
            Json::Value jsonResponse;
            std::istringstream respIss(responseStr);
            if (Json::parseFromStream(readerBuilder, respIss, &jsonResponse, &errs)) {
                auto result = dto::TransactionStatusResponse::fromJson(jsonResponse);
                
                LOG_INFO << "DirectReceiveMoneyStatusCheck response => " << responseStr << ", reference: " << paymentReference;

                response.code = (result.responseCode == constants::TransactionStatusCode::HUBTEL_TRANSACTION_SUCCESS_CODE) ? 0 : 1;
                response.message = (result.responseCode == constants::TransactionStatusCode::HUBTEL_TRANSACTION_SUCCESS_CODE) ? "Transaction processed successfully." : "Transaction failed.";
                co_return response;
            }
        }
        
        LOG_INFO << "DirectReceiveMoneyStatusCheck response => " << (httpResp ? httpResp->getBody() : "no response") << ", reference: " << paymentReference;
        response.code = 1;
        response.message = "Failed to initiate transaction status check. Please try again.";
        co_return response;

    } catch (const std::exception &e) {
        LOG_ERROR << "Error! Failed to initiate Hubtel transaction_status_check. " << e.what();
        response.code = 1;
        response.message = "Failed to initiate transaction status check. Please try again.";
        co_return response;
    }
}


drogon::Task<dto::TransactionResponse> HubtelApi::initializeDirectSendToMomo(const std::string &businessId, const dto::DirectSendMomoDto &dto) {

    dto::TransactionResponse response;
    response.clientReference = dto.getClientReference();

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
            response.message = "Failed to initiate withdrawal to mobile money transaction. Configuration error.";
            co_return response;
        }

        const auto &hubtelConfig = configJson["HubtelPaymentApi"];
        if (hubtelConfig.isNull()) {
            response.code = 1;
            response.message = "Failed to initiate withdrawal to mobile money transaction. Hubtel configuration missing.";
            co_return response;
        }

        std::string username = hubtelConfig["ApiIdOrUsername"].asString();
        std::string password = hubtelConfig["ApiKeyOrPassword"].asString();
        std::string prepaidAccountId = hubtelConfig["PrepaidAccountId"].asString();

        std::string provider = dto.getProvider();
        std::transform(provider.begin(), provider.end(), provider.begin(), ::tolower);
        if (provider.find("mtn") != std::string::npos) {
            provider = "mtn-gh";
        } else if (provider.find("vodafone") != std::string::npos) {
            provider = "vodafone-gh";
        } else if (provider.find("airtel") != std::string::npos || provider.find("tigo") != std::string::npos) {
            provider = "tigo-gh";
        }

        std::ostringstream amountStream;
        amountStream << std::fixed << std::setprecision(2) << dto.getAmount();

        Json::Value payload;
        payload["RecipientName"] = dto.getRecipientName();
        payload["RecipientMsisdn"] = dto.getRecipientMsisdn();
        payload["CustomerEmail"] = dto.getCustomerEmail();
        payload["Channel"] = provider;
        payload["Amount"] = std::round(dto.getAmount() * 100.0) / 100.0;
        payload["PrimaryCallbackUrl"] = dto.getPrimaryCallbackUrl();
        payload["Description"] = dto.getDescription();
        payload["ClientReference"] = dto.getClientReference();

        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        std::string body = Json::writeString(writer, payload);

        std::string auth = username + ":" + password;
        std::string authHeader = "Basic " + drogon::utils::base64Encode((const unsigned char*)auth.data(), auth.length());

        auto client = drogon::HttpClient::newHttpClient("https://smp.hubtel.com");
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Post);
        req->setPath("/api/merchants/" + prepaidAccountId + "/send/mobilemoney");
        req->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        req->setBody(body);
        req->addHeader("Authorization", authHeader);

        LOG_INFO << "[initializeDirectSendToMomo] Body: " << req->getBody();
        
        auto httpResp = co_await client->sendRequestCoro(req);

        if (httpResp == nullptr) {
            LOG_ERROR << "[initializeDirectSendToMomo] Hubtel Direct Send Money API "
                         "returned a null response (transport failure). "
                         "Reference: " << dto.getClientReference();
            response.code = 1;
            response.message = "Failed to initiate withdrawal to mobile money transaction. Please try again.";
            co_return response;
        }

        const std::string responseStr = std::string(httpResp->getBody());

        LOG_INFO << "[initializeDirectSendToMomo] Hubtel Direct Send Money API "
         << "status=" << httpResp->statusCode()
         << ", bodyLen=" << responseStr.size()
         << ", body=" << responseStr;
        if (httpResp && (httpResp->statusCode() == drogon::k200OK || httpResp->statusCode() == drogon::k201Created)) {
            const std::string responseStr = std::string(httpResp->getBody());
            
            Json::Value jsonResponse;
            std::istringstream respIss(responseStr);
            if (Json::parseFromStream(readerBuilder, respIss, &jsonResponse, &errs)) {

                auto result = dto::TransactionStatusResponse::fromJson(jsonResponse);
                
                LOG_INFO << "DirectSendMobileMoney response => " << responseStr << ", Reference: " << dto.getClientReference() << ", channel: " << provider << ", amount: " << dto.getAmount() << ", phone: " << dto.getRecipientMsisdn();

                response.code = (result.responseCode == constants::TransactionStatusCode::HUBTEL_TRANSACTION_REQUEST_ACCEPTED) ? 0 : 1;
                response.message = (result.responseCode == constants::TransactionStatusCode::HUBTEL_TRANSACTION_REQUEST_ACCEPTED) ? "Request accepted and being processed." : "Failed to initiate transaction. Please try again.";
                co_return response;
            }
        }
        
        LOG_ERROR << "Failed to initiate direct_send_money. Reference: " << dto.getClientReference() << ", channel: " << provider << ", amount: " << dto.getAmount() << ", phone: " << dto.getRecipientMsisdn();
        response.code = 1;
        response.message = "Failed to initiate withdrawal to mobile money transaction. Please try again.";
        co_return response;

    } catch (const std::exception &e) {
        LOG_ERROR << "Error initializing Hubtel payment: " << e.what();
        response.code = 1;
        response.message = "Failed to initiate withdrawal to mobile money transaction. Please try again.";
        co_return response;
    }
}


drogon::Task<dto::TransactionResponse> HubtelApi::initializeDirectSendToBank(const std::string &businessId, const dto::DirectSendToBankDto &dto) {

    dto::TransactionResponse response;
    response.clientReference = dto.getClientReference();

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
            response.message = "Failed to initiate transaction. Configuration error.";
            co_return response;
        }

        const auto &hubtelConfig = configJson["HubtelPaymentApi"];
        if (hubtelConfig.isNull()) {
            response.code = 1;
            response.message = "Failed to initiate transaction. Hubtel configuration missing.";
            co_return response;
        }

        std::string bankCode;
        std::string targetBankName = dto.getBankName();
        std::string targetBankNameLower = targetBankName;
        std::transform(targetBankNameLower.begin(), targetBankNameLower.end(), targetBankNameLower.begin(), ::tolower);

        const auto &bankCodesList = hubtelConfig["BankCodes"];

        if (bankCodesList.isArray()) {
            for (const auto &b : bankCodesList) {
                std::string currentBankChannel = b["BankChannel"].asString();
                std::string currentBankChannelLower = currentBankChannel;
                std::transform(currentBankChannelLower.begin(), currentBankChannelLower.end(), currentBankChannelLower.begin(), ::tolower);
                
                if (currentBankChannelLower == targetBankNameLower) {
                    bankCode = b["BankCode"].asString();
                    break;
                }
            }
        }

        LOG_INFO << "DirectSendToBank bankCode => " << bankCode;

        std::string username = hubtelConfig["ApiIdOrUsername"].asString();
        std::string password = hubtelConfig["ApiKeyOrPassword"].asString();
        std::string prepaidAccountId = hubtelConfig["PrepaidAccountId"].asString();

        std::string callbackUrl = dto.getPrimaryCallbackUrl() + "/" + businessId;
        if (callbackUrl.empty()) {
            callbackUrl = "https://ds-api.rhyoliteprime.com/api/v1/hubtel-callback-processor/smart-suite/direct-send-to-bank/"+businessId;
        }

        Json::Value payload;
        payload["BankName"] = dto.getBankName();
        payload["BankBranch"] = dto.getBankBranch();
        payload["BankBranchCode"] = dto.getBankBranchCode();
        payload["BankAccountNumber"] = dto.getBankAccountNumber();
        payload["BankAccountName"] = dto.getBankAccountName();
        payload["Amount"] = dto.getAmount();
        payload["PrimaryCallbackUrl"] = callbackUrl;
        payload["Description"] = dto.getDescription();
        payload["RecipientPhoneNumber"] = dto.getRecipientPhoneNumber();
        payload["ClientReference"] = dto.getClientReference();

        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        std::string body = Json::writeString(writer, payload);

        std::string auth = username + ":" + password;
        std::string authHeader = "Basic " + drogon::utils::base64Encode((const unsigned char*)auth.data(), auth.length());

        auto client = drogon::HttpClient::newHttpClient("https://smp.hubtel.com");
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Post);
        req->setPath("/api/merchants/" + prepaidAccountId + "/send/bank/gh/" + bankCode);
        req->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        req->setBody(body);
        req->addHeader("Authorization", authHeader);
        
        auto httpResp = co_await client->sendRequestCoro(req);

        if (httpResp && (httpResp->statusCode() == drogon::k200OK || httpResp->statusCode() == drogon::k201Created)) {
            const std::string responseStr = std::string(httpResp->getBody());
            
            Json::Value jsonResponse;
            std::istringstream respIss(responseStr);
            if (Json::parseFromStream(readerBuilder, respIss, &jsonResponse, &errs)) {
                auto result = dto::TransactionStatusResponse::fromJson(jsonResponse);
                
                LOG_INFO << "DirectSendToBank response => " << responseStr << ", Reference: " << dto.getClientReference() << ", bank: " << dto.getBankName() << ", amount: " << dto.getAmount() << ", phone: " << dto.getRecipientPhoneNumber();

                response.code = (result.responseCode == constants::TransactionStatusCode::HUBTEL_TRANSACTION_REQUEST_ACCEPTED) ? 0 : 1;
                response.message = (result.responseCode == constants::TransactionStatusCode::HUBTEL_TRANSACTION_REQUEST_ACCEPTED) ? "Request accepted and being processed." : "Failed to initiate transaction. Please try again.";
                co_return response;
            }
        }
        
        response.code = 1;
        response.message = "Failed to initiate transaction. Please try again.";
        co_return response;

    } catch (const std::exception &e) {
        LOG_ERROR << "Error initializing Hubtel send_to_bank payment: " << e.what();
        response.code = 1;
        response.message = "Failed to initiate send to bank transaction. Please try again.";
        co_return response;
    }
}


drogon::Task<dto::TransactionResponse> HubtelApi::getAccountBalance(const std::string &businessId, const std::string &accountType) {

    dto::TransactionResponse response;

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
            response.message = "Failed to initiate transaction. Configuration error.";
            co_return response;
        }

        const auto &hubtelConfig = configJson["HubtelPaymentApi"];
        if (hubtelConfig.isNull()) {
            response.code = 1;
            response.message = "Failed to initiate transaction. Hubtel configuration missing.";
            co_return response;
        }

        std::string username = hubtelConfig["ApiIdOrUsername"].asString();
        std::string password = hubtelConfig["ApiKeyOrPassword"].asString();
        std::string prepaidAccountId = hubtelConfig["PrepaidAccountId"].asString();
        std::string posSalesAccountId = hubtelConfig["PosSalesAccountId"].asString();

        std::string auth = username + ":" + password;
        std::string authHeader = "Basic " + drogon::utils::base64Encode((const unsigned char*)auth.data(), auth.length());

        auto client = drogon::HttpClient::newHttpClient("https://trnf.hubtel.com");
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Get);

        req->setPath(accountType == "prepaid"  ? "/api/inter-transfers/prepaid/" + prepaidAccountId  : "/api/inter-transfers/" + posSalesAccountId);

        req->setContentTypeCode(drogon::CT_APPLICATION_JSON);

        req->addHeader("Authorization", authHeader);

        auto httpResp = co_await client->sendRequestCoro(req);

        if (httpResp && (httpResp->statusCode() == drogon::k200OK || httpResp->statusCode() == drogon::k201Created)) {
            const std::string responseStr = std::string(httpResp->getBody());

            Json::Value jsonResponse;
            std::istringstream respIss(responseStr);
            if (Json::parseFromStream(readerBuilder, respIss, &jsonResponse, &errs)) {
                auto result = dto::AccountBalanceResponse::fromJson(jsonResponse);

                response.code = (result.responseCode == constants::TransactionStatusCode::HUBTEL_TRANSACTION_SUCCESS_CODE) ? 0 : 1;
                response.message = (result.responseCode == constants::TransactionStatusCode::HUBTEL_TRANSACTION_SUCCESS_CODE) ? "Balance Retrieved Successfully" : "Failed to retrieved balance.";
                co_return response;
            }

        }

        response.code = 1;
        response.message = "Failed to retrieved balance. Please try again.";
        co_return response;

    } catch (const std::exception &e) {
        LOG_ERROR << "Error retrieving Hubtel " + accountType + "account balance" << e.what();
        response.code = 1;
        response.message = "Error retrieving Hubtel." + accountType + "account balance";
        co_return response;
    }


}

drogon::Task<bool> HubtelApi::sendSms(const std::string &phoneNumber, const std::string &clientSecret, const std::string &clientId,
                                      const std::string &senderId,
                                      const std::string &messageContent) {

  LOG_INFO << "smsMessage => " << messageContent;

  auto client = drogon::HttpClient::newHttpClient("https://sms.hubtel.com");

  // Construct the path with query parameters
  auto req = drogon::HttpRequest::newHttpRequest();
  req->setPath("/v1/messages/send");
  req->setParameter("clientsecret", clientSecret);
  req->setParameter("clientid", clientId);
  req->setParameter("from", senderId);
  req->setParameter("to", phoneNumber);
  req->setParameter("content", messageContent);
  req->setMethod(drogon::Get);

  try {

    auto resp = co_await client->sendRequestCoro(req);

    // The API returns the response body which we should log
    std::string smsNotificationResponse = std::string(resp->getBody());
    LOG_INFO << "hubtelSmsApiResponse => " << smsNotificationResponse
             << ",phoneNo => " << phoneNumber;

    if (resp->getStatusCode() != drogon::k201Created) {
      LOG_ERROR << "Hubtel SMS request failed. Status: "
                << resp->getStatusCode();
      co_return false;
    }
    co_return true;

  } catch (const std::exception &e) {
    LOG_ERROR << "Exception sending SMS: " << e.what();
    co_return false;
  }
}

} // namespace data_support::services
