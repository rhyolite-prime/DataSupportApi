//
// Created by Emmanuel Addo-Odame on 23/08/2026.
//
#include "ErpApi.h"
#include "dto/BaseApiResponse.h"
#include "models/BusinessAccounts.h"
#include <drogon/drogon.h>
#include <drogon/orm/CoroMapper.h>

#include "ErpCurrencies.h"
#include "PosBranches.h"
#include "PosCoupons.h"
#include "PosDeliveryChannels.h"
#include "PosInventoryCategories.h"
#include "PosInventoryItems.h"
#include "PosPickupPoints.h"
#include "utils/JsonUtils.h"

namespace {

// Max number of *dynamic* filter parameters we allow. Current filters are:
//   businessId, keywords, categoryId, categorySlug  →  4
// If you add more filters, bump this constant and add the corresponding case.
constexpr std::size_t kMaxDynamicParams = 4;

// Binds a std::vector<std::string> of filter values into execSqlCoro,
// followed by any extra trailing parameters (e.g. pageSize, offset).
template <typename... Tail>
drogon::Task<drogon::orm::Result>
runSql(const drogon::orm::DbClientPtr &client, const std::string &sql,
       const std::vector<std::string> &dynamicParams, Tail &&...tail) {
  switch (dynamicParams.size()) {
  case 0:
    co_return co_await client->execSqlCoro(sql, std::forward<Tail>(tail)...);
  case 1:
    co_return co_await client->execSqlCoro(sql, dynamicParams[0],
                                           std::forward<Tail>(tail)...);
  case 2:
    co_return co_await client->execSqlCoro(
        sql, dynamicParams[0], dynamicParams[1], std::forward<Tail>(tail)...);
  case 3:
    co_return co_await client->execSqlCoro(sql, dynamicParams[0],
                                           dynamicParams[1], dynamicParams[2],
                                           std::forward<Tail>(tail)...);
  case 4:
    co_return co_await client->execSqlCoro(
        sql, dynamicParams[0], dynamicParams[1], dynamicParams[2],
        dynamicParams[3], std::forward<Tail>(tail)...);
  default:
    throw std::runtime_error("runSql: too many dynamic parameters (increase "
                             "kMaxDynamicParams and add a case)");
  }
}

} // namespace

namespace data_support::services {

drogon::Task<dto::BaseApiResponse> ErpApi::getBranches() {

  // rely on synced data
  dto::BaseApiResponse res;
  co_return res;
}

drogon::Task<dto::BaseApiResponse> ErpApi::getDeliveryChannels() {

  // rely on synced data
  dto::BaseApiResponse res;
  co_return res;
}

drogon::Task<dto::BaseApiResponse> ErpApi::getCoupons() {

  // rely on synced data
  dto::BaseApiResponse res;
  co_return res;
}

drogon::Task<dto::BaseApiResponse> ErpApi::getPickupPoints() {
  dto::BaseApiResponse res;
  co_return res;
}

drogon::Task<dto::BaseApiResponse>
ErpApi::getCategories(const std::string &businessId, int pageNo, int pageSize) {

  dto::BaseApiResponse response;

  LOG_INFO << "[getCategories] Entering method. businessId: '" << businessId
           << "', pageNo: " << pageNo << ", pageSize: " << pageSize;

  // ---------------------------------------------------------------
  // 0. Validate businessId
  // ---------------------------------------------------------------
  if (businessId.empty()) {
    LOG_WARN << "[getCategories] businessId is empty. Aborting.";
    response.message = "BusinessAccountId is required.";
    response.success = false;
    co_return response;
  }

  try {
    // ---------------------------------------------------------------
    // 1. Database client + mapper
    // ---------------------------------------------------------------
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
      LOG_ERROR << "[getCategories] Failed to get database client!";
      response.message = "Database client unavailable.";
      response.success = false;
      co_return response;
    }
    LOG_DEBUG << "[getCategories] Database client obtained.";

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::PosInventoryCategories>
        mapper(dbClient);

    // ---------------------------------------------------------------
    // 2. Validate and clamp pagination parameters
    // ---------------------------------------------------------------
    if (pageNo < 1)
      pageNo = 1;
    if (pageSize < 1)
      pageSize = 20;
    if (pageSize > 100) {
      LOG_WARN << "[getCategories] pageSize " << pageSize
               << " exceeds max, clamping to 100";
      pageSize = 100;
    }
    const int offset = (pageNo - 1) * pageSize;

    LOG_DEBUG << "[getCategories] Effective pagination — pageNo: " << pageNo
              << ", pageSize: " << pageSize << ", offset: " << offset;

    // ---------------------------------------------------------------
    // 3. Business-scoped criteria (always applied)
    // ---------------------------------------------------------------
    drogon::orm::Criteria businessCriteria(
        drogon_model::DataSupportDb::PosInventoryCategories::Cols::_business_id,
        drogon::orm::CompareOperator::EQ, businessId);

    // ---------------------------------------------------------------
    // 4. Count total for this business
    // ---------------------------------------------------------------
    const int64_t totalCount = co_await mapper.count(businessCriteria);
    LOG_DEBUG << "[getCategories] Total categories for business '" << businessId
              << "': " << totalCount;

    // ---------------------------------------------------------------
    // 5. Fetch the requested page
    // ---------------------------------------------------------------
    auto categories = co_await mapper
                          .orderBy(drogon_model::DataSupportDb::
                                       PosInventoryCategories::Cols::_name,
                                   drogon::orm::SortOrder::ASC)
                          .paginate(pageNo, pageSize)
                          .findBy(businessCriteria);

    LOG_DEBUG << "[getCategories] Fetched " << categories.size()
              << " categories for page " << pageNo;

    // ---------------------------------------------------------------
    // 6. Build the JSON response — camelCase
    // ---------------------------------------------------------------
    Json::Value data = Json::arrayValue;

    for (const auto &category : categories) {
      Json::Value j =
          utils::JsonUtils::convertKeysToCamelCase(category.toJson());
      data.append(j);
    }

    const int totalPages = (totalCount + pageSize - 1) / pageSize;

    response.success = true;
    response.message = "Categories retrieved successfully.";
    response.result["data"] = data;
    response.result["totalCount"] = (Json::UInt64)totalCount;
    response.result["pageNo"] = pageNo;
    response.result["pageSize"] = pageSize;
    response.result["totalPages"] = totalPages;
    response.result["lowerBound"] = totalCount == 0 ? 0 : (Json::UInt64)(offset + 1);
    response.result["upperBound"] =  (Json::UInt64)(pageNo == totalPages ? totalCount : (int64_t)pageNo * pageSize);

    LOG_INFO << "[getCategories] Exiting method. Returned " << data.size()
             << " of " << totalCount << " categories (page " << pageNo << "/"
             << totalPages << ").";

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "[getCategories] Database error: " << e.base().what();
    response.message = "Database error occurred while fetching categories: " +
                       std::string(e.base().what());
    response.success = false;

  } catch (const std::exception &e) {
    LOG_ERROR << "[getCategories] General exception: " << e.what();
    response.message = "Failed to fetch categories. Please try again. Error: " +
                       std::string(e.what());
    response.success = false;
  }

  co_return response;
}

drogon::Task<dto::BaseApiResponse> ErpApi::getCurrencies() {

  // rely on synced data
  dto::BaseApiResponse res;
  co_return res;
}

// auth

drogon::Task<dto::BaseApiResponse> ErpApi::verifyCustomer() {

  // direct call
  dto::BaseApiResponse res;
  co_return res;
}

drogon::Task<dto::erp_api::ErpOperationResponse> ErpApi::posCustomerSignup(const std::string &businessId,
                          const dto::erp_api::PosCustomerSignupDto &dto) {

  dto::erp_api::ErpOperationResponse response;
  response.success = false;
  response.code = 1;

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
      LOG_ERROR
          << "[posCustomerSignup] FAILED: Could not parse ConfigurationData - "
          << errs;
      response.message =
          "Failed to initialize customer signup. Configuration error.";
      co_return response;
    }

    if (!configJson.isMember("ErpApi") || !configJson["ErpApi"].isObject()) {
      LOG_ERROR << "[posCustomerSignup] FAILED: ErpApi configuration missing";
      response.message = "ERP API not configured for this account.";
      co_return response;
    }

    std::string accountId = configJson["ErpApi"]["AccountId"].asString();
    std::string accountSecret =
        configJson["ErpApi"]["AccountSecret"].asString();

    auto client =
        drogon::HttpClient::newHttpClient("https://erp-api.rhyoliteprime.com");
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("/api/services/app/Customer/Register");
    req->addHeader("AccountId", accountId);
    req->addHeader("AccountSecret", accountSecret);
    req->setContentTypeCode(drogon::CT_APPLICATION_JSON);

    Json::Value reqJson;
    reqJson["Username"] = dto.getUsername();
    reqJson["FirstName"] = dto.getFirstName();
    reqJson["LastName"] = dto.getLastName();
    reqJson["PhoneNumber"] = dto.getPhoneNo();
    reqJson["Email"] = dto.getEmail();
    reqJson["Pin"] = dto.getPassword();

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    req->setBody(Json::writeString(writer, reqJson));

    auto resp = co_await client->sendRequestCoro(req);

    if (resp) {
      auto respJson = resp->getJsonObject();
      if (resp->getStatusCode() == drogon::k200OK) {
        response.success = true;
        response.code = 0;
        response.message = "Signup successful";

        if (respJson && respJson->isMember("success") &&
            !(*respJson)["success"].asBool()) {
          response.success = false;
          response.code = 1;
          if (respJson->isMember("error") && !(*respJson)["error"].isNull()) {
            response.message = (*respJson)["error"]["message"].asString();
          } else {
            response.message = "Signup failed";
          }
        }
      } else {
        response.message = "Request failed with status code " +
                           std::to_string(resp->getStatusCode());
        if (respJson && respJson->isMember("error") &&
            !(*respJson)["error"].isNull()) {
          response.message = (*respJson)["error"]["message"].asString();
        }
      }
    } else {
      response.message = "Network error. Failed to reach the server.";
    }

  } catch (const drogon::orm::UnexpectedRows &e) {
    LOG_ERROR << "[posCustomerSignup] Business account not found: " << e.what();
    response.message = "Business account not found.";
  } catch (const std::exception &e) {
    LOG_ERROR << "[posCustomerSignup] Exception: " << e.what();
    response.message = "An error occurred during signup.";
  }

  co_return response;
}

drogon::Task<dto::erp_api::ErpOperationResponse> ErpApi::posCustomerLogin(const std::string &businessId,
                         const dto::erp_api::PosCustomerLoginDto &dto) {

  dto::erp_api::ErpOperationResponse response;
  response.success = false;
  response.code = 1;

  try {
    auto dbClient = drogon::app().getDbClient();
    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> bizMapper(dbClient);
    auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR
          << "[posCustomerLogin] FAILED: Could not parse ConfigurationData - "
          << errs;
      response.message =
          "Failed to initialize customer login. Configuration error.";
      co_return response;
    }

    if (!configJson.isMember("ErpApi") || !configJson["ErpApi"].isObject()) {
      LOG_ERROR << "[posCustomerLogin] FAILED: ErpApi configuration missing";
      response.message = "ERP API not configured for this account.";
      co_return response;
    }

    std::string accountId = configJson["ErpApi"]["AccountId"].asString();
    std::string accountSecret = configJson["ErpApi"]["AccountSecret"].asString();

    auto client = drogon::HttpClient::newHttpClient("https://erp-api.rhyoliteprime.com");
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("/api/services/app/Customer/Login");
    req->addHeader("AccountId", accountId);
    req->addHeader("AccountSecret", accountSecret);
    req->setContentTypeCode(drogon::CT_APPLICATION_JSON);

    Json::Value reqJson;
    reqJson["AccountIdentifier"] = dto.getCustomerIdentifier();
    reqJson["Password"] = dto.getPassword();

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    req->setBody(Json::writeString(writer, reqJson));

    auto resp = co_await client->sendRequestCoro(req);

    if (resp) {
      auto respJson = resp->getJsonObject();
      if (resp->getStatusCode() == drogon::k200OK) {
        response.success = true;
        response.code = 0;
        response.message = "Login successful";

        if (respJson && respJson->isMember("success") && !(*respJson)["success"].asBool()) {
          response.success = false;
          response.code = 1;
          if (respJson->isMember("error") && !(*respJson)["error"].isNull()) {
            response.message = (*respJson)["error"]["message"].asString();
          } else {
            response.message = "Login failed";
          }
        } else if (respJson && respJson->isMember("result") && !(*respJson)["result"].isNull()) {
          auto result = (*respJson)["result"];
          if (result.isMember("customerId") && !result["customerId"].isNull()) {
            response.result["customerId"] = result["customerId"].asString();
          }
          if (result.isMember("token") && !result["token"].isNull()) {
            response.result["token"] = result["token"].asString();
          }

          if (result.isMember("email") && !result["email"].isNull()) {
            response.result["email"] = result["email"].asString();
          }

          if (result.isMember("username") && !result["username"].isNull()) {
            response.result["username"] = result["username"].asString();
          }

        }
      } else {
        response.message = "Request failed with status code " + std::to_string(resp->getStatusCode());
        if (respJson && respJson->isMember("error") &&
            !(*respJson)["error"].isNull()) {
          response.message = (*respJson)["error"]["message"].asString();
        }
      }
    } else {
      response.message = "Network error. Failed to reach the server.";
    }

  } catch (const drogon::orm::UnexpectedRows &e) {
    LOG_ERROR << "[posCustomerLogin] Business account not found: " << e.what();
    response.message = "Business account not found.";
  } catch (const std::exception &e) {
    LOG_ERROR << "[posCustomerLogin] Exception: " << e.what();
    response.message = "An error occurred during login.";
  }

  co_return response;
}

