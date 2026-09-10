//
// Created by Emmanuel Addo-Odame on 23/08/2026.
//

#ifndef DATASUPPORTAPI_ERPAPI_H
#define DATASUPPORTAPI_ERPAPI_H

#include <drogon/drogon.h>
#include "dto/BaseApiResponse.h"
#include "dto/erp_api/SyncApiCredentialDto.h"
#include "dto/erp_api/UnifiedSyncDto.h"

namespace data_support::services {

    class ErpApi {
    public:
        //------- POS FUNCTIONS ----------
        //setup resources
        drogon::Task<dto::BaseApiResponse> getBranches();
        drogon::Task<dto::BaseApiResponse> getCoupons();
        drogon::Task<dto::BaseApiResponse> getCategories();
        drogon::Task<dto::BaseApiResponse> getCurrencies();
        drogon::Task<dto::BaseApiResponse> getDeliveryChannels();
        drogon::Task<dto::BaseApiResponse> getPickupPoints();

        //auth
        drogon::Task<dto::BaseApiResponse> verifyCustomer();
        drogon::Task<dto::BaseApiResponse> signup();
        drogon::Task<dto::BaseApiResponse> login();
        drogon::Task<dto::BaseApiResponse> resetPassword();
        drogon::Task<dto::BaseApiResponse> verifyOtp();

        // cart
        drogon::Task<dto::BaseApiResponse> addItemToCart();
        drogon::Task<dto::BaseApiResponse> addMultipleItemsToCart();
        drogon::Task<dto::BaseApiResponse> removeItemFromCart();
        drogon::Task<dto::BaseApiResponse> removeMultipleItemsFromCart();
        drogon::Task<dto::BaseApiResponse> clearCart();

        //checkout
        drogon::Task<dto::BaseApiResponse> initializeWebCheckout();
        drogon::Task<dto::BaseApiResponse> initializeUssdCheckout();
        drogon::Task<dto::BaseApiResponse> processPaymentCallback();

        //inventory
        drogon::Task<dto::BaseApiResponse> getInventoryItems();

        //sales
        drogon::Task<dto::BaseApiResponse> getOrderHistory();

        //profile
        drogon::Task<dto::BaseApiResponse> getUserProfile();
        drogon::Task<dto::BaseApiResponse> changePassword();
        drogon::Task<dto::BaseApiResponse> updateUserProfile();
        drogon::Task<dto::BaseApiResponse> getNotifications();


        //------- PAYROLL FUNCTIONS ----------



        //------- GL FUNCTIONS ----------



        //------- SCHOOL MANAGER FUNCTIONS ----------



        //------- SYNC API CREDENTIALS ----------

        drogon::Task<dto::BaseApiResponse> syncApiCredentials(const dto::erp_api::SyncApiCredentialDto &dto);

        drogon::Task<dto::BaseApiResponse> syncPosBranches(const dto::erp_api::SyncPosBranchDto &dto);

        drogon::Task<dto::BaseApiResponse> syncPosInventoryCategories(const dto::erp_api::SyncInventoryCategoryDto &dto);


    };

}

#endif //DATASUPPORTAPI_ERPAPI_H
