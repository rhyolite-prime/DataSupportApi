//
// Created by Emmanuel Addo-Odame on 23/08/2026.
//

#ifndef DATASUPPORTAPI_ERPAPI_H
#define DATASUPPORTAPI_ERPAPI_H

#include <drogon/drogon.h>
#include "dto/BaseApiResponse.h"
#include "dto/erp_api/BasePosDto.h"
#include "dto/erp_api/SyncApiCredentialDto.h"
#include "dto/erp_api/UnifiedFilterDto.h"
#include "dto/erp_api/UnifiedSyncDto.h"

namespace data_support::services {

    class ErpApi {
    public:
        //------- POS FUNCTIONS ----------
        //setup resources
        drogon::Task<dto::BaseApiResponse> getBranches();
        drogon::Task<dto::BaseApiResponse> getCoupons();
        drogon::Task<dto::BaseApiResponse> getCategories(const std::string &businessId, int pageNo, int pageSize);
        drogon::Task<dto::BaseApiResponse> getCurrencies();
        drogon::Task<dto::BaseApiResponse> getDeliveryChannels();
        drogon::Task<dto::BaseApiResponse> getPickupPoints();

        //auth
        drogon::Task<dto::BaseApiResponse> verifyCustomer();

        drogon::Task<dto::erp_api::ErpOperationResponse> posCustomerSignup(const std::string &businessId, const dto::erp_api::PosCustomerSignupDto &dto);

        drogon::Task<dto::erp_api::ErpOperationResponse> posCustomerLogin(const std::string &businessId, const dto::erp_api::PosCustomerLoginDto &dto);

        drogon::Task<dto::BaseApiResponse> resetPassword(const std::string &businessId, const std::string &customerHeader);
        drogon::Task<dto::BaseApiResponse> verifyOtp(const std::string &businessId, const std::string &customerHeader);

        // cart
        drogon::Task<dto::erp_api::ErpOperationResponse> addItemToCart(const std::string &businessId, const std::string &customerHeader, const dto::erp_api::PosShopCartDto &dto);
        drogon::Task<dto::erp_api::ErpOperationResponse> getCartItemCount(const std::string &businessId, const std::string &customerHeader);
        drogon::Task<dto::erp_api::ErpOperationResponse> getCartItems(const std::string &businessId, const std::string &customerHeader);

        drogon::Task<dto::erp_api::ErpOperationResponse> removeItemFromCart(const std::string &businessId, const std::string &customerHeader, const std::string &itemId);
        drogon::Task<dto::erp_api::ErpOperationResponse> clearCart(const std::string &businessId, const std::string &customerHeader);

        //wish list
        drogon::Task<dto::erp_api::ErpOperationResponse> addItemToWishlist(const std::string &businessId, const std::string &customerHeader, const std::string &itemId);
        drogon::Task<dto::erp_api::ErpOperationResponse> removeItemFromWishlist(const std::string &businessId, const std::string &customerHeader, const std::string &itemId);
        drogon::Task<dto::erp_api::ErpOperationResponse> getWishlistItemCount(const std::string &businessId, const std::string &customerHeader);
        drogon::Task<dto::erp_api::ErpOperationResponse> getWishlistItems(const std::string &businessId, const std::string &customerHeader);


        //checkout
        drogon::Task<dto::BaseApiResponse> initializeWebCheckout(const std::string &businessId, const std::string &customerHeader);
        drogon::Task<dto::BaseApiResponse> initializeUssdCheckout(const std::string &businessId, const std::string &customerHeader);
        drogon::Task<dto::BaseApiResponse> processPaymentCallback();

        //inventory
        drogon::Task<dto::BaseApiResponse> getInventoryItems(const std::string &businessId, const dto::erp_api::InventoryItemFilterDto &dto);

        drogon::Task<dto::BaseApiResponse> getInventoryItemDetail(const std::string &businessId, const std::string &slug);

        //sales
        drogon::Task<dto::BaseApiResponse> getCustomerOrderHistory(const std::string &businessId, const std::string &customerHeader);

        //profile
        drogon::Task<dto::BaseApiResponse> getUserProfile(const std::string &businessId, const std::string &customerHeader);
        drogon::Task<dto::BaseApiResponse> changePassword(const std::string &businessId, const std::string &customerHeader);
        drogon::Task<dto::BaseApiResponse> updateUserProfile(const std::string &businessId, const std::string &customerHeader);
        drogon::Task<dto::BaseApiResponse> getNotifications(const std::string &businessId, const std::string &customerHeader);


        //------- PAYROLL FUNCTIONS ----------



        //------- GL FUNCTIONS ----------



        //------- SCHOOL MANAGER FUNCTIONS ----------



        //------- SYNC API CREDENTIALS ----------

        drogon::Task<dto::BaseApiResponse> syncApiCredentials(const dto::erp_api::SyncApiCredentialDto &dto);

        drogon::Task<dto::BaseApiResponse> syncPosBranches(const dto::erp_api::SyncPosBranchDto &dto);

        drogon::Task<dto::BaseApiResponse> syncPosInventoryCategories(const dto::erp_api::SyncInventoryCategoryDto &dto);

        drogon::Task<dto::BaseApiResponse> syncPosCoupons(const dto::erp_api::SyncPosCouponDto &dto);

        drogon::Task<dto::BaseApiResponse> syncErpCurrencies(const dto::erp_api::SyncErpCurrencyDto &dto);

        drogon::Task<dto::BaseApiResponse> syncPosDeliveryChannels(const dto::erp_api::SyncPosDeliveryChannelDto &dto);

        drogon::Task<dto::BaseApiResponse> syncPosPickupPoints(const dto::erp_api::SyncPosPickupPointDto &dto);

        drogon::Task<dto::BaseApiResponse> syncPosInventoryItems(const dto::erp_api::SyncPosInventoryItemDto &dto);

        //drogon::Task<dto::BaseApiResponse> syncPosBannerSliders(const dto::erp_api::SyncPosBannerSliderDto &dto);


    };

}

#endif //DATASUPPORTAPI_ERPAPI_H