drogon::Task<dto::BaseApiResponse> ErpApi::resetPassword(const std::string &businessId, const std::string &customerHeader) {

  // direct call
  dto::BaseApiResponse res;
  co_return res;
}

drogon::Task<dto::BaseApiResponse> ErpApi::verifyOtp(const std::string &businessId, const std::string &customerHeader) {

  // direct call
  dto::BaseApiResponse res;
  co_return res;
}

drogon::Task<dto::erp_api::ErpOperationResponse> ErpApi::addItemToCart(const std::string &businessId, const std::string &customerHeader, const dto::erp_api::PosShopCartDto &dto){

  // direct call
  dto::erp_api::ErpOperationResponse response;
  response.success = false;
  response.code = 1;

  try {
    auto dbClient = drogon::app().getDbClient();
    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> bizMapper(dbClient);
    auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR
          << "[addItemToCart] FAILED: Could not parse ConfigurationData - "
          << errs;
      response.message = "Failed to initialize add item to cart. Configuration error.";
      co_return response;
    }

    if (!configJson.isMember("ErpApi") || !configJson["ErpApi"].isObject()) {
      LOG_ERROR << "[posCustomerLogin] FAILED: ErpApi configuration missing";
      response.message = "ERP API not configured for this account.";
      co_return response;
    }

    std::string accountId = configJson["ErpApi"]["AccountId"].asString();
    std::string accountSecret = configJson["ErpApi"]["AccountSecret"].asString();

    auto client = drogon::HttpClient::newHttpClient("https://erp-api.rhyoliteprime.com");
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("/api/services/app/ShopCart/AddToCart");
    req->addHeader("AccountId", accountId);
    req->addHeader("AccountSecret", accountSecret);
    req->addHeader("X-Customer", customerHeader);
    req->setContentTypeCode(drogon::CT_APPLICATION_JSON);

    Json::Value reqJson;
    reqJson["ItemId"] = dto.getItemId();
    reqJson["Quantity"] = dto.getQuantity();

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    req->setBody(Json::writeString(writer, reqJson));

    auto resp = co_await client->sendRequestCoro(req);

    if (resp) {
      auto respJson = resp->getJsonObject();
      if (resp->getStatusCode() == drogon::k200OK) {
        response.success = true;
        response.code = 0;
        response.message = "Item Added Successfully.";
      }
    } else {
      response.message = "Network error. Failed to reach the server.";
    }

  } catch (const drogon::orm::UnexpectedRows &e) {
    LOG_ERROR << "[addItemToCart] Business account not found: " << e.what();
    response.message = "Business account not found.";
  } catch (const std::exception &e) {
    LOG_ERROR << "[addItemToCart] Exception: " << e.what();
    response.message = "An error occurred during login.";
  }

  co_return response;

}

  drogon::Task<dto::erp_api::ErpOperationResponse> ErpApi::getCartItemCount(const std::string &businessId, const std::string &customerHeader) {
  dto::erp_api::ErpOperationResponse response;
  response.success = false;
  response.code = 1;

  if (businessId.empty()) {
    response.message = "BusinessAccountId is required.";
    co_return response;
  }
  if (customerHeader.empty()) {
    response.message = "Customer authentication is required.";
    co_return response;
  }

  try {
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
      LOG_ERROR << "[getCartItemCount] Database client unavailable";
      response.message = "Database client unavailable.";
      co_return response;
    }

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts>
        bizMapper(dbClient);
    auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "[getCartItemCount] FAILED: Could not parse "
                   "ConfigurationData - "
                << errs;
      response.message = "Failed to get cart count. Configuration error.";
      co_return response;
    }

    if (!configJson.isMember("ErpApi") || !configJson["ErpApi"].isObject()) {
      LOG_ERROR << "[getCartItemCount] FAILED: ErpApi configuration missing";
      response.message = "ERP API not configured for this account.";
      co_return response;
    }

    std::string accountId = configJson["ErpApi"]["AccountId"].asString();
    std::string accountSecret =
        configJson["ErpApi"]["AccountSecret"].asString();

    auto client =
        drogon::HttpClient::newHttpClient("https://erp-api.rhyoliteprime.com");
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath("/api/services/app/ShopCart/GetCartItemCount");
    req->addHeader("AccountId", accountId);
    req->addHeader("AccountSecret", accountSecret);
    req->addHeader("X-Customer", customerHeader);

    auto resp = co_await client->sendRequestCoro(req);

    if (!resp) {
      response.message = "Network error. Failed to reach the server.";
      co_return response;
    }

    auto respJson = resp->getJsonObject();

    if (resp->getStatusCode() != drogon::k200OK) {
      response.message = "Request failed with status code " +
                         std::to_string(resp->getStatusCode());
      if (respJson && respJson->isMember("error") &&
          !(*respJson)["error"].isNull() &&
          (*respJson)["error"].isMember("message")) {
        response.message = (*respJson)["error"]["message"].asString();
      }
      co_return response;
    }

    // ABP wrapper: { result, success, error, ... }
    if (respJson && respJson->isMember("success") &&
        !(*respJson)["success"].asBool()) {
      response.success = false;
      response.code = 1;
      if (respJson->isMember("error") && !(*respJson)["error"].isNull() &&
          (*respJson)["error"].isMember("message")) {
        response.message = (*respJson)["error"]["message"].asString();
      } else {
        response.message = "Failed to retrieve cart item count.";
      }
      co_return response;
    }

    int cartCount = 0;
    if (respJson && respJson->isMember("result") &&
        !(*respJson)["result"].isNull() &&
        (*respJson)["result"].isNumeric()) {
      cartCount = (*respJson)["result"].asInt();
    }

    response.success = true;
    response.code = 0;
    response.message = "Cart item count retrieved successfully.";
    response.result["cartCount"] = cartCount;

  } catch (const drogon::orm::UnexpectedRows &e) {
    LOG_ERROR << "[getCartItemCount] Business account not found: " << e.what();
    response.message = "Business account not found.";
  } catch (const std::exception &e) {
    LOG_ERROR << "[getCartItemCount] Exception: " << e.what();
    response.message = "An error occurred while fetching cart item count.";
  }

  co_return response;
}

  drogon::Task<dto::erp_api::ErpOperationResponse> ErpApi::getCartItems(const std::string &businessId, const std::string &customerHeader) {
  dto::erp_api::ErpOperationResponse response;
  response.success = false;
  response.code = 1;

  if (businessId.empty()) {
    response.message = "BusinessAccountId is required.";
    co_return response;
  }
  if (customerHeader.empty()) {
    response.message = "Customer authentication is required.";
    co_return response;
  }

  try {
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
      LOG_ERROR << "[getCartItems] Database client unavailable";
      response.message = "Database client unavailable.";
      co_return response;
    }

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> bizMapper(dbClient);
    auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "[getCartItems] FAILED: Could not parse ConfigurationData - "
                << errs;
      response.message = "Failed to get cart items. Configuration error.";
      co_return response;
    }

    if (!configJson.isMember("ErpApi") || !configJson["ErpApi"].isObject()) {
      LOG_ERROR << "[getCartItems] FAILED: ErpApi configuration missing";
      response.message = "ERP API not configured for this account.";
      co_return response;
    }

    std::string accountId = configJson["ErpApi"]["AccountId"].asString();
    std::string accountSecret =
        configJson["ErpApi"]["AccountSecret"].asString();

    auto client =
        drogon::HttpClient::newHttpClient("https://erp-api.rhyoliteprime.com");
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath("/api/services/app/ShopCart/GetCartItems");
    req->addHeader("AccountId", accountId);
    req->addHeader("AccountSecret", accountSecret);
    req->addHeader("X-Customer", customerHeader);

    auto resp = co_await client->sendRequestCoro(req);

    if (!resp) {
      response.message = "Network error. Failed to reach the server.";
      co_return response;
    }

    auto respJson = resp->getJsonObject();

    if (resp->getStatusCode() != drogon::k200OK) {
      response.message = "Request failed with status code " +
                         std::to_string(resp->getStatusCode());
      if (respJson && respJson->isMember("error") &&
          !(*respJson)["error"].isNull() &&
          (*respJson)["error"].isMember("message")) {
        response.message = (*respJson)["error"]["message"].asString();
      }
      co_return response;
    }

    // ABP wrapper failure: { success: false, error: { message } }
    if (respJson && respJson->isMember("success") &&
        !(*respJson)["success"].asBool()) {
      response.success = false;
      response.code = 1;
      if (respJson->isMember("error") && !(*respJson)["error"].isNull() &&
          (*respJson)["error"].isMember("message")) {
        response.message = (*respJson)["error"]["message"].asString();
      } else {
        response.message = "Failed to retrieve cart items.";
      }
      co_return response;
    }

    Json::Value items(Json::arrayValue);
    int cartCount = 0; // sum of quantities, matches GetCartItemCount (1+1+3=5)
    double subTotal = 0.0;

    if (respJson && respJson->isMember("result") &&
        (*respJson)["result"].isArray()) {
      for (const auto &it : (*respJson)["result"]) {
        items.append(it);
        int qty = it.isMember("quantity") && it["quantity"].isNumeric()
                      ? it["quantity"].asInt()
                      : 0;
        double price = it.isMember("unitPrice") && it["unitPrice"].isNumeric()
                           ? it["unitPrice"].asDouble()
                           : 0.0;
        cartCount += qty;
        subTotal += qty * price;
      }
    }

    response.success = true;
    response.code = 0;
    response.message = "Cart items retrieved successfully.";
    response.result["items"] = items;
    response.result["itemCount"] = (Json::UInt)items.size();
    response.result["cartCount"] = cartCount;
    response.result["subTotal"] = subTotal;

  } catch (const drogon::orm::UnexpectedRows &e) {
    LOG_ERROR << "[getCartItems] Business account not found: " << e.what();
    response.message = "Business account not found.";
  } catch (const std::exception &e) {
    LOG_ERROR << "[getCartItems] Exception: " << e.what();
    response.message = "An error occurred while fetching cart items.";
  }

  co_return response;
}


  drogon::Task<dto::erp_api::ErpOperationResponse> ErpApi::removeItemFromCart(const std::string &businessId, const std::string &customerHeader, const std::string &itemId) {

  dto::erp_api::ErpOperationResponse response;
  response.success = false;
  response.code = 1;

  if (businessId.empty()) {
    response.message = "BusinessAccountId is required.";
    co_return response;
  }
  if (customerHeader.empty()) {
    response.message = "Customer authentication is required.";
    co_return response;
  }

  try {
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
      LOG_ERROR << "[removeItemFromCart] Database client unavailable";
      response.message = "Database client unavailable.";
      co_return response;
    }

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> bizMapper(dbClient);
    auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "[removeItemFromCart] FAILED: Could not parse ConfigurationData - "
                << errs;
      response.message = "Failed to get cart items. Configuration error.";
      co_return response;
    }

    if (!configJson.isMember("ErpApi") || !configJson["ErpApi"].isObject()) {
      LOG_ERROR << "[removeItemFromCart] FAILED: ErpApi configuration missing";
      response.message = "ERP API not configured for this account.";
      co_return response;
    }

    std::string accountId = configJson["ErpApi"]["AccountId"].asString();
    std::string accountSecret = configJson["ErpApi"]["AccountSecret"].asString();

    auto client = drogon::HttpClient::newHttpClient("https://erp-api.rhyoliteprime.com");
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath("/api/services/app/ShopCart/RemoveItem");
    req->setParameter("itemId", itemId);
    req->addHeader("AccountId", accountId);
    req->addHeader("AccountSecret", accountSecret);
    req->addHeader("X-Customer", customerHeader);


    auto resp = co_await client->sendRequestCoro(req);

    if (!resp) {
      response.message = "Network error. Failed to reach the server.";
      co_return response;
    }

    auto respJson = resp->getJsonObject();

    if (resp->getStatusCode() != drogon::k200OK) {
      response.message = "Request failed with status code " + std::to_string(resp->getStatusCode());
      if (respJson && respJson->isMember("error") && !(*respJson)["error"].isNull() && (*respJson)["error"].isMember("message")) {
        response.message = (*respJson)["error"]["message"].asString();
      }
      co_return response;
    }


    if (respJson && respJson->isMember("success") && !(*respJson)["success"].asBool()) {
      response.success = false;
      response.code = 1;
      if (respJson->isMember("error") && !(*respJson)["error"].isNull() && (*respJson)["error"].isMember("message")) {
        response.message = (*respJson)["error"]["message"].asString();
      } else {
        response.message = "Failed to remove item from cart.";
      }
      co_return response;
    }

    response.success = true;
    response.code = 0;
    response.message = "Cart item removed successfully.";


  } catch (const drogon::orm::UnexpectedRows &e) {
    LOG_ERROR << "[removeItemFromCart] Business account not found: " << e.what();
    response.message = "Business account not found.";
  } catch (const std::exception &e) {
    LOG_ERROR << "[removeItemFromCart] Exception: " << e.what();
    response.message = "An error occurred while removing cart items.";
  }

  co_return response;

 }


drogon::Task<dto::erp_api::ErpOperationResponse> ErpApi::clearCart(const std::string &businessId, const std::string &customerHeader) {

    dto::erp_api::ErpOperationResponse response;
  response.success = false;
  response.code = 1;

  if (businessId.empty()) {
    response.message = "BusinessAccountId is required.";
    co_return response;
  }
  if (customerHeader.empty()) {
    response.message = "Customer authentication is required.";
    co_return response;
  }

  try {
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
      LOG_ERROR << "[removeItemFromCart] Database client unavailable";
      response.message = "Database client unavailable.";
      co_return response;
    }

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts> bizMapper(dbClient);
    auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "[removeItemFromCart] FAILED: Could not parse ConfigurationData - "
                << errs;
      response.message = "Failed to get cart items. Configuration error.";
      co_return response;
    }

    if (!configJson.isMember("ErpApi") || !configJson["ErpApi"].isObject()) {
      LOG_ERROR << "[removeItemFromCart] FAILED: ErpApi configuration missing";
      response.message = "ERP API not configured for this account.";
      co_return response;
    }

    std::string accountId = configJson["ErpApi"]["AccountId"].asString();
    std::string accountSecret = configJson["ErpApi"]["AccountSecret"].asString();

    auto client = drogon::HttpClient::newHttpClient("https://erp-api.rhyoliteprime.com");
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath("/api/services/app/ShopCart/ClearCart");

    req->addHeader("AccountId", accountId);
    req->addHeader("AccountSecret", accountSecret);
    req->addHeader("X-Customer", customerHeader);


    auto resp = co_await client->sendRequestCoro(req);

    if (!resp) {
      response.message = "Network error. Failed to reach the server.";
      co_return response;
    }

    auto respJson = resp->getJsonObject();

    if (resp->getStatusCode() != drogon::k200OK) {
      response.message = "Request failed with status code " + std::to_string(resp->getStatusCode());
      if (respJson && respJson->isMember("error") && !(*respJson)["error"].isNull() && (*respJson)["error"].isMember("message")) {
        response.message = (*respJson)["error"]["message"].asString();
      }
      co_return response;
    }


    if (respJson && respJson->isMember("success") && !(*respJson)["success"].asBool()) {
      response.success = false;
      response.code = 1;
      if (respJson->isMember("error") && !(*respJson)["error"].isNull() && (*respJson)["error"].isMember("message")) {
        response.message = (*respJson)["error"]["message"].asString();
      } else {
        response.message = "Failed to remove item from cart.";
      }
      co_return response;
    }

    response.success = true;
    response.code = 0;
    response.message = "Cart item removed successfully.";


  } catch (const drogon::orm::UnexpectedRows &e) {
    LOG_ERROR << "[removeItemFromCart] Business account not found: " << e.what();
    response.message = "Business account not found.";
  } catch (const std::exception &e) {
    LOG_ERROR << "[removeItemFromCart] Exception: " << e.what();
    response.message = "An error occurred while removing cart items.";
  }

  co_return response;
}


  //wishlist

drogon::Task<dto::erp_api::ErpOperationResponse> ErpApi::addItemToWishlist(const std::string &businessId,
                          const std::string &customerHeader,
                          const std::string &itemId) {

  dto::erp_api::ErpOperationResponse response;
  response.success = false;
  response.code = 1;

  if (businessId.empty()) {
    response.message = "BusinessAccountId is required.";
    co_return response;
  }
  if (customerHeader.empty()) {
    response.message = "Customer authentication is required.";
    co_return response;
  }
  if (itemId.empty()) {
    response.message = "ItemId is required.";
    co_return response;
  }

  try {
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
      LOG_ERROR << "[addItemToWishlist] Database client unavailable";
      response.message = "Database client unavailable.";
      co_return response;
    }

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts>  bizMapper(dbClient);
    auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "[addItemToWishlist] FAILED: Could not parse "
                   "ConfigurationData - "
                << errs;
      response.message =
          "Failed to add item to wishlist. Configuration error.";
      co_return response;
    }

    if (!configJson.isMember("ErpApi") || !configJson["ErpApi"].isObject()) {
      LOG_ERROR << "[addItemToWishlist] FAILED: ErpApi configuration missing";
      response.message = "ERP API not configured for this account.";
      co_return response;
    }

    std::string accountId = configJson["ErpApi"]["AccountId"].asString();
    std::string accountSecret =  configJson["ErpApi"]["AccountSecret"].asString();

    auto client = drogon::HttpClient::newHttpClient("https://erp-api.rhyoliteprime.com");
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath("/api/services/app/Wishlist/AddToWishlist");
    req->setParameter("itemId", itemId);
    req->addHeader("AccountId", accountId);
    req->addHeader("AccountSecret", accountSecret);
    req->addHeader("X-Customer", customerHeader);
    req->setContentTypeCode(drogon::CT_APPLICATION_JSON);

    auto resp = co_await client->sendRequestCoro(req);

    if (!resp) {
      response.message = "Network error. Failed to reach the server.";
      co_return response;
    }

    auto respJson = resp->getJsonObject();

    if (resp->getStatusCode() != drogon::k200OK) {
      response.message = "Request failed with status code " +
                         std::to_string(resp->getStatusCode());
      if (respJson && respJson->isMember("error") &&
          !(*respJson)["error"].isNull() &&
          (*respJson)["error"].isMember("message")) {
        response.message = (*respJson)["error"]["message"].asString();
      }
      co_return response;
    }

    if (respJson && respJson->isMember("success") &&
        !(*respJson)["success"].asBool()) {
      response.success = false;
      response.code = 1;
      if (respJson->isMember("error") && !(*respJson)["error"].isNull() &&
          (*respJson)["error"].isMember("message")) {
        response.message = (*respJson)["error"]["message"].asString();
      } else {
        response.message = "Failed to add item to wishlist.";
      }
      co_return response;
    }

    response.success = true;
    response.code = 0;
    response.message = "Item added to wishlist successfully.";

  } catch (const drogon::orm::UnexpectedRows &e) {
    LOG_ERROR << "[addItemToWishlist] Business account not found: " << e.what();
    response.message = "Business account not found.";
  } catch (const std::exception &e) {
    LOG_ERROR << "[addItemToWishlist] Exception: " << e.what();
    response.message = "An error occurred while adding item to wishlist.";
  }

  co_return response;
}


drogon::Task<dto::erp_api::ErpOperationResponse> ErpApi::removeItemFromWishlist(const std::string &businessId,
                               const std::string &customerHeader,
                               const std::string &itemId) {

  dto::erp_api::ErpOperationResponse response;
  response.success = false;
  response.code = 1;

  if (businessId.empty()) {
    response.message = "BusinessAccountId is required.";
    co_return response;
  }
  if (customerHeader.empty()) {
    response.message = "Customer authentication is required.";
    co_return response;
  }
  if (itemId.empty()) {
    response.message = "ItemId is required.";
    co_return response;
  }

  try {
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
      LOG_ERROR << "[removeItemFromWishlist] Database client unavailable";
      response.message = "Database client unavailable.";
      co_return response;
    }

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts>
        bizMapper(dbClient);
    auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "[removeItemFromWishlist] FAILED: Could not parse "
                   "ConfigurationData - "
                << errs;
      response.message =
          "Failed to remove item from wishlist. Configuration error.";
      co_return response;
    }

    if (!configJson.isMember("ErpApi") || !configJson["ErpApi"].isObject()) {
      LOG_ERROR
          << "[removeItemFromWishlist] FAILED: ErpApi configuration missing";
      response.message = "ERP API not configured for this account.";
      co_return response;
    }

    std::string accountId = configJson["ErpApi"]["AccountId"].asString();
    std::string accountSecret = configJson["ErpApi"]["AccountSecret"].asString();

    auto client = drogon::HttpClient::newHttpClient("https://erp-api.rhyoliteprime.com");
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath("/api/services/app/Wishlist/RemoveItem");
    req->setParameter("itemId", itemId);
    req->addHeader("AccountId", accountId);
    req->addHeader("AccountSecret", accountSecret);
    req->addHeader("X-Customer", customerHeader);

    auto resp = co_await client->sendRequestCoro(req);

    if (!resp) {
      response.message = "Network error. Failed to reach the server.";
      co_return response;
    }

    auto respJson = resp->getJsonObject();

    if (resp->getStatusCode() != drogon::k200OK) {
      response.message = "Request failed with status code " +
                         std::to_string(resp->getStatusCode());
      if (respJson && respJson->isMember("error") &&
          !(*respJson)["error"].isNull() &&
          (*respJson)["error"].isMember("message")) {
        response.message = (*respJson)["error"]["message"].asString();
      }
      co_return response;
    }

    if (respJson && respJson->isMember("success") &&
        !(*respJson)["success"].asBool()) {
      response.success = false;
      response.code = 1;
      if (respJson->isMember("error") && !(*respJson)["error"].isNull() &&
          (*respJson)["error"].isMember("message")) {
        response.message = (*respJson)["error"]["message"].asString();
      } else {
        response.message = "Failed to remove item from wishlist.";
      }
      co_return response;
    }

    response.success = true;
    response.code = 0;
    response.message = "Wishlist item removed successfully.";

  } catch (const drogon::orm::UnexpectedRows &e) {
    LOG_ERROR << "[removeItemFromWishlist] Business account not found: "
              << e.what();
    response.message = "Business account not found.";
  } catch (const std::exception &e) {
    LOG_ERROR << "[removeItemFromWishlist] Exception: " << e.what();
    response.message = "An error occurred while removing wishlist items.";
  }

  co_return response;
}


drogon::Task<dto::erp_api::ErpOperationResponse> ErpApi::getWishlistItemCount(const std::string &businessId,
                             const std::string &customerHeader) {

  dto::erp_api::ErpOperationResponse response;
  response.success = false;
  response.code = 1;

  if (businessId.empty()) {
    response.message = "BusinessAccountId is required.";
    co_return response;
  }
  if (customerHeader.empty()) {
    response.message = "Customer authentication is required.";
    co_return response;
  }

  try {
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
      LOG_ERROR << "[getWishlistItemCount] Database client unavailable";
      response.message = "Database client unavailable.";
      co_return response;
    }

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts>
        bizMapper(dbClient);
    auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "[getWishlistItemCount] FAILED: Could not parse "
                   "ConfigurationData - "
                << errs;
      response.message =
          "Failed to get wishlist count. Configuration error.";
      co_return response;
    }

    if (!configJson.isMember("ErpApi") || !configJson["ErpApi"].isObject()) {
      LOG_ERROR
          << "[getWishlistItemCount] FAILED: ErpApi configuration missing";
      response.message = "ERP API not configured for this account.";
      co_return response;
    }

    std::string accountId = configJson["ErpApi"]["AccountId"].asString();
    std::string accountSecret =
        configJson["ErpApi"]["AccountSecret"].asString();

    auto client =
        drogon::HttpClient::newHttpClient("https://erp-api.rhyoliteprime.com");
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath("/api/services/app/Wishlist/GetWishlistItemCount");
    req->addHeader("AccountId", accountId);
    req->addHeader("AccountSecret", accountSecret);
    req->addHeader("X-Customer", customerHeader);

    auto resp = co_await client->sendRequestCoro(req);

    if (!resp) {
      response.message = "Network error. Failed to reach the server.";
      co_return response;
    }

    auto respJson = resp->getJsonObject();

    if (resp->getStatusCode() != drogon::k200OK) {
      response.message = "Request failed with status code " +
                         std::to_string(resp->getStatusCode());
      if (respJson && respJson->isMember("error") &&
          !(*respJson)["error"].isNull() &&
          (*respJson)["error"].isMember("message")) {
        response.message = (*respJson)["error"]["message"].asString();
      }
      co_return response;
    }

    // ABP wrapper: { result, success, error, ... }
    if (respJson && respJson->isMember("success") &&
        !(*respJson)["success"].asBool()) {
      response.success = false;
      response.code = 1;
      if (respJson->isMember("error") && !(*respJson)["error"].isNull() &&
          (*respJson)["error"].isMember("message")) {
        response.message = (*respJson)["error"]["message"].asString();
      } else {
        response.message = "Failed to retrieve wishlist item count.";
      }
      co_return response;
    }

    int wishlistCount = 0;
    if (respJson && respJson->isMember("result") &&
        !(*respJson)["result"].isNull() &&
        (*respJson)["result"].isNumeric()) {
      wishlistCount = (*respJson)["result"].asInt();
    }

    response.success = true;
    response.code = 0;
    response.message = "Wishlist item count retrieved successfully.";
    response.result["wishlistCount"] = wishlistCount;

  } catch (const drogon::orm::UnexpectedRows &e) {
    LOG_ERROR << "[getWishlistItemCount] Business account not found: "
              << e.what();
    response.message = "Business account not found.";
  } catch (const std::exception &e) {
    LOG_ERROR << "[getWishlistItemCount] Exception: " << e.what();
    response.message =
        "An error occurred while fetching wishlist item count.";
  }

  co_return response;
}


drogon::Task<dto::erp_api::ErpOperationResponse> ErpApi::getWishlistItems(const std::string &businessId,
                         const std::string &customerHeader) {

  dto::erp_api::ErpOperationResponse response;
  response.success = false;
  response.code = 1;

  if (businessId.empty()) {
    response.message = "BusinessAccountId is required.";
    co_return response;
  }
  if (customerHeader.empty()) {
    response.message = "Customer authentication is required.";
    co_return response;
  }

  try {
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
      LOG_ERROR << "[getWishlistItems] Database client unavailable";
      response.message = "Database client unavailable.";
      co_return response;
    }

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts>
        bizMapper(dbClient);
    auto businessAccount = co_await bizMapper.findByPrimaryKey(businessId);
    auto configStr = businessAccount.getValueOfConfigurationData();

    Json::Value configJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream iss(configStr);

    if (!Json::parseFromStream(readerBuilder, iss, &configJson, &errs)) {
      LOG_ERROR << "[getWishlistItems] FAILED: Could not parse "
                   "ConfigurationData - "
                << errs;
      response.message =
          "Failed to get wishlist items. Configuration error.";
      co_return response;
    }

    if (!configJson.isMember("ErpApi") || !configJson["ErpApi"].isObject()) {
      LOG_ERROR << "[getWishlistItems] FAILED: ErpApi configuration missing";
      response.message = "ERP API not configured for this account.";
      co_return response;
    }

    std::string accountId = configJson["ErpApi"]["AccountId"].asString();
    std::string accountSecret =
        configJson["ErpApi"]["AccountSecret"].asString();

    auto client =
        drogon::HttpClient::newHttpClient("https://erp-api.rhyoliteprime.com");
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath("/api/services/app/Wishlist/GetWishlistItems");
    req->addHeader("AccountId", accountId);
    req->addHeader("AccountSecret", accountSecret);
    req->addHeader("X-Customer", customerHeader);

    auto resp = co_await client->sendRequestCoro(req);

    if (!resp) {
      response.message = "Network error. Failed to reach the server.";
      co_return response;
    }

    auto respJson = resp->getJsonObject();

    if (resp->getStatusCode() != drogon::k200OK) {
      response.message = "Request failed with status code " +
                         std::to_string(resp->getStatusCode());
      if (respJson && respJson->isMember("error") &&
          !(*respJson)["error"].isNull() &&
          (*respJson)["error"].isMember("message")) {
        response.message = (*respJson)["error"]["message"].asString();
      }
      co_return response;
    }

    // ABP wrapper failure: { success: false, error: { message } }
    if (respJson && respJson->isMember("success") &&
        !(*respJson)["success"].asBool()) {
      response.success = false;
      response.code = 1;
      if (respJson->isMember("error") && !(*respJson)["error"].isNull() &&
          (*respJson)["error"].isMember("message")) {
        response.message = (*respJson)["error"]["message"].asString();
      } else {
        response.message = "Failed to retrieve wishlist items.";
      }
      co_return response;
    }

    Json::Value items(Json::arrayValue);

    if (respJson && respJson->isMember("result") &&
        (*respJson)["result"].isArray()) {
      for (const auto &it : (*respJson)["result"]) {
        items.append(it);
      }
    }

    response.success = true;
    response.code = 0;
    response.message = "Wishlist items retrieved successfully.";
    response.result["items"] = items;
    response.result["itemCount"] = (Json::UInt)items.size();
    response.result["wishlistCount"] = (Json::UInt)items.size();

  } catch (const drogon::orm::UnexpectedRows &e) {
    LOG_ERROR << "[getWishlistItems] Business account not found: " << e.what();
    response.message = "Business account not found.";
  } catch (const std::exception &e) {
    LOG_ERROR << "[getWishlistItems] Exception: " << e.what();
    response.message = "An error occurred while fetching wishlist items.";
  }

  co_return response;
}

// checkout

drogon::Task<dto::BaseApiResponse> ErpApi::initializeWebCheckout(const std::string &businessId, const std::string &customerHeader) {

  // direct call
  dto::BaseApiResponse res;
  co_return res;
}

drogon::Task<dto::BaseApiResponse> ErpApi::initializeUssdCheckout(const std::string &businessId, const std::string &customerHeader) {

  // direct call
  dto::BaseApiResponse res;
  co_return res;
}

drogon::Task<dto::BaseApiResponse> ErpApi::processPaymentCallback() {

  // direct call
  dto::BaseApiResponse res;
  co_return res;
}

drogon::Task<dto::BaseApiResponse> ErpApi::getInventoryItems(const std::string &businessId, const dto::erp_api::InventoryItemFilterDto &dto) {

  dto::BaseApiResponse response;

  LOG_INFO << "[getInventoryItems] Entering method. businessId: '" << businessId
           << "', CategoryId: '" << dto.getCategoryId() << "', CategorySlug: '"
           << dto.getCategorySlug() << "', Keywords: '" << dto.getKeywords()
           << "', SortBy: '" << dto.getSortBy()
           << "', pageNo: " << dto.getPageNo()
           << ", pageSize: " << dto.getPageSize();

  // ---------------------------------------------------------------
  // 0. Validate businessId
  // ---------------------------------------------------------------
  if (businessId.empty()) {
    LOG_WARN << "[getInventoryItems] businessId is empty. Aborting.";
    response.message = "BusinessAccountId is required.";
    response.success = false;
    co_return response;
  }

  try {
    // ---------------------------------------------------------------
    // 1. Database client
    // ---------------------------------------------------------------
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
      LOG_ERROR << "[getInventoryItems] Failed to get database client!";
      response.message = "Database client unavailable.";
      response.success = false;
      co_return response;
    }

    // ---------------------------------------------------------------
    // 2. Pagination
    // ---------------------------------------------------------------
    int pageNo = dto.getPageNo();
    int pageSize = dto.getPageSize();

    if (pageNo < 1)
      pageNo = 1;
    if (pageSize < 1)
      pageSize = 20;
    if (pageSize > 100) {
      LOG_WARN << "[getInventoryItems] pageSize " << pageSize
               << " exceeds max, clamping to 100";
      pageSize = 100;
    }
    const int offset = (pageNo - 1) * pageSize;

    // ---------------------------------------------------------------
    // 3. Predicate builder
    //
    //  - Each filter is only appended when it has a value.
    //  - Parameters are bound in the order the placeholders appear,
    //    so the returned placeholder is stable across count and data
    //    queries (they share the same whereClause + filterParams).
    // ---------------------------------------------------------------
    std::vector<std::string> whereClauses;
    std::vector<std::string> filterParams;

    auto bindParam = [&](const std::string &value) -> std::string {
      filterParams.push_back(value);
      return "$" + std::to_string(filterParams.size());
    };

    // 3a. business_id — always applied
    whereClauses.push_back("t.business_id = " + bindParam(businessId));

    // 3b. keywords — case-insensitive substring match, only if set
    if (!dto.getKeywords().empty()) {
      const std::string p = bindParam(dto.getKeywords());
      whereClauses.push_back(
          "(COALESCE(t.name, '') ILIKE '%' || " + p +
          " || '%'"
          " OR COALESCE(t.short_description, '') ILIKE '%' || " +
          p + " || '%')");

      LOG_DEBUG << "[getInventoryItems] Applied keywords filter: '"
                << dto.getKeywords() << "'";
    }

    // 3c. category — id and/or slug (OR-combined), only if either is set
    const bool hasCategoryId = !dto.getCategoryId().empty();
    const bool hasCategorySlug = !dto.getCategorySlug().empty();

    if (hasCategoryId || hasCategorySlug) {
      std::vector<std::string> catClauses;

      if (hasCategoryId) {
        catClauses.push_back("t.category_id = " +
                             bindParam(dto.getCategoryId()));
        LOG_DEBUG << "[getInventoryItems] Applied categoryId filter: "
                  << dto.getCategoryId();
      }
      if (hasCategorySlug) {
        catClauses.push_back("t.category_slug = " +
                             bindParam(dto.getCategorySlug()));
        LOG_DEBUG << "[getInventoryItems] Applied categorySlug filter: "
                  << dto.getCategorySlug();
      }

      std::string joined;
      for (std::size_t i = 0; i < catClauses.size(); ++i) {
        if (i)
          joined += " OR ";
        joined += catClauses[i];
      }
      whereClauses.push_back("(" + joined + ")");
    }

    // 3d. Assemble the WHERE clause
    std::string whereClause;
    if (!whereClauses.empty()) {
      whereClause = " WHERE ";
      for (std::size_t i = 0; i < whereClauses.size(); ++i) {
        if (i)
          whereClause += " AND ";
        whereClause += whereClauses[i];
      }
    }

    LOG_DEBUG << "[getInventoryItems] Built WHERE with " << filterParams.size()
              << " bound parameter(s): " << whereClause;

    // ---------------------------------------------------------------
    // 4. ORDER BY — sortBy whitelisted, safe to interpolate
    //
    //    featured          -> items tagged 'featured' first, then name
    //    newest            -> created_at DESC
    //    price:high-to-low -> selling_price DESC
    //    price:low-to-high -> selling_price ASC
    //    top-rates         -> rating DESC
    //    (default)         -> name ASC
    // ---------------------------------------------------------------
    const std::string &sortBy = dto.getSortBy();
    std::string orderBy;
    if (sortBy == "featured") {
      orderBy = " ORDER BY (CASE WHEN NULLIF(t.tags, '')::jsonb ? 'featured'"
                "                 THEN 0 ELSE 1 END) ASC, t.name ASC";
    } else if (sortBy == "newest") {
      orderBy = " ORDER BY t.created_at DESC";
    } else if (sortBy == "price:high-to-low") {
      orderBy = " ORDER BY t.selling_price::numeric DESC NULLS LAST";
    } else if (sortBy == "price:low-to-high") {
      orderBy = " ORDER BY t.selling_price::numeric ASC NULLS LAST";
    } else if (sortBy == "top-rates") {
      orderBy = " ORDER BY t.rating DESC NULLS LAST";
    } else {
      orderBy = " ORDER BY t.name ASC";
    }

    LOG_DEBUG << "[getInventoryItems] Applying sort: '"
              << (sortBy.empty() ? "name (default)" : sortBy) << "'";

    // ---------------------------------------------------------------
    // 5. Count
    // ---------------------------------------------------------------
    const std::string countSql =
        "SELECT COUNT(*)::bigint AS total FROM pos_inventory_items t" +
        whereClause;

    auto countResult = co_await runSql(dbClient, countSql, filterParams);
    const int64_t totalCount = countResult[0]["total"].as<int64_t>();

    LOG_DEBUG << "[getInventoryItems] Total matching items: " << totalCount;

    // ---------------------------------------------------------------
    // 6. Data page — LIMIT/OFFSET placeholders continue after the
    //    filter placeholders, so numbering stays contiguous.
    // ---------------------------------------------------------------
    const std::size_t nextParamNo = filterParams.size() + 1;
    const std::string limitPh = "$" + std::to_string(nextParamNo);
    const std::string offsetPh = "$" + std::to_string(nextParamNo + 1);

    const std::string dataSql = "SELECT to_jsonb(t) AS row_json"
                                "  FROM pos_inventory_items t" +
                                whereClause + orderBy + " LIMIT " + limitPh +
                                " OFFSET " + offsetPh;

    auto dataResult = co_await runSql(dbClient, dataSql, filterParams,
                                      (int64_t)pageSize, (int64_t)offset);

    LOG_DEBUG << "[getInventoryItems] Fetched " << dataResult.size()
              << " rows for page " << pageNo;

    // ---------------------------------------------------------------
    // 7. Deserialize JSON-array-as-text columns
    // ---------------------------------------------------------------
    auto parseArrayField = [](const Json::Value &src,
                              const std::string &key) -> Json::Value {
      if (!src.isMember(key)) {
        return Json::Value(Json::arrayValue);
      }
      if (!src[key].isString()) {
        return src[key];
      }
      const std::string &raw = src[key].asString();
      if (raw.empty()) {
        return Json::Value(Json::arrayValue);
      }
      Json::Value parsed;
      Json::CharReaderBuilder rb;
      std::string errs;
      std::istringstream iss(raw);
      if (Json::parseFromStream(rb, iss, &parsed, &errs) && parsed.isArray()) {
        return parsed;
      }
      return Json::Value(Json::arrayValue);
    };

    // ---------------------------------------------------------------
    // 8. Build the JSON response — camelCase
    // ---------------------------------------------------------------
    Json::Value data = Json::arrayValue;

    for (const auto &row : dataResult) {
      const std::string rowJson = row["row_json"].as<std::string>();

      Json::Value parsed;
      Json::CharReaderBuilder rb;
      std::string errs;
      std::istringstream iss(rowJson);
      if (!Json::parseFromStream(rb, iss, &parsed, &errs) ||
          !parsed.isObject()) {
        LOG_WARN << "[getInventoryItems] Skipping unparsable row: " << errs;
        continue;
      }

      Json::Value j = utils::JsonUtils::convertKeysToCamelCase(parsed);

      j["fullDescription"] = parseArrayField(j, "fullDescription");
      j["images"] = parseArrayField(j, "images");
      j["tags"] = parseArrayField(j, "tags");
      j["variants"] = parseArrayField(j, "variants");
      j["productAttributes"] = parseArrayField(j, "productAttributes");
      j["specifications"] = parseArrayField(j, "specifications");
      j["bulkPricing"] = parseArrayField(j, "bulkPricing");

      data.append(j);
    }

    const int totalPages =
        (totalCount == 0) ? 0 : (int)((totalCount + pageSize - 1) / pageSize);

    response.success = true;
    response.message = "Inventory items retrieved successfully.";
    response.result["data"] = data;
    response.result["totalCount"] = (Json::UInt64)totalCount;
    response.result["pageNo"] = pageNo;
    response.result["pageSize"] = pageSize;
    response.result["totalPages"] = totalPages;
    response.result["lowerBound"] =
        totalCount == 0 ? 0 : (Json::UInt64)(offset + 1);
    response.result["upperBound"] =
        (Json::UInt64)(pageNo == totalPages ? totalCount
                                            : (int64_t)pageNo * pageSize);

    LOG_INFO << "[getInventoryItems] Exiting method. Returned " << data.size()
             << " of " << totalCount << " items (page " << pageNo << "/"
             << totalPages << ").";

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "[getInventoryItems] Database error: " << e.base().what();
    response.message =
        "Database error occurred while fetching inventory items: " +
        std::string(e.base().what());
    response.success = false;

  } catch (const std::exception &e) {
    LOG_ERROR << "[getInventoryItems] General exception: " << e.what();
    response.message =
        "Failed to fetch inventory items. Please try again. Error: " +
        std::string(e.what());
    response.success = false;
  }

  co_return response;
}


drogon::Task<dto::BaseApiResponse> ErpApi::getInventoryItemDetail(const std::string &businessId, const std::string &slug) {

  dto::BaseApiResponse response;

  LOG_INFO << "[getInventoryItemDetail] Entering method. businessId: '"
           << businessId << "', slug: '" << slug << "'";

  // ---------------------------------------------------------------
  // 0. Validate inputs
  // ---------------------------------------------------------------
  if (businessId.empty()) {
    LOG_WARN << "[getInventoryItemDetail] businessId is empty. Aborting.";
    response.message = "BusinessAccountId is required.";
    response.success = false;
    co_return response;
  }

  if (slug.empty()) {
    LOG_WARN << "[getInventoryItemDetail] slug is empty. Aborting.";
    response.message = "Item slug is required.";
    response.success = false;
    co_return response;
  }

  try {
    // ---------------------------------------------------------------
    // 1. Database client
    // ---------------------------------------------------------------
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
      LOG_ERROR << "[getInventoryItemDetail] Failed to get database client!";
      response.message = "Database client unavailable.";
      response.success = false;
      co_return response;
    }

    // ---------------------------------------------------------------
    // 2. Query single item scoped to business + slug
    // ---------------------------------------------------------------
    const std::string sql =
        "SELECT to_jsonb(t) AS row_json"
        "  FROM pos_inventory_items t"
        " WHERE t.business_id = $1"
        "   AND t.slug = $2"
        " LIMIT 1";

    auto result = co_await dbClient->execSqlCoro(sql, businessId, slug);

    if (result.empty()) {
      LOG_WARN << "[getInventoryItemDetail] No item found for business '"
               << businessId << "' with slug '" << slug << "'.";
      response.message = "Inventory item not found.";
      response.success = false;
      co_return response;
    }

    // ---------------------------------------------------------------
    // 3. Parse the row JSON
    // ---------------------------------------------------------------
    const std::string rowJson = result[0]["row_json"].as<std::string>();

    Json::Value parsed;
    Json::CharReaderBuilder rb;
    std::string errs;
    std::istringstream iss(rowJson);
    if (!Json::parseFromStream(rb, iss, &parsed, &errs) ||
        !parsed.isObject()) {
      LOG_ERROR << "[getInventoryItemDetail] Failed to parse row JSON: "
                << errs;
      response.message = "Failed to parse inventory item data.";
      response.success = false;
      co_return response;
    }

    // ---------------------------------------------------------------
    // 4. Convert to camelCase
    // ---------------------------------------------------------------
    Json::Value j = utils::JsonUtils::convertKeysToCamelCase(parsed);

    // ---------------------------------------------------------------
    // 5. Deserialize JSON-array-as-text columns
    // ---------------------------------------------------------------
    auto parseArrayField = [](const Json::Value &src,
                              const std::string &key) -> Json::Value {
      if (!src.isMember(key)) {
        return Json::Value(Json::arrayValue);
      }
      if (!src[key].isString()) {
        return src[key];
      }
      const std::string &raw = src[key].asString();
      if (raw.empty()) {
        return Json::Value(Json::arrayValue);
      }
      Json::Value out;
      Json::CharReaderBuilder rr;
      std::string e;
      std::istringstream s(raw);
      if (Json::parseFromStream(rr, s, &out, &e) && out.isArray()) {
        return out;
      }
      return Json::Value(Json::arrayValue);
    };

    j["fullDescription"] = parseArrayField(j, "fullDescription");
    j["images"] = parseArrayField(j, "images");
    j["tags"] = parseArrayField(j, "tags");
    j["variants"] = parseArrayField(j, "variants");
    j["productAttributes"] = parseArrayField(j, "productAttributes");
    j["specifications"] = parseArrayField(j, "specifications");
    j["bulkPricing"] = parseArrayField(j, "bulkPricing");

    // ---------------------------------------------------------------
    // 6. Build response
    // ---------------------------------------------------------------
    response.success = true;
    response.message = "Inventory item retrieved successfully.";
    response.result["data"] = j;

    LOG_INFO << "[getInventoryItemDetail] Exiting method. Returned item '"
             << slug << "' for business '" << businessId << "'.";

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "[getInventoryItemDetail] Database error: "
              << e.base().what();
    response.message =
        "Database error occurred while fetching inventory item: " +
        std::string(e.base().what());
    response.success = false;

  } catch (const std::exception &e) {
    LOG_ERROR << "[getInventoryItemDetail] General exception: " << e.what();
    response.message =
        "Failed to fetch inventory item. Please try again. Error: " +
        std::string(e.what());
    response.success = false;
  }

  co_return response;
}



drogon::Task<dto::BaseApiResponse> ErpApi::getCustomerOrderHistory(const std::string &businessId, const std::string &customerHeader) {

  // direct call
  dto::BaseApiResponse response;
  co_return response;

}

drogon::Task<dto::BaseApiResponse> ErpApi::getUserProfile(const std::string &businessId, const std::string &customerHeader) {

  // direct call
  dto::BaseApiResponse response;
  co_return response;
}

drogon::Task<dto::BaseApiResponse> ErpApi::changePassword(const std::string &businessId, const std::string &customerHeader) {

  // direct call
  dto::BaseApiResponse res;
  co_return res;
}

drogon::Task<dto::BaseApiResponse> ErpApi::updateUserProfile(const std::string &businessId, const std::string &customerHeader) {

  // direct call

  dto::BaseApiResponse res;
  co_return res;
}

drogon::Task<dto::BaseApiResponse> ErpApi::getNotifications(const std::string &businessId, const std::string &customerHeader) {

  // rely on synced data
  dto::BaseApiResponse res;
  co_return res;
}

//------- SYNC API RESOURCES ----------

drogon::Task<dto::BaseApiResponse> ErpApi::syncApiCredentials(const dto::erp_api::SyncApiCredentialDto &dto) {

  dto::BaseApiResponse response;

  try {
    auto dbClient = drogon::app().getDbClient();
    drogon::orm::CoroMapper<drogon_model::DataSupportDb::BusinessAccounts>
        mapper(dbClient);

    auto account = co_await mapper.findByPrimaryKey(dto.getBusinessAccountId());

    std::string configStr = account.getValueOfConfigurationData();
    Json::Value configData;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;

    if (!configStr.empty()) {
      std::istringstream iss(configStr);
      if (!Json::parseFromStream(readerBuilder, iss, &configData, &errs)) {
        LOG_ERROR << "Failed to parse ConfigurationData: " << errs;
        response.message = "Configuration error.";
        response.success = false;
        co_return response;
      }
    } else {
      configData = Json::Value(Json::objectValue);
    }

    if (!configData.isMember("ErpApi") || !configData["ErpApi"].isObject()) {
      configData["ErpApi"] = Json::Value(Json::objectValue);
    }

    configData["ErpApi"]["AccountId"] = dto.getAccountId();
    configData["ErpApi"]["AccountSecret"] = dto.getAccountSecret();

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    account.setConfigurationData(Json::writeString(writer, configData));

    co_await mapper.update(account);

    response.message = "API credentials synced successfully";
    response.success = true;

  } catch (const std::exception &e) {
    LOG_ERROR << "Error syncing API credentials: " << e.what();

    response.message = "Failed to sync API credentials. Please try again.";
    response.success = false;
  }

  co_return response;
}

drogon::Task<dto::BaseApiResponse> ErpApi::syncPosBranches(const dto::erp_api::SyncPosBranchDto &dto) {
  dto::BaseApiResponse response;

  LOG_INFO << "[syncPosBranches] Entering method. Operation: '"
           << dto.getOperation() << "', BusinessAccountId: '"
           << dto.getBusinessAccountId() << "', BranchId: '" << dto.getId()
           << "'";

  try {
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
      LOG_ERROR << "[syncPosBranches] Failed to get database client!";
      response.message = "Database client unavailable.";
      response.success = false;
      co_return response;
    }
    LOG_DEBUG << "[syncPosBranches] Database client obtained.";

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::PosBranches> mapper(
        dbClient);
    LOG_DEBUG << "[syncPosBranches] CoroMapper created.";

    // Get data from DTO
    std::string businessAccountId = dto.getBusinessAccountId();
    const std::string &operation = dto.getOperation();

    LOG_DEBUG << "[syncPosBranches] Populating branch model with DTO data...";

    // Create model object
    drogon_model::DataSupportDb::PosBranches branch;

    // Set all fields
    branch.setId(dto.getId());
    branch.setBusinessId(dto.getBusinessAccountId());
    branch.setName(dto.getName());
    branch.setLocation(dto.getLocation());
    branch.setCountryName(dto.getCountryName());
    branch.setCountryCode(dto.getCountryCode());
    branch.setRegionOrState(dto.getRegionOrState());
    branch.setCity(dto.getCity());
    branch.setAddress(dto.getAddress());
    branch.setPhoneNo(dto.getPhoneNo());
    branch.setBranchManager(dto.getBranchManager());
    branch.setRevenueAccountId(dto.getRevenueAccountId());
    branch.setRevenueAccountNo(dto.getRevenueAccountNo());
    branch.setGeoCoordinates(dto.getGeoCoordinates());
    branch.setIsDefault(dto.getIsDefault());
    branch.setTenantId(dto.getTenantId());

    LOG_DEBUG << "[syncPosBranches] Branch model populated. ID: "
              << branch.getValueOfId() << ", Name: " << branch.getValueOfName();

    if (operation == "create") {
      LOG_INFO << "[syncPosBranches] Attempting to INSERT branch: "
               << dto.getId();

      co_await mapper.insert(branch);

      LOG_INFO << "[syncPosBranches] INSERT successful for branch: "
               << dto.getId();
      response.message = "Branch created successfully";
      response.success = true;

    } else if (operation == "update") {
      LOG_INFO << "[syncPosBranches] Attempting to UPDATE branch: "
               << dto.getId();

      branch.setUpdatedAt(trantor::Date::now());
      co_await mapper.update(branch);

      LOG_INFO << "[syncPosBranches] UPDATE successful for branch: "
               << dto.getId();
      response.message = "Branch updated successfully";
      response.success = true;

    } else if (operation == "delete") {
      LOG_INFO << "[syncPosBranches] Attempting to DELETE branch: "
               << dto.getId();

      co_await mapper.deleteByPrimaryKey(dto.getId());

      LOG_INFO << "[syncPosBranches] DELETE successful for branch: "
               << dto.getId();
      response.message = "Branch deleted successfully";
      response.success = true;

    } else {
      LOG_WARN << "[syncPosBranches] Unsupported operation: '" << operation
               << "' for branch: " << dto.getId();
      response.message = "Unsupported operation: " + operation;
      response.success = false;
    }

  } catch (const drogon::orm::UnexpectedRows &e) {
    // This exception is thrown by findByPrimaryKey when no record is found.
    // It is NOT expected for insert/update/delete. If it occurs here, something
    // is wrong.
    LOG_ERROR << "[syncPosBranches] UnexpectedRows exception caught: "
              << e.what()
              << ". This may indicate a missing record for an update/delete "
                 "operation.";
    response.message =
        "Record not found or database inconsistency: " + std::string(e.what());
    response.success = false;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "[syncPosBranches] Database error: " << e.base().what();
    response.message = "Database error occurred while syncing branch: " +
                       std::string(e.base().what());
    response.success = false;

  } catch (const std::exception &e) {
    LOG_ERROR << "[syncPosBranches] General exception: " << e.what();
    response.message = "Failed to sync branch. Please try again. Error: " +
                       std::string(e.what());
    response.success = false;
  }

  LOG_INFO << "[syncPosBranches] Exiting method. Success: " << response.success
           << ", Message: '" << response.message << "'";

  co_return response;
}

drogon::Task<dto::BaseApiResponse> ErpApi::syncPosInventoryCategories(const dto::erp_api::SyncInventoryCategoryDto &dto) {

  dto::BaseApiResponse response;

  LOG_INFO << "[syncPosInventoryCategories] Entering method. Operation: '"
           << dto.getOperation() << "', BusinessAccountId: '"
           << dto.getBusinessAccountId() << "', CategoryId: '" << dto.getId()
           << "'";

  try {
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
      LOG_ERROR
          << "[syncPosInventoryCategories] Failed to get database client!";
      response.message = "Database client unavailable.";
      response.success = false;
      co_return response;
    }
    LOG_DEBUG << "[syncPosInventoryCategories] Database client obtained.";

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::PosInventoryCategories>
        mapper(dbClient);
    LOG_DEBUG << "[syncPosInventoryCategories] CoroMapper created.";

    // Get envelope fields
    const std::string &businessAccountId = dto.getBusinessAccountId();
    const std::string &operation = dto.getOperation();

    if (businessAccountId.empty()) {
      LOG_WARN << "[syncPosInventoryCategories] BusinessAccountId is empty. "
                  "Aborting.";
      response.message = "BusinessAccountId is required.";
      response.success = false;
      co_return response;
    }

    if (dto.getId().empty()) {
      LOG_WARN
          << "[syncPosInventoryCategories] Category Id is empty. Aborting.";
      response.message = "Category Id is required.";
      response.success = false;
      co_return response;
    }

    // ---------------------------------------------------------------
    // DELETE — handled first because we only need the primary key
    // ---------------------------------------------------------------
    if (operation == "delete") {
      LOG_INFO << "[syncPosInventoryCategories] Attempting to DELETE category: "
               << dto.getId();

      auto deletedCount = co_await mapper.deleteByPrimaryKey(dto.getId());

      LOG_INFO
          << "[syncPosInventoryCategories] DELETE successful for category: "
          << dto.getId() << ", rows affected: " << deletedCount;

      response.message = "Category deleted successfully";
      response.success = true;
      co_return response;
    }

    // ---------------------------------------------------------------
    // BUILD MODEL for create / update
    // ---------------------------------------------------------------
    LOG_DEBUG << "[syncPosInventoryCategories] Populating category model with "
                 "DTO data...";

    drogon_model::DataSupportDb::PosInventoryCategories category;

    category.setId(dto.getId());
    category.setBusinessId(businessAccountId);
    category.setName(dto.getName());
    category.setSlug(dto.getSlug());
    category.setImageUrl(dto.getImageUrl());
    category.setDescription(dto.getDescription());
    category.setParentCategoryId(dto.getParentCategoryId());
    category.setIsService(dto.getIsService());
    category.setProductCount(dto.getProductCount());
    category.setTenantId(dto.getTenantId());

    LOG_DEBUG << "[syncPosInventoryCategories] Category model populated. ID: "
              << category.getValueOfId()
              << ", Name: " << category.getValueOfName();

    // ---------------------------------------------------------------
    // CREATE
    // ---------------------------------------------------------------
    if (operation == "create") {
      LOG_INFO << "[syncPosInventoryCategories] Attempting to INSERT category: "
               << dto.getId();

      category.setCreatedAt(trantor::Date::now());
      co_await mapper.insert(category);

      LOG_INFO
          << "[syncPosInventoryCategories] INSERT successful for category: "
          << dto.getId();

      response.message = "Category created successfully";
      response.success = true;
      co_return response;
    }

    // ---------------------------------------------------------------
    // UPDATE
    // ---------------------------------------------------------------
    if (operation == "update") {
      LOG_INFO << "[syncPosInventoryCategories] Attempting to UPDATE category: "
               << dto.getId();

      category.setUpdatedAt(trantor::Date::now());

      // update() uses the primary key set on the model, so make sure it's there
      auto updatedCount = co_await mapper.update(category);

      LOG_INFO
          << "[syncPosInventoryCategories] UPDATE successful for category: "
          << dto.getId() << ", rows affected: " << updatedCount;

      response.message = "Category updated successfully";
      response.success = true;
      co_return response;
    }

    // ---------------------------------------------------------------
    // UNKNOWN OPERATION
    // ---------------------------------------------------------------
    LOG_WARN << "[syncPosInventoryCategories] Unsupported operation: '"
             << operation << "' for category: " << dto.getId();
    response.message = "Unsupported operation: " + operation;
    response.success = false;

  } catch (const drogon::orm::UnexpectedRows &e) {
    LOG_ERROR << "[syncPosInventoryCategories] UnexpectedRows exception: "
              << e.what() << ". The category may not exist for update.";
    response.message =
        "Record not found or database inconsistency: " + std::string(e.what());
    response.success = false;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "[syncPosInventoryCategories] Database error: "
              << e.base().what();
    response.message = "Database error occurred while syncing category: " +
                       std::string(e.base().what());
    response.success = false;

  } catch (const std::exception &e) {
    LOG_ERROR << "[syncPosInventoryCategories] General exception: " << e.what();
    response.message = "Failed to sync category. Please try again. Error: " +
                       std::string(e.what());
    response.success = false;
  }

  LOG_INFO << "[syncPosInventoryCategories] Exiting method. Success: "
           << response.success << ", Message: '" << response.message << "'";

  co_return response;
}

drogon::Task<dto::BaseApiResponse>
ErpApi::syncPosCoupons(const dto::erp_api::SyncPosCouponDto &dto) {

  dto::BaseApiResponse response;

  LOG_INFO << "[syncPosCoupons] Entering method. Operation: '"
           << dto.getOperation() << "', BusinessAccountId: '"
           << dto.getBusinessAccountId() << "', CouponIdentifier: '"
           << dto.getCouponIdentifier() << "'";

  try {
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
      LOG_ERROR << "[syncPosCoupons] Failed to get database client!";
      response.message = "Database client unavailable.";
      response.success = false;
      co_return response;
    }
    LOG_DEBUG << "[syncPosCoupons] Database client obtained.";

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::PosCoupons> mapper(
        dbClient);
    LOG_DEBUG << "[syncPosCoupons] CoroMapper created.";

    // ---------------------------------------------------------------
    // Envelope validation
    // ---------------------------------------------------------------
    const std::string &businessAccountId = dto.getBusinessAccountId();
    const std::string &operation = dto.getOperation();

    if (businessAccountId.empty()) {
      LOG_WARN << "[syncPosCoupons] BusinessAccountId is empty. Aborting.";
      response.message = "BusinessAccountId is required.";
      response.success = false;
      co_return response;
    }

    if (dto.getCouponIdentifier().empty()) {
      LOG_WARN << "[syncPosCoupons] CouponIdentifier is empty. Aborting.";
      response.message = "CouponIdentifier is required.";
      response.success = false;
      co_return response;
    }

    // ---------------------------------------------------------------
    // DELETE — resolve by CouponIdentifier + BusinessId
    // ---------------------------------------------------------------
    if (operation == "delete") {
      LOG_INFO << "[syncPosCoupons] Attempting to DELETE coupon: "
               << dto.getCouponIdentifier();

      auto deletedCount = co_await mapper.deleteBy(
          drogon::orm::Criteria(
              drogon_model::DataSupportDb::PosCoupons::Cols::_business_id,
              drogon::orm::CompareOperator::EQ, businessAccountId) &&
          drogon::orm::Criteria(
              drogon_model::DataSupportDb::PosCoupons::Cols::_coupon_identifier,
              drogon::orm::CompareOperator::EQ, dto.getCouponIdentifier()));

      LOG_INFO << "[syncPosCoupons] DELETE completed for coupon: "
               << dto.getCouponIdentifier()
               << ", rows affected: " << deletedCount;

      response.message = "Coupon deleted successfully";
      response.success = true;
      co_return response;
    }

    // ---------------------------------------------------------------
    // BUILD MODEL for create / update
    // ---------------------------------------------------------------
    LOG_DEBUG << "[syncPosCoupons] Populating coupon model with DTO data...";

    drogon_model::DataSupportDb::PosCoupons coupon;

    coupon.setBusinessId(businessAccountId);
    coupon.setCouponValue(std::to_string(dto.getCouponValue()));
    coupon.setMinimumOrderLimit(std::to_string(dto.getMinimumOrderLimit()));
    coupon.setMaximumOrderLimit(std::to_string(dto.getMaximumOrderLimit()));
    coupon.setCouponIdentifier(dto.getCouponIdentifier());
    coupon.setValidTill(trantor::Date::fromDbStringLocal(dto.getValidTill()));
    coupon.setUsageQuota(dto.getUsageQuota());
    coupon.setUsageCount(dto.getUsageCount());
    coupon.setStatus(dto.getStatus());
    coupon.setUsePercentage(dto.getUsePercentage());
    coupon.setBranchId(dto.getBranchId());
    coupon.setBranchName(dto.getBranchName());

    // Serialize the Assignments array to a JSON string for storage
    if (!dto.getAssignments().isNull() && dto.getAssignments().isArray()) {
      Json::StreamWriterBuilder writer;
      writer["indentation"] = "";
      coupon.setAssignments(Json::writeString(writer, dto.getAssignments()));
    } else {
      coupon.setAssignments("[]");
    }

    LOG_DEBUG << "[syncPosCoupons] Coupon model populated. Identifier: "
              << coupon.getValueOfCouponIdentifier()
              << ", Value: " << coupon.getValueOfCouponValue();

    // ---------------------------------------------------------------
    // CREATE
    // ---------------------------------------------------------------
    if (operation == "create") {
      LOG_INFO << "[syncPosCoupons] Attempting to INSERT coupon: "
               << dto.getCouponIdentifier();

      coupon.setCreatedAt(trantor::Date::now());
      co_await mapper.insert(coupon);

      LOG_INFO << "[syncPosCoupons] INSERT successful for coupon: "
               << dto.getCouponIdentifier();

      response.message = "Coupon created successfully";
      response.success = true;
      co_return response;
    }

    // ---------------------------------------------------------------
    // UPDATE
    // ---------------------------------------------------------------
    if (operation == "update") {
      LOG_INFO << "[syncPosCoupons] Attempting to UPDATE coupon: "
               << dto.getCouponIdentifier();

      // Since the DTO does not carry the primary key, look up the
      // existing record by (business_id, coupon_identifier) first
      // and copy its primary key onto the model before updating.
      auto existing = co_await mapper.findOne(
          drogon::orm::Criteria(
              drogon_model::DataSupportDb::PosCoupons::Cols::_business_id,
              drogon::orm::CompareOperator::EQ, businessAccountId) &&
          drogon::orm::Criteria(
              drogon_model::DataSupportDb::PosCoupons::Cols::_coupon_identifier,
              drogon::orm::CompareOperator::EQ, dto.getCouponIdentifier()));

      coupon.setId(existing.getValueOfId());
      coupon.setUpdatedAt(trantor::Date::now());

      auto updatedCount = co_await mapper.update(coupon);

      LOG_INFO << "[syncPosCoupons] UPDATE successful for coupon: "
               << dto.getCouponIdentifier()
               << ", rows affected: " << updatedCount;

      response.message = "Coupon updated successfully";
      response.success = true;
      co_return response;
    }

    // ---------------------------------------------------------------
    // UNKNOWN OPERATION
    // ---------------------------------------------------------------
    LOG_WARN << "[syncPosCoupons] Unsupported operation: '" << operation
             << "' for coupon: " << dto.getCouponIdentifier();
    response.message = "Unsupported operation: " + operation;
    response.success = false;

  } catch (const drogon::orm::UnexpectedRows &e) {
    LOG_ERROR << "[syncPosCoupons] UnexpectedRows exception: " << e.what()
              << ". The coupon may not exist for update.";
    response.message =
        "Record not found or database inconsistency: " + std::string(e.what());
    response.success = false;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "[syncPosCoupons] Database error: " << e.base().what();
    response.message = "Database error occurred while syncing coupon: " +
                       std::string(e.base().what());
    response.success = false;

  } catch (const std::exception &e) {
    LOG_ERROR << "[syncPosCoupons] General exception: " << e.what();
    response.message = "Failed to sync coupon. Please try again. Error: " +
                       std::string(e.what());
    response.success = false;
  }

  LOG_INFO << "[syncPosCoupons] Exiting method. Success: " << response.success
           << ", Message: '" << response.message << "'";

  co_return response;
}

drogon::Task<dto::BaseApiResponse>
ErpApi::syncErpCurrencies(const dto::erp_api::SyncErpCurrencyDto &dto) {

  dto::BaseApiResponse response;

  LOG_INFO << "[syncErpCurrencies] Entering method. Operation: '"
           << dto.getOperation() << "', BusinessAccountId: '"
           << dto.getBusinessAccountId() << "', CurrencyCode: '"
           << dto.getCode() << "'";

  try {
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
      LOG_ERROR << "[syncErpCurrencies] Failed to get database client!";
      response.message = "Database client unavailable.";
      response.success = false;
      co_return response;
    }
    LOG_DEBUG << "[syncErpCurrencies] Database client obtained.";

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::ErpCurrencies> mapper(
        dbClient);
    LOG_DEBUG << "[syncErpCurrencies] CoroMapper created.";

    // ---------------------------------------------------------------
    // Envelope validation
    // ---------------------------------------------------------------
    const std::string &businessAccountId = dto.getBusinessAccountId();
    const std::string &operation = dto.getOperation();

    if (businessAccountId.empty()) {
      LOG_WARN << "[syncErpCurrencies] BusinessAccountId is empty. Aborting.";
      response.message = "BusinessAccountId is required.";
      response.success = false;
      co_return response;
    }

    if (dto.getCode().empty()) {
      LOG_WARN << "[syncErpCurrencies] Currency Code is empty. Aborting.";
      response.message = "Currency Code is required.";
      response.success = false;
      co_return response;
    }

    // ---------------------------------------------------------------
    // DELETE — resolve by (business_id, code)
    // ---------------------------------------------------------------
    if (operation == "delete") {
      LOG_INFO << "[syncErpCurrencies] Attempting to DELETE currency: "
               << dto.getCode();

      auto deletedCount = co_await mapper.deleteBy(
          drogon::orm::Criteria(
              drogon_model::DataSupportDb::ErpCurrencies::Cols::_business_id,
              drogon::orm::CompareOperator::EQ, businessAccountId) &&
          drogon::orm::Criteria(
              drogon_model::DataSupportDb::ErpCurrencies::Cols::_code,
              drogon::orm::CompareOperator::EQ, dto.getCode()));

      LOG_INFO << "[syncErpCurrencies] DELETE completed for currency: "
               << dto.getCode() << ", rows affected: " << deletedCount;

      response.message = "Currency deleted successfully";
      response.success = true;
      co_return response;
    }

    // ---------------------------------------------------------------
    // BUILD MODEL for create / update
    // ---------------------------------------------------------------
    LOG_DEBUG
        << "[syncErpCurrencies] Populating currency model with DTO data...";

    drogon_model::DataSupportDb::ErpCurrencies currency;

    currency.setBusinessId(businessAccountId);
    currency.setName(dto.getName());
    currency.setCode(dto.getCode());
    currency.setMinorName(dto.getMinorName());
    currency.setBuyRate(std::to_string(dto.getBuyRate()));
    currency.setSellRate(std::to_string(dto.getSellRate()));
    currency.setIsDefault(dto.getIsDefault());

    LOG_DEBUG << "[syncErpCurrencies] Currency model populated. Code: "
              << currency.getValueOfCode()
              << ", Name: " << currency.getValueOfName();

    // ---------------------------------------------------------------
    // CREATE
    // ---------------------------------------------------------------
    if (operation == "create") {
      LOG_INFO << "[syncErpCurrencies] Attempting to INSERT currency: "
               << dto.getCode();

      currency.setCreatedAt(trantor::Date::now());
      co_await mapper.insert(currency);

      LOG_INFO << "[syncErpCurrencies] INSERT successful for currency: "
               << dto.getCode();

      response.message = "Currency created successfully";
      response.success = true;
      co_return response;
    }

    // ---------------------------------------------------------------
    // UPDATE
    // ---------------------------------------------------------------
    if (operation == "update") {
      LOG_INFO << "[syncErpCurrencies] Attempting to UPDATE currency: "
               << dto.getCode();

      // Look up the existing row by (business_id, code) to obtain
      // its primary key, then copy it onto the model before updating.
      auto existing = co_await mapper.findOne(
          drogon::orm::Criteria(
              drogon_model::DataSupportDb::ErpCurrencies::Cols::_business_id,
              drogon::orm::CompareOperator::EQ, businessAccountId) &&
          drogon::orm::Criteria(
              drogon_model::DataSupportDb::ErpCurrencies::Cols::_code,
              drogon::orm::CompareOperator::EQ, dto.getCode()));

      currency.setId(existing.getValueOfId());
      currency.setUpdatedAt(trantor::Date::now());

      auto updatedCount = co_await mapper.update(currency);

      LOG_INFO << "[syncErpCurrencies] UPDATE successful for currency: "
               << dto.getCode() << ", rows affected: " << updatedCount;

      response.message = "Currency updated successfully";
      response.success = true;
      co_return response;
    }

    // ---------------------------------------------------------------
    // UNKNOWN OPERATION
    // ---------------------------------------------------------------
    LOG_WARN << "[syncErpCurrencies] Unsupported operation: '" << operation
             << "' for currency: " << dto.getCode();
    response.message = "Unsupported operation: " + operation;
    response.success = false;

  } catch (const drogon::orm::UnexpectedRows &e) {
    LOG_ERROR << "[syncErpCurrencies] UnexpectedRows exception: " << e.what()
              << ". The currency may not exist for update.";
    response.message =
        "Record not found or database inconsistency: " + std::string(e.what());
    response.success = false;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "[syncErpCurrencies] Database error: " << e.base().what();
    response.message = "Database error occurred while syncing currency: " +
                       std::string(e.base().what());
    response.success = false;

  } catch (const std::exception &e) {
    LOG_ERROR << "[syncErpCurrencies] General exception: " << e.what();
    response.message = "Failed to sync currency. Please try again. Error: " +
                       std::string(e.what());
    response.success = false;
  }

  LOG_INFO << "[syncErpCurrencies] Exiting method. Success: "
           << response.success << ", Message: '" << response.message << "'";

  co_return response;
}

drogon::Task<dto::BaseApiResponse> ErpApi::syncPosDeliveryChannels(
    const dto::erp_api::SyncPosDeliveryChannelDto &dto) {

  dto::BaseApiResponse response;

  LOG_INFO << "[syncPosDeliveryChannels] Entering method. Operation: '"
           << dto.getOperation() << "', BusinessAccountId: '"
           << dto.getBusinessAccountId() << "', ChannelName: '" << dto.getName()
           << "'";

  try {
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
      LOG_ERROR << "[syncPosDeliveryChannels] Failed to get database client!";
      response.message = "Database client unavailable.";
      response.success = false;
      co_return response;
    }
    LOG_DEBUG << "[syncPosDeliveryChannels] Database client obtained.";

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::PosDeliveryChannels>
        mapper(dbClient);
    LOG_DEBUG << "[syncPosDeliveryChannels] CoroMapper created.";

    // ---------------------------------------------------------------
    // Envelope validation
    // ---------------------------------------------------------------
    const std::string &businessAccountId = dto.getBusinessAccountId();
    const std::string &operation = dto.getOperation();

    if (businessAccountId.empty()) {
      LOG_WARN
          << "[syncPosDeliveryChannels] BusinessAccountId is empty. Aborting.";
      response.message = "BusinessAccountId is required.";
      response.success = false;
      co_return response;
    }

    if (dto.getName().empty()) {
      LOG_WARN << "[syncPosDeliveryChannels] Delivery channel Name is empty. "
                  "Aborting.";
      response.message = "Delivery channel Name is required.";
      response.success = false;
      co_return response;
    }

    // ---------------------------------------------------------------
    // DELETE — resolve by (business_id, name)
    // ---------------------------------------------------------------
    if (operation == "delete") {
      LOG_INFO
          << "[syncPosDeliveryChannels] Attempting to DELETE delivery channel: "
          << dto.getName();

      auto deletedCount = co_await mapper.deleteBy(
          drogon::orm::Criteria(drogon_model::DataSupportDb::
                                    PosDeliveryChannels::Cols::_business_id,
                                drogon::orm::CompareOperator::EQ,
                                businessAccountId) &&
          drogon::orm::Criteria(
              drogon_model::DataSupportDb::PosDeliveryChannels::Cols::_name,
              drogon::orm::CompareOperator::EQ, dto.getName()));

      LOG_INFO << "[syncPosDeliveryChannels] DELETE completed for channel: "
               << dto.getName() << ", rows affected: " << deletedCount;

      response.message = "Delivery channel deleted successfully";
      response.success = true;
      co_return response;
    }

    // ---------------------------------------------------------------
    // BUILD MODEL for create / update
    // ---------------------------------------------------------------
    LOG_DEBUG << "[syncPosDeliveryChannels] Populating delivery channel model "
                 "with DTO data...";

    drogon_model::DataSupportDb::PosDeliveryChannels channel;

    channel.setBusinessId(businessAccountId);
    channel.setName(dto.getName());
    channel.setDescription(dto.getDescription());
    channel.setIsLocal(dto.getIsLocal());

    // Serialize the ZoneRates array to a JSON string for storage
    if (!dto.getZoneRates().isNull() && dto.getZoneRates().isArray()) {
      Json::StreamWriterBuilder writer;
      writer["indentation"] = "";
      channel.setZoneRates(Json::writeString(writer, dto.getZoneRates()));
    } else {
      channel.setZoneRates("[]");
    }

    LOG_DEBUG << "[syncPosDeliveryChannels] Channel model populated. Name: "
              << channel.getValueOfName()
              << ", IsLocal: " << channel.getValueOfIsLocal()
              << ", ZoneRates count: " << dto.getZoneRates().size();

    // ---------------------------------------------------------------
    // CREATE
    // ---------------------------------------------------------------
    if (operation == "create") {
      LOG_INFO << "[syncPosDeliveryChannels] Attempting to INSERT channel: "
               << dto.getName();

      channel.setCreatedAt(trantor::Date::now());
      co_await mapper.insert(channel);

      LOG_INFO << "[syncPosDeliveryChannels] INSERT successful for channel: "
               << dto.getName();

      response.message = "Delivery channel created successfully";
      response.success = true;
      co_return response;
    }

    // ---------------------------------------------------------------
    // UPDATE
    // ---------------------------------------------------------------
    if (operation == "update") {
      LOG_INFO << "[syncPosDeliveryChannels] Attempting to UPDATE channel: "
               << dto.getName();

      // Look up the existing row by (business_id, name) to obtain
      // its primary key, then copy it onto the model before updating.
      auto existing = co_await mapper.findOne(
          drogon::orm::Criteria(drogon_model::DataSupportDb::
                                    PosDeliveryChannels::Cols::_business_id,
                                drogon::orm::CompareOperator::EQ,
                                businessAccountId) &&
          drogon::orm::Criteria(
              drogon_model::DataSupportDb::PosDeliveryChannels::Cols::_name,
              drogon::orm::CompareOperator::EQ, dto.getName()));

      channel.setId(existing.getValueOfId());
      channel.setUpdatedAt(trantor::Date::now());

      auto updatedCount = co_await mapper.update(channel);

      LOG_INFO << "[syncPosDeliveryChannels] UPDATE successful for channel: "
               << dto.getName() << ", rows affected: " << updatedCount;

      response.message = "Delivery channel updated successfully";
      response.success = true;
      co_return response;
    }

    // ---------------------------------------------------------------
    // UNKNOWN OPERATION
    // ---------------------------------------------------------------
    LOG_WARN << "[syncPosDeliveryChannels] Unsupported operation: '"
             << operation << "' for channel: " << dto.getName();
    response.message = "Unsupported operation: " + operation;
    response.success = false;

  } catch (const drogon::orm::UnexpectedRows &e) {
    LOG_ERROR << "[syncPosDeliveryChannels] UnexpectedRows exception: "
              << e.what() << ". The delivery channel may not exist for update.";
    response.message =
        "Record not found or database inconsistency: " + std::string(e.what());
    response.success = false;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "[syncPosDeliveryChannels] Database error: "
              << e.base().what();
    response.message =
        "Database error occurred while syncing delivery channel: " +
        std::string(e.base().what());
    response.success = false;

  } catch (const std::exception &e) {
    LOG_ERROR << "[syncPosDeliveryChannels] General exception: " << e.what();
    response.message =
        "Failed to sync delivery channel. Please try again. Error: " +
        std::string(e.what());
    response.success = false;
  }

  LOG_INFO << "[syncPosDeliveryChannels] Exiting method. Success: "
           << response.success << ", Message: '" << response.message << "'";

  co_return response;
}

drogon::Task<dto::BaseApiResponse>
ErpApi::syncPosPickupPoints(const dto::erp_api::SyncPosPickupPointDto &dto) {

  dto::BaseApiResponse response;

  LOG_INFO << "[syncPosPickupPoints] Entering method. Operation: '"
           << dto.getOperation() << "', BusinessAccountId: '"
           << dto.getBusinessAccountId() << "', SystemName: '"
           << dto.getSystemName() << "'";

  try {
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
      LOG_ERROR << "[syncPosPickupPoints] Failed to get database client!";
      response.message = "Database client unavailable.";
      response.success = false;
      co_return response;
    }
    LOG_DEBUG << "[syncPosPickupPoints] Database client obtained.";

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::PosPickupPoints>
        mapper(dbClient);
    LOG_DEBUG << "[syncPosPickupPoints] CoroMapper created.";

    // ---------------------------------------------------------------
    // Envelope validation
    // ---------------------------------------------------------------
    const std::string &businessAccountId = dto.getBusinessAccountId();
    const std::string &operation = dto.getOperation();

    if (businessAccountId.empty()) {
      LOG_WARN << "[syncPosPickupPoints] BusinessAccountId is empty. Aborting.";
      response.message = "BusinessAccountId is required.";
      response.success = false;
      co_return response;
    }

    if (dto.getSystemName().empty()) {
      LOG_WARN << "[syncPosPickupPoints] SystemName is empty. Aborting.";
      response.message = "Pickup point SystemName is required.";
      response.success = false;
      co_return response;
    }

    // ---------------------------------------------------------------
    // DELETE — resolve by (business_id, system_name)
    // ---------------------------------------------------------------
    if (operation == "delete") {
      LOG_INFO << "[syncPosPickupPoints] Attempting to DELETE pickup point: "
               << dto.getSystemName();

      auto deletedCount = co_await mapper.deleteBy(
          drogon::orm::Criteria(
              drogon_model::DataSupportDb::PosPickupPoints::Cols::_business_id,
              drogon::orm::CompareOperator::EQ, businessAccountId) &&
          drogon::orm::Criteria(
              drogon_model::DataSupportDb::PosPickupPoints::Cols::_system_name,
              drogon::orm::CompareOperator::EQ, dto.getSystemName()));

      LOG_INFO << "[syncPosPickupPoints] DELETE completed for pickup point: "
               << dto.getSystemName() << ", rows affected: " << deletedCount;

      response.message = "Pickup point deleted successfully";
      response.success = true;
      co_return response;
    }

    // ---------------------------------------------------------------
    // BUILD MODEL for create / update
    // ---------------------------------------------------------------
    LOG_DEBUG << "[syncPosPickupPoints] Populating pickup point model with DTO "
                 "data...";

    drogon_model::DataSupportDb::PosPickupPoints pickupPoint;

    pickupPoint.setBusinessId(businessAccountId);
    pickupPoint.setFriendlyName(dto.getFriendlyName());
    pickupPoint.setSystemName(dto.getSystemName());
    pickupPoint.setImageUrl(dto.getImageUrl());
    pickupPoint.setGhanaPostGps(dto.getGhanaPostGps());
    pickupPoint.setGpsCoordinates(dto.getGpsCoordinates());
    pickupPoint.setIsActive(dto.getIsActive());

    LOG_DEBUG
        << "[syncPosPickupPoints] Pickup point model populated. SystemName: "
        << pickupPoint.getValueOfSystemName()
        << ", FriendlyName: " << pickupPoint.getValueOfFriendlyName()
        << ", IsActive: " << pickupPoint.getValueOfIsActive();

    // ---------------------------------------------------------------
    // CREATE
    // ---------------------------------------------------------------
    if (operation == "create") {
      LOG_INFO << "[syncPosPickupPoints] Attempting to INSERT pickup point: "
               << dto.getSystemName();

      pickupPoint.setCreatedAt(trantor::Date::now());
      co_await mapper.insert(pickupPoint);

      LOG_INFO << "[syncPosPickupPoints] INSERT successful for pickup point: "
               << dto.getSystemName();

      response.message = "Pickup point created successfully";
      response.success = true;
      co_return response;
    }

    // ---------------------------------------------------------------
    // UPDATE
    // ---------------------------------------------------------------
    if (operation == "update") {
      LOG_INFO << "[syncPosPickupPoints] Attempting to UPDATE pickup point: "
               << dto.getSystemName();

      // Look up the existing row by (business_id, system_name) to obtain
      // its primary key, then copy it onto the model before updating.
      auto existing = co_await mapper.findOne(
          drogon::orm::Criteria(
              drogon_model::DataSupportDb::PosPickupPoints::Cols::_business_id,
              drogon::orm::CompareOperator::EQ, businessAccountId) &&
          drogon::orm::Criteria(
              drogon_model::DataSupportDb::PosPickupPoints::Cols::_system_name,
              drogon::orm::CompareOperator::EQ, dto.getSystemName()));

      pickupPoint.setId(existing.getValueOfId());
      pickupPoint.setUpdatedAt(trantor::Date::now());

      auto updatedCount = co_await mapper.update(pickupPoint);

      LOG_INFO << "[syncPosPickupPoints] UPDATE successful for pickup point: "
               << dto.getSystemName() << ", rows affected: " << updatedCount;

      response.message = "Pickup point updated successfully";
      response.success = true;
      co_return response;
    }

    // ---------------------------------------------------------------
    // UNKNOWN OPERATION
    // ---------------------------------------------------------------
    LOG_WARN << "[syncPosPickupPoints] Unsupported operation: '" << operation
             << "' for pickup point: " << dto.getSystemName();
    response.message = "Unsupported operation: " + operation;
    response.success = false;

  } catch (const drogon::orm::UnexpectedRows &e) {
    LOG_ERROR << "[syncPosPickupPoints] UnexpectedRows exception: " << e.what()
              << ". The pickup point may not exist for update.";
    response.message =
        "Record not found or database inconsistency: " + std::string(e.what());
    response.success = false;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "[syncPosPickupPoints] Database error: " << e.base().what();
    response.message = "Database error occurred while syncing pickup point: " +
                       std::string(e.base().what());
    response.success = false;

  } catch (const std::exception &e) {
    LOG_ERROR << "[syncPosPickupPoints] General exception: " << e.what();
    response.message =
        "Failed to sync pickup point. Please try again. Error: " +
        std::string(e.what());
    response.success = false;
  }

  LOG_INFO << "[syncPosPickupPoints] Exiting method. Success: "
           << response.success << ", Message: '" << response.message << "'";

  co_return response;
}

drogon::Task<dto::BaseApiResponse> ErpApi::syncPosInventoryItems(
    const dto::erp_api::SyncPosInventoryItemDto &dto) {

  dto::BaseApiResponse response;

  LOG_INFO << "[syncPosInventoryItems] Entering method. Operation: '"
           << dto.getOperation() << "', BusinessAccountId: '"
           << dto.getBusinessAccountId() << "', Sku: '" << dto.getSku() << "'";

  try {
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
      LOG_ERROR << "[syncPosInventoryItems] Failed to get database client!";
      response.message = "Database client unavailable.";
      response.success = false;
      co_return response;
    }
    LOG_DEBUG << "[syncPosInventoryItems] Database client obtained.";

    drogon::orm::CoroMapper<drogon_model::DataSupportDb::PosInventoryItems>
        mapper(dbClient);
    LOG_DEBUG << "[syncPosInventoryItems] CoroMapper created.";

    // ---------------------------------------------------------------
    // Envelope validation
    // ---------------------------------------------------------------
    const std::string &businessAccountId = dto.getBusinessAccountId();
    const std::string &operation = dto.getOperation();

    if (businessAccountId.empty()) {
      LOG_WARN
          << "[syncPosInventoryItems] BusinessAccountId is empty. Aborting.";
      response.message = "BusinessAccountId is required.";
      response.success = false;
      co_return response;
    }

    if (dto.getSku().empty()) {
      LOG_WARN << "[syncPosInventoryItems] Sku is empty. Aborting.";
      response.message = "Inventory item Sku is required.";
      response.success = false;
      co_return response;
    }

    // ---------------------------------------------------------------
    // DELETE — resolve by (business_id, sku)
    // ---------------------------------------------------------------
    if (operation == "delete") {
      LOG_INFO
          << "[syncPosInventoryItems] Attempting to DELETE inventory item: "
          << dto.getSku();

      auto deletedCount = co_await mapper.deleteBy(
          drogon::orm::Criteria(drogon_model::DataSupportDb::PosInventoryItems::
                                    Cols::_business_id,
                                drogon::orm::CompareOperator::EQ,
                                businessAccountId) &&
          drogon::orm::Criteria(
              drogon_model::DataSupportDb::PosInventoryItems::Cols::_sku,
              drogon::orm::CompareOperator::EQ, dto.getSku()));

      LOG_INFO << "[syncPosInventoryItems] DELETE completed for item: "
               << dto.getSku() << ", rows affected: " << deletedCount;

      response.message = "Inventory item deleted successfully";
      response.success = true;
      co_return response;
    }

    // ---------------------------------------------------------------
    // BUILD MODEL for create / update
    // ---------------------------------------------------------------
    LOG_DEBUG << "[syncPosInventoryItems] Populating inventory item model with "
                 "DTO data...";

    drogon_model::DataSupportDb::PosInventoryItems item;

    // Basic fields
    item.setId(dto.getId());
    item.setBusinessId(businessAccountId);
    item.setTenantId(dto.getTenantId());
    item.setName(dto.getName());
    item.setSlug(dto.getSlug());
    item.setShortDescription(dto.getShortDescription());
    item.setAdminComment(dto.getAdminComment());
    item.setMerchantId(dto.getMerchantId());
    item.setMerchantName(dto.getMerchantName());
    item.setSku(dto.getSku());
    item.setStockQuantity(dto.getStockQuantity());
    item.setOldSellingPrice(std::to_string(dto.getOldSellingPrice()));
    item.setSellingPrice(std::to_string(dto.getSellingPrice()));
    item.setCostPrice(std::to_string(dto.getCostPrice()));

    // Shipping
    item.setWeight(std::to_string(dto.getWeight()));
    item.setIsFreeShipping(dto.getIsFreeShipping());
    item.setShipSeparately(dto.getShipSeparately());
    item.setUomCodeId(dto.getUomCodeId());
    item.setUomName(dto.getUomName());

    // Category / tags (add categorySlug)
    item.setCategoryId(dto.getCategoryId());
    item.setCategoryName(dto.getCategoryName());
    item.setCategorySlug(dto.getCategorySlug());
    item.setMinimumCartQuantity(dto.getMinimumCartQuantity());
    item.setMaximumCartQuantity(dto.getMaximumCartQuantity());
    item.setBranchId(dto.getBranchId());
    item.setBranchName(dto.getBranchName());
    item.setShelfLocation(dto.getShelfLocation());

    // Flags
    item.setIsPublished(dto.getIsPublished());
    item.setAllowCustomerReviews(dto.getAllowCustomerReviews());
    item.setMarkAsNew(dto.getMarkAsNew());
    item.setDisableBuyButton(dto.getDisableBuyButton());
    item.setDisableWishlistButton(dto.getDisableWishlistButton());
    item.setAvailableForPreOrder(dto.getAvailableForPreOrder());
    item.setIsTaxExempt(dto.getIsTaxExempt());
    item.setIsNotReturnable(dto.getIsNotReturnable());
    item.setIsRental(dto.getIsRental());
    item.setIsService(dto.getIsService());
    item.setRating(dto.getRating());
    item.setReviewsCount(dto.getReviewsCount());

    // Dates
    if (!dto.getAvailableStartDate().empty()) {
      item.setAvailableStartDate(
          trantor::Date::fromDbStringLocal(dto.getAvailableStartDate()));
    }
    if (!dto.getAvailableEndDate().empty()) {
      item.setAvailableEndDate(
          trantor::Date::fromDbStringLocal(dto.getAvailableEndDate()));
    }

    // SEO
    item.setMetaTitle(dto.getMetaTitle());
    item.setMetaKeywords(dto.getMetaKeywords());
    item.setMetaDescription(dto.getMetaDescription());
    item.setLeadTime(dto.getLeadTime());

    // ── JSON arrays → serialize to JSON strings for the DB ───────────────
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";

    auto writeJson = [&](const Json::Value &v) -> std::string {
      if (v.isNull() || (!v.isArray() && !v.isObject()))
        return "[]";
      return Json::writeString(writer, v);
    };

    item.setFullDescription(writeJson(dto.getFullDescription()));
    item.setImages(writeJson(dto.getImages()));
    item.setTags(writeJson(dto.getTagList()));
    item.setVariants(writeJson(dto.getVariants()));
    item.setProductAttributes(writeJson(dto.getProductAttributes()));
    item.setSpecifications(writeJson(dto.getSpecifications()));
    item.setBulkPricing(writeJson(dto.getBulkPricing()));

    LOG_DEBUG << "[syncPosInventoryItems] Item model populated. Sku: "
              << item.getValueOfSku() << ", Name: " << item.getValueOfName()
              << ", SellingPrice: " << item.getValueOfSellingPrice()
              << ", Images count: " << dto.getImages().size()
              << ", Tags count: " << dto.getTagList().size()
              << ", Attributes count: " << dto.getProductAttributes().size();

    // ---------------------------------------------------------------
    // CREATE
    // ---------------------------------------------------------------
    if (operation == "create") {
      LOG_INFO << "[syncPosInventoryItems] Attempting to INSERT item: "
               << dto.getSku();

      item.setCreatedAt(trantor::Date::now());
      co_await mapper.insert(item);

      LOG_INFO << "[syncPosInventoryItems] INSERT successful for item: "
               << dto.getSku();

      response.message = "Inventory item created successfully";
      response.success = true;
      co_return response;
    }

    // ---------------------------------------------------------------
    // UPDATE
    // ---------------------------------------------------------------
    if (operation == "update") {
      LOG_INFO << "[syncPosInventoryItems] Attempting to UPDATE item: "
               << dto.getSku();

      // Look up existing row by (business_id, sku) to obtain the primary key,
      // then copy it onto the model before updating.
      auto existing = co_await mapper.findOne(
          drogon::orm::Criteria(drogon_model::DataSupportDb::PosInventoryItems::
                                    Cols::_business_id,
                                drogon::orm::CompareOperator::EQ,
                                businessAccountId) &&
          drogon::orm::Criteria(
              drogon_model::DataSupportDb::PosInventoryItems::Cols::_sku,
              drogon::orm::CompareOperator::EQ, dto.getSku()));

      item.setId(existing.getValueOfId());
      item.setUpdatedAt(trantor::Date::now());

      auto updatedCount = co_await mapper.update(item);

      LOG_INFO << "[syncPosInventoryItems] UPDATE successful for item: "
               << dto.getSku() << ", rows affected: " << updatedCount;

      response.message = "Inventory item updated successfully";
      response.success = true;
      co_return response;
    }

    // ---------------------------------------------------------------
    // UNKNOWN OPERATION
    // ---------------------------------------------------------------
    LOG_WARN << "[syncPosInventoryItems] Unsupported operation: '" << operation
             << "' for item: " << dto.getSku();
    response.message = "Unsupported operation: " + operation;
    response.success = false;

  } catch (const drogon::orm::UnexpectedRows &e) {
    LOG_ERROR << "[syncPosInventoryItems] UnexpectedRows exception: "
              << e.what() << ". The inventory item may not exist for update.";
    response.message =
        "Record not found or database inconsistency: " + std::string(e.what());
    response.success = false;

  } catch (const drogon::orm::DrogonDbException &e) {
    LOG_ERROR << "[syncPosInventoryItems] Database error: " << e.base().what();
    response.message =
        "Database error occurred while syncing inventory item: " +
        std::string(e.base().what());
    response.success = false;

  } catch (const std::exception &e) {
    LOG_ERROR << "[syncPosInventoryItems] General exception: " << e.what();
    response.message =
        "Failed to sync inventory item. Please try again. Error: " +
        std::string(e.what());
    response.success = false;
  }

  LOG_INFO << "[syncPosInventoryItems] Exiting method. Success: "
           << response.success << ", Message: '" << response.message << "'";

  co_return response;
}

} // namespace data_support::services
