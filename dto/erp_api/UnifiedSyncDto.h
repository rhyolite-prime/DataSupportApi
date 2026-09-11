//
// Created by Emmanuel Addo-Odame on 06/09/2026.
//

#ifndef DATASUPPORTAPI_SYNCENTITYDATADTO_H
#define DATASUPPORTAPI_SYNCENTITYDATADTO_H
#include <json/json.h>
#include <string>

namespace data_support::dto::erp_api {

    class SyncPosBranchDto {

    public:

        SyncPosBranchDto() = default;

        void fromJson(const Json::Value& json);

        // Getters for wrapper fields
        [[nodiscard]] const std::string& getOperation() const { return operation_; }
        [[nodiscard]] const std::string& getBusinessAccountId() const { return business_account_id_; }

        [[nodiscard]] const std::string& getId() const { return id_; }
        [[nodiscard]] const std::string& getName() const { return name_; }
        [[nodiscard]] const std::string& getLocation() const { return location_; }
        [[nodiscard]] const std::string& getCountryName() const { return country_name_; }
        [[nodiscard]] const std::string& getCountryCode() const { return country_code_; }
        [[nodiscard]] const std::string& getRegionOrState() const { return region_or_state_; }
        [[nodiscard]] const std::string& getCity() const { return city_; }
        [[nodiscard]] const std::string& getAddress() const { return address_; }
        [[nodiscard]] const std::string& getPhoneNo() const { return phone_no_; }
        [[nodiscard]] const std::string& getBranchManager() const { return branch_manager_; }
        [[nodiscard]] const std::string& getRevenueAccountId() const { return revenue_account_id_; }
        [[nodiscard]] const std::string& getRevenueAccountNo() const { return revenue_account_no_; }
        [[nodiscard]] const std::string& getGeoCoordinates() const { return geo_coordinates_; }
        [[nodiscard]] bool getIsDefault() const { return is_default_; }
        [[nodiscard]] int getTenantId() const { return tenant_id_; }

        // Setters
        void setOperation(const std::string& value) { operation_ = value; }
        void setBusinessAccountId(const std::string& value) { business_account_id_ = value; }

        void setId(const std::string& value) { id_ = value; }
        void setName(const std::string& value) { name_ = value; }
        void setLocation(const std::string& value) { location_ = value; }
        void setCountryName(const std::string& value) { country_name_ = value; }
        void setCountryCode(const std::string& value) { country_code_ = value; }
        void setRegionOrState(const std::string& value) { region_or_state_ = value; }
        void setCity(const std::string& value) { city_ = value; }
        void setAddress(const std::string& value) { address_ = value; }
        void setPhoneNo(const std::string& value) { phone_no_ = value; }
        void setBranchManager(const std::string& value) { branch_manager_ = value; }
        void setRevenueAccountId(const std::string& value) { revenue_account_id_ = value; }
        void setRevenueAccountNo(const std::string& value) { revenue_account_no_ = value; }
        void setGeoCoordinates(const std::string& value) { geo_coordinates_ = value; }
        void setIsDefault(bool value) { is_default_ = value; }
        void setTenantId(int value) { tenant_id_ = value; }


    private:

        std::string operation_;
        std::string business_account_id_;

        std::string id_;
        std::string name_;
        std::string location_;
        std::string country_name_;
        std::string country_code_;
        std::string region_or_state_;
        std::string city_;
        std::string address_;
        std::string phone_no_;
        std::string branch_manager_;
        std::string revenue_account_id_;
        std::string revenue_account_no_;
        std::string geo_coordinates_;
        bool is_default_ = false;
        int tenant_id_ {0};

    };

    inline void SyncPosBranchDto::fromJson(const Json::Value& json) {

        // Parse wrapper fields (top-level)
        if (json.isMember("Operation") && !json["Operation"].isNull()) {
            setOperation(json["Operation"].asString());
        }
        if (json.isMember("BusinessAccountId") && !json["BusinessAccountId"].isNull()) {
            setBusinessAccountId(json["BusinessAccountId"].asString());
        }

        const Json::Value& data = json.isMember("data") ? json["data"] : json;

            if (data.isMember("Id") && !data["Id"].isNull()) {
                id_ = data["Id"].asString();
            }

            if (data.isMember("Name") && !data["Name"].isNull()) {
                name_ = data["Name"].asString();
            }
            if (data.isMember("Location") && !data["Location"].isNull()) {
                location_ = data["Location"].asString();
            }
            if (data.isMember("CountryName") && !data["CountryName"].isNull()) {
                country_name_ = data["CountryName"].asString();
            }
            if (data.isMember("CountryCode") && !data["CountryCode"].isNull()) {
                country_code_ = data["CountryCode"].asString();
            }
            if (data.isMember("RegionOrState") && !data["RegionOrState"].isNull()) {
                region_or_state_ = data["RegionOrState"].asString();
            }
            if (data.isMember("City") && !data["City"].isNull()) {
                city_ = data["City"].asString();
            }
            if (data.isMember("Address") && !data["Address"].isNull()) {
                address_ = data["Address"].asString();
            }
            if (data.isMember("PhoneNo") && !data["PhoneNo"].isNull()) {
                phone_no_ = data["PhoneNo"].asString();
            }
            if (data.isMember("BranchManager") && !data["BranchManager"].isNull()) {
                branch_manager_ = data["BranchManager"].asString();
            }
            if (data.isMember("RevenueAccountId") && !data["RevenueAccountId"].isNull()) {
                revenue_account_id_ = data["RevenueAccountId"].asString();
            }
            if (data.isMember("RevenueAccountNo") && !data["RevenueAccountNo"].isNull()) {
                revenue_account_no_ = data["RevenueAccountNo"].asString();
            }
            if (data.isMember("GeoCoordinates") && !data["GeoCoordinates"].isNull()) {
                geo_coordinates_ = data["GeoCoordinates"].asString();
            }
            if (data.isMember("IsDefault") && !data["IsDefault"].isNull()) {
                is_default_ = data["IsDefault"].asBool();
            }
            if (data.isMember("TenantId") && !data["TenantId"].isNull()) {
                tenant_id_ = data["TenantId"].asInt();
            }

    }

    class SyncInventoryCategoryDto {
        public:

        SyncInventoryCategoryDto() = default;

        void fromJson(const Json::Value& json);

        // Getters for wrapper fields
        [[nodiscard]] const std::string& getOperation() const { return operation_; }
        [[nodiscard]] const std::string& getBusinessAccountId() const { return business_account_id_; }

        [[nodiscard]] const std::string& getId() const { return id_; }
        [[nodiscard]] const std::string& getName() const { return name_; }
        [[nodiscard]] const std::string& getSlug() const { return slug_; }
        [[nodiscard]] const std::string& getImageUrl() const { return image_url_; }
        [[nodiscard]] const std::string& getDescription() const { return description_; }
        [[nodiscard]] const std::string& getParentCategoryId() const { return parent_category_id_; }
        [[nodiscard]] bool getIsService() const { return is_service_; }
        [[nodiscard]] int getProductCount() const { return product_count_; }
        [[nodiscard]] int getTenantId() const { return tenant_id_; }

        // Setters
        void setOperation(const std::string& value) { operation_ = value; }
        void setBusinessAccountId(const std::string& value) { business_account_id_ = value; }

        void setId(const std::string& value) { id_ = value; }
        void setName(const std::string& value) { name_ = value; }
        void setSlug(const std::string& value) { slug_ = value; }
        void setImageUrl(const std::string& value) { image_url_ = value; }
        void setDescription(const std::string& value) { description_ = value; }
        void setParentCategoryId(const std::string& value) { parent_category_id_ = value; }
        void setIsService(bool value) { is_service_ = value; }
        void setProductCount(int value) { product_count_ = value; }
        void setTenantId(int value) { tenant_id_ = value; }


    private:

        std::string operation_;
        std::string business_account_id_;

        std::string id_;
        std::string name_;
        std::string image_url_;
        std::string slug_;
        std::string description_;
        bool is_service_ {false};
        std::string parent_category_id_;
        int product_count_ {0};
        int tenant_id_ {0};
    };

    inline void SyncInventoryCategoryDto::fromJson(const Json::Value& json) {

        // Parse wrapper fields (top-level)
        if (json.isMember("Operation") && !json["Operation"].isNull()) {
            setOperation(json["Operation"].asString());
        }
        if (json.isMember("BusinessAccountId") && !json["BusinessAccountId"].isNull()) {
            setBusinessAccountId(json["BusinessAccountId"].asString());
        }

        const Json::Value& data = json.isMember("data") ? json["data"] : json;

            if (data.isMember("Id") && !data["Id"].isNull()) {
                id_ = data["Id"].asString();
            }

            if (data.isMember("Name") && !data["Name"].isNull()) {
                name_ = data["Name"].asString();
            }

            if (data.isMember("Slug") && !data["Slug"].isNull()) {
                slug_ = data["Slug"].asString();
            }

            if (data.isMember("ImageUrl") && !data["ImageUrl"].isNull()) {
                image_url_ = data["ImageUrl"].asString();
            }

            if (data.isMember("Description") && !data["Description"].isNull()) {
                description_ = data["Description"].asString();
            }

            if (data.isMember("ParentCategoryId") && !data["ParentCategoryId"].isNull()) {
                parent_category_id_ = data["ParentCategoryId"].asString();
            }

            if (data.isMember("IsService") && !data["IsService"].isNull()) {
                is_service_ = data["IsService"].asBool();
            }

            if (data.isMember("ProductCount") && !data["ProductCount"].isNull()) {
                product_count_ = data["ProductCount"].asInt();
            }

            if (data.isMember("TenantId") && !data["TenantId"].isNull()) {
                tenant_id_ = data["TenantId"].asInt();
            }

    }

    class SyncPosCouponDto {
    public:
        SyncPosCouponDto() = default;

        void fromJson(const Json::Value& json);

        // Wrapper getters
        [[nodiscard]] const std::string& getOperation() const { return operation_; }
        [[nodiscard]] const std::string& getBusinessAccountId() const { return business_account_id_; }

        // Coupon getters
        [[nodiscard]] double getCouponValue() const { return coupon_value_; }
        [[nodiscard]] double getMinimumOrderLimit() const { return minimum_order_limit_; }
        [[nodiscard]] double getMaximumOrderLimit() const { return maximum_order_limit_; }
        [[nodiscard]] const std::string& getCouponIdentifier() const { return coupon_identifier_; }
        [[nodiscard]] const std::string& getValidTill() const { return valid_till_; }
        [[nodiscard]] int getUsageQuota() const { return usage_quota_; }
        [[nodiscard]] int getUsageCount() const { return usage_count_; }
        [[nodiscard]] int getStatus() const { return status_; }
        [[nodiscard]] bool getUsePercentage() const { return use_percentage_; }
        [[nodiscard]] const std::string& getBranchId() const { return branch_id_; }
        [[nodiscard]] const std::string& getBranchName() const { return branch_name_; }
        [[nodiscard]] const Json::Value& getAssignments() const { return assignments_; }

        // Setters
        void setOperation(const std::string& value) { operation_ = value; }
        void setBusinessAccountId(const std::string& value) { business_account_id_ = value; }

        void setCouponValue(double value) { coupon_value_ = value; }
        void setMinimumOrderLimit(double value) { minimum_order_limit_ = value; }
        void setMaximumOrderLimit(double value) { maximum_order_limit_ = value; }
        void setCouponIdentifier(const std::string& value) { coupon_identifier_ = value; }
        void setValidTill(const std::string& value) { valid_till_ = value; }
        void setUsageQuota(int value) { usage_quota_ = value; }
        void setUsageCount(int value) { usage_count_ = value; }
        void setStatus(int value) { status_ = value; }
        void setUsePercentage(bool value) { use_percentage_ = value; }
        void setBranchId(const std::string& value) { branch_id_ = value; }
        void setBranchName(const std::string& value) { branch_name_ = value; }
        void setAssignments(const Json::Value& value) { assignments_ = value; }

    private:
        std::string operation_;
        std::string business_account_id_;

        double coupon_value_ {0.0};
        double minimum_order_limit_ {0.0};
        double maximum_order_limit_ {0.0};
        std::string coupon_identifier_;
        std::string valid_till_;
        int usage_quota_ {0};
        int usage_count_ {0};
        int status_ {0};
        bool use_percentage_ {false};
        std::string branch_id_;
        std::string branch_name_;
        Json::Value assignments_ {Json::arrayValue};

    };

    inline void SyncPosCouponDto::fromJson(const Json::Value& json) {

        // Parse wrapper fields (top-level)
        if (json.isMember("Operation") && !json["Operation"].isNull()) {
            setOperation(json["Operation"].asString());
        }
        if (json.isMember("BusinessAccountId") && !json["BusinessAccountId"].isNull()) {
            setBusinessAccountId(json["BusinessAccountId"].asString());
        }

        // The entity fields may be nested under "data" or "Data" or at the top level
        const Json::Value& data = json.isMember("data") ? json["data"] : json;

        if (data.isMember("CouponValue") && !data["CouponValue"].isNull()) {
            coupon_value_ = data["CouponValue"].asDouble();
        }
        if (data.isMember("MinimumOrderLimit") && !data["MinimumOrderLimit"].isNull()) {
            minimum_order_limit_ = data["MinimumOrderLimit"].asDouble();
        }
        if (data.isMember("MaximumOrderLimit") && !data["MaximumOrderLimit"].isNull()) {
            maximum_order_limit_ = data["MaximumOrderLimit"].asDouble();
        }
        if (data.isMember("CouponIdentifier") && !data["CouponIdentifier"].isNull()) {
            coupon_identifier_ = data["CouponIdentifier"].asString();
        }
        if (data.isMember("ValidTill") && !data["ValidTill"].isNull()) {
            valid_till_ = data["ValidTill"].asString();
        }
        if (data.isMember("UsageQuota") && !data["UsageQuota"].isNull()) {
            usage_quota_ = data["UsageQuota"].asInt();
        }
        if (data.isMember("UsageCount") && !data["UsageCount"].isNull()) {
            usage_count_ = data["UsageCount"].asInt();
        }
        if (data.isMember("Status") && !data["Status"].isNull()) {
            status_ = data["Status"].asInt();
        }
        if (data.isMember("UsePercentage") && !data["UsePercentage"].isNull()) {
            use_percentage_ = data["UsePercentage"].asBool();
        }
        if (data.isMember("BranchId") && !data["BranchId"].isNull()) {
            branch_id_ = data["BranchId"].asString();
        }
        if (data.isMember("BranchName") && !data["BranchName"].isNull()) {
            branch_name_ = data["BranchName"].asString();
        }
        if (data.isMember("Assignments") && !data["Assignments"].isNull()) {
            assignments_ = data["Assignments"];
        }
    }

    class SyncErpCurrencyDto {
    public:
        SyncErpCurrencyDto() = default;

        void fromJson(const Json::Value& json);

        // Wrapper getters
        [[nodiscard]] const std::string& getOperation() const { return operation_; }
        [[nodiscard]] const std::string& getBusinessAccountId() const { return business_account_id_; }

        // Currency getters
        [[nodiscard]] const std::string& getName() const { return name_; }
        [[nodiscard]] const std::string& getCode() const { return code_; }
        [[nodiscard]] const std::string& getMinorName() const { return minor_name_; }
        [[nodiscard]] double getBuyRate() const { return buy_rate_; }
        [[nodiscard]] double getSellRate() const { return sell_rate_; }
        [[nodiscard]] bool getIsDefault() const { return is_default_; }

        // Wrapper setters
        void setOperation(const std::string& value) { operation_ = value; }
        void setBusinessAccountId(const std::string& value) { business_account_id_ = value; }

        // Currency setters
        void setName(const std::string& value) { name_ = value; }
        void setCode(const std::string& value) { code_ = value; }
        void setMinorName(const std::string& value) { minor_name_ = value; }
        void setBuyRate(double value) { buy_rate_ = value; }
        void setSellRate(double value) { sell_rate_ = value; }
        void setIsDefault(bool value) { is_default_ = value; }

    private:
        std::string operation_;
        std::string business_account_id_;

        std::string name_;
        std::string code_;
        std::string minor_name_;
        double buy_rate_ {0.0};
        double sell_rate_ {0.0};
        bool is_default_ {false};
    };

    inline void SyncErpCurrencyDto::fromJson(const Json::Value& json) {

        // Parse wrapper fields (top-level)
        if (json.isMember("Operation") && !json["Operation"].isNull()) {
            setOperation(json["Operation"].asString());
        }
        if (json.isMember("BusinessAccountId") && !json["BusinessAccountId"].isNull()) {
            setBusinessAccountId(json["BusinessAccountId"].asString());
        }

        // The entity fields may be nested under "data" or at the top level
        const Json::Value& data = json.isMember("data") ? json["data"] : json;

        if (data.isMember("Name") && !data["Name"].isNull()) {
            name_ = data["Name"].asString();
        }
        if (data.isMember("Code") && !data["Code"].isNull()) {
            code_ = data["Code"].asString();
        }
        if (data.isMember("MinorName") && !data["MinorName"].isNull()) {
            minor_name_ = data["MinorName"].asString();
        }
        if (data.isMember("BuyRate") && !data["BuyRate"].isNull()) {
            buy_rate_ = data["BuyRate"].asDouble();
        }
        if (data.isMember("SellRate") && !data["SellRate"].isNull()) {
            sell_rate_ = data["SellRate"].asDouble();
        }
        if (data.isMember("IsDefault") && !data["IsDefault"].isNull()) {
            is_default_ = data["IsDefault"].asBool();
        }
    }

    class SyncPosDeliveryChannelDto {
    public:
        SyncPosDeliveryChannelDto() = default;

        void fromJson(const Json::Value& json);

        // Wrapper getters
        [[nodiscard]] const std::string& getOperation() const { return operation_; }
        [[nodiscard]] const std::string& getBusinessAccountId() const { return business_account_id_; }

        // Delivery channel getters
        [[nodiscard]] const std::string& getName() const { return name_; }
        [[nodiscard]] const std::string& getDescription() const { return description_; }
        [[nodiscard]] bool getIsLocal() const { return is_local_; }
        [[nodiscard]] const Json::Value& getZoneRates() const { return zone_rates_; }

        // Wrapper setters
        void setOperation(const std::string& value) { operation_ = value; }
        void setBusinessAccountId(const std::string& value) { business_account_id_ = value; }

        // Delivery channel setters
        void setName(const std::string& value) { name_ = value; }
        void setDescription(const std::string& value) { description_ = value; }
        void setIsLocal(bool value) { is_local_ = value; }
        void setZoneRates(const Json::Value& value) { zone_rates_ = value; }

    private:
        std::string operation_;
        std::string business_account_id_;

        std::string name_;
        std::string description_;
        bool is_local_ {false};
        Json::Value zone_rates_ {Json::arrayValue};
    };

    inline void SyncPosDeliveryChannelDto::fromJson(const Json::Value& json) {

        // Parse wrapper fields (top-level)
        if (json.isMember("Operation") && !json["Operation"].isNull()) {
            setOperation(json["Operation"].asString());
        }
        if (json.isMember("BusinessAccountId") && !json["BusinessAccountId"].isNull()) {
            setBusinessAccountId(json["BusinessAccountId"].asString());
        }

        // The entity fields may be nested under "data" or at the top level
        const Json::Value& data = json.isMember("data") ? json["data"] : json;

        if (data.isMember("Name") && !data["Name"].isNull()) {
            name_ = data["Name"].asString();
        }
        if (data.isMember("Description") && !data["Description"].isNull()) {
            description_ = data["Description"].asString();
        }
        if (data.isMember("IsLocal") && !data["IsLocal"].isNull()) {
            is_local_ = data["IsLocal"].asBool();
        }
        if (data.isMember("ZoneRates") && !data["ZoneRates"].isNull()) {
            zone_rates_ = data["ZoneRates"];
        }
    }

    class SyncPosPickupPointDto {
    public:
        SyncPosPickupPointDto() = default;

        void fromJson(const Json::Value& json);

        // Wrapper getters
        [[nodiscard]] const std::string& getOperation() const { return operation_; }
        [[nodiscard]] const std::string& getBusinessAccountId() const { return business_account_id_; }

        // Pickup point getters
        [[nodiscard]] const std::string& getFriendlyName() const { return friendly_name_; }
        [[nodiscard]] const std::string& getSystemName() const { return system_name_; }
        [[nodiscard]] const std::string& getImageUrl() const { return image_url_; }
        [[nodiscard]] const std::string& getGhanaPostGps() const { return ghana_post_gps_; }
        [[nodiscard]] const std::string& getGpsCoordinates() const { return gps_coordinates_; }
        [[nodiscard]] int getIsActive() const { return is_active_; }

        // Wrapper setters
        void setOperation(const std::string& value) { operation_ = value; }
        void setBusinessAccountId(const std::string& value) { business_account_id_ = value; }

        // Pickup point setters
        void setFriendlyName(const std::string& value) { friendly_name_ = value; }
        void setSystemName(const std::string& value) { system_name_ = value; }
        void setImageUrl(const std::string& value) { image_url_ = value; }
        void setGhanaPostGps(const std::string& value) { ghana_post_gps_ = value; }
        void setGpsCoordinates(const std::string& value) { gps_coordinates_ = value; }
        void setIsActive(int value) { is_active_ = value; }

    private:
        std::string operation_;
        std::string business_account_id_;

        std::string friendly_name_;
        std::string system_name_;
        std::string image_url_;
        std::string ghana_post_gps_;
        std::string gps_coordinates_;
        int is_active_ {0};
    };

    inline void SyncPosPickupPointDto::fromJson(const Json::Value& json) {

        // Parse wrapper fields (top-level)
        if (json.isMember("Operation") && !json["Operation"].isNull()) {
            setOperation(json["Operation"].asString());
        }
        if (json.isMember("BusinessAccountId") && !json["BusinessAccountId"].isNull()) {
            setBusinessAccountId(json["BusinessAccountId"].asString());
        }

        // The entity fields may be nested under "data" or at the top level
        const Json::Value& data = json.isMember("data") ? json["data"] : json;

        if (data.isMember("FriendlyName") && !data["FriendlyName"].isNull()) {
            friendly_name_ = data["FriendlyName"].asString();
        }
        if (data.isMember("SystemName") && !data["SystemName"].isNull()) {
            system_name_ = data["SystemName"].asString();
        }
        if (data.isMember("ImageUrl") && !data["ImageUrl"].isNull()) {
            image_url_ = data["ImageUrl"].asString();
        }
        if (data.isMember("GhanaPostGps") && !data["GhanaPostGps"].isNull()) {
            ghana_post_gps_ = data["GhanaPostGps"].asString();
        }
        if (data.isMember("GpsCoordinates") && !data["GpsCoordinates"].isNull()) {
            gps_coordinates_ = data["GpsCoordinates"].asString();
        }
        if (data.isMember("IsActive") && !data["IsActive"].isNull()) {
            is_active_ = data["IsActive"].asInt();
        }
    }

    class SyncPosInventoryItemDto {
    public:
        SyncPosInventoryItemDto() = default;

        void fromJson(const Json::Value& json);

        // Wrapper getters
        [[nodiscard]] const std::string& getOperation() const { return operation_; }
        [[nodiscard]] const std::string& getBusinessAccountId() const { return business_account_id_; }
        [[nodiscard]] const std::string& getId() const { return id_; }
        [[nodiscard]] int getTenantId() const { return tenant_id_; }

        // Basic getters
        [[nodiscard]] const std::string& getName() const { return name_; }
        [[nodiscard]] const std::string& getSlug() const { return slug_; }
        [[nodiscard]] const std::string& getShortDescription() const { return short_description_; }
        [[nodiscard]] const Json::Value& getFullDescription() const { return full_description_; }
        [[nodiscard]] const std::string& getAdminComment() const { return admin_comment_; }
        [[nodiscard]] const std::string& getMerchantId() const { return merchant_id_; }
        [[nodiscard]] const std::string& getMerchantName() const { return merchant_name_; }
        [[nodiscard]] const std::string& getSku() const { return sku_; }
        [[nodiscard]] const Json::Value& getImages() const { return images_; }
        [[nodiscard]] int getStockQuantity() const { return stock_quantity_; }
        [[nodiscard]] double getOldSellingPrice() const { return old_selling_price_; }
        [[nodiscard]] double getSellingPrice() const { return selling_price_; }
        [[nodiscard]] double getCostPrice() const { return cost_price_; }

        // Shipping getters
        [[nodiscard]] double getWeight() const { return weight_; }
        [[nodiscard]] bool getIsFreeShipping() const { return is_free_shipping_; }
        [[nodiscard]] bool getShipSeparately() const { return ship_separately_; }
        [[nodiscard]] const std::string& getUomCodeId() const { return uom_code_id_; }
        [[nodiscard]] const std::string& getUomName() const { return uom_name_; }
        [[nodiscard]] const std::string& getLeadTime() const { return lead_time_; }

        // Category / tags getters
        [[nodiscard]] const std::string& getCategoryId() const { return category_id_; }
        [[nodiscard]] const std::string& getCategoryName() const { return category_name_; }
        [[nodiscard]] const std::string& getCategorySlug() const { return category_slug_; }
        [[nodiscard]] const Json::Value& getTagList() const { return tag_list_; }
        [[nodiscard]] const Json::Value& getVariants() const { return variants_; }
        [[nodiscard]] int getMinimumCartQuantity() const { return minimum_cart_quantity_; }
        [[nodiscard]] int getMaximumCartQuantity() const { return maximum_cart_quantity_; }
        [[nodiscard]] const std::string& getBranchId() const { return branch_id_; }
        [[nodiscard]] const std::string& getBranchName() const { return branch_name_; }
        [[nodiscard]] const std::string& getShelfLocation() const { return shelf_location_; }

        // Flags getters
        [[nodiscard]] bool getIsPublished() const { return is_published_; }
        [[nodiscard]] bool getAllowCustomerReviews() const { return allow_customer_reviews_; }
        [[nodiscard]] const std::string& getAvailableStartDate() const { return available_start_date_; }
        [[nodiscard]] const std::string& getAvailableEndDate() const { return available_end_date_; }
        [[nodiscard]] bool getMarkAsNew() const { return mark_as_new_; }
        [[nodiscard]] bool getDisableBuyButton() const { return disable_buy_button_; }
        [[nodiscard]] bool getDisableWishlistButton() const { return disable_wishlist_button_; }
        [[nodiscard]] bool getAvailableForPreOrder() const { return available_for_pre_order_; }
        [[nodiscard]] bool getIsTaxExempt() const { return is_tax_exempt_; }
        [[nodiscard]] bool getIsNotReturnable() const { return is_not_returnable_; }
        [[nodiscard]] bool getIsRental() const { return is_rental_; }
        [[nodiscard]] bool getIsService() const { return is_service_; }

        // Attributes / SEO getters
        [[nodiscard]] const Json::Value& getProductAttributes() const { return product_attributes_; }
        [[nodiscard]] const Json::Value& getSpecifications() const { return specifications_; }
        [[nodiscard]] const Json::Value& getBulkPricing() const { return bulk_pricing_; }
        [[nodiscard]] const std::string& getMetaTitle() const { return meta_title_; }
        [[nodiscard]] const std::string& getMetaKeywords() const { return meta_keywords_; }
        [[nodiscard]] const std::string& getMetaDescription() const { return meta_description_; }
        [[nodiscard]] int getRating() const { return rating_; }
        [[nodiscard]] int getReviewsCount() const { return reviews_count_; }

        // Wrapper setters
        void setOperation(const std::string& value) { operation_ = value; }
        void setBusinessAccountId(const std::string& value) { business_account_id_ = value; }
        void setId(const std::string& value) { id_ = value; }
        void setTenantId(int value) { tenant_id_ = value; }

        // Basic setters
        void setName(const std::string& value) { name_ = value; }
        void setShortDescription(const std::string& value) { short_description_ = value; }
        void setFullDescription(const Json::Value& value) { full_description_ = value; }
        void setAdminComment(const std::string& value) { admin_comment_ = value; }
        void setMerchantId(const std::string& value) { merchant_id_ = value; }
        void setMerchantName(const std::string& value) { merchant_name_ = value; }
        void setSku(const std::string& value) { sku_ = value; }
        void setImages(const Json::Value& value) { images_ = value; }
        void setStockQuantity(int value) { stock_quantity_ = value; }
        void setOldSellingPrice(double value) { old_selling_price_ = value; }
        void setSellingPrice(double value) { selling_price_ = value; }
        void setCostPrice(double value) { cost_price_ = value; }

        // Shipping setters
        void setWeight(double value) { weight_ = value; }
        void setIsFreeShipping(bool value) { is_free_shipping_ = value; }
        void setShipSeparately(bool value) { ship_separately_ = value; }
        void setUomCodeId(const std::string& value) { uom_code_id_ = value; }
        void setUomName(const std::string& value) { uom_name_ = value; }

        // Category / tags setters
        void setCategoryId(const std::string& value) { category_id_ = value; }
        void setCategoryName(const std::string& value) { category_name_ = value; }
        void setTagList(const Json::Value& value) { tag_list_ = value; }
        void setMinimumCartQuantity(int value) { minimum_cart_quantity_ = value; }
        void setMaximumCartQuantity(int value) { maximum_cart_quantity_ = value; }
        void setBranchId(const std::string& value) { branch_id_ = value; }
        void setBranchName(const std::string& value) { branch_name_ = value; }
        void setShelfLocation(const std::string& value) { shelf_location_ = value; }

        // Flags setters
        void setIsPublished(bool value) { is_published_ = value; }
        void setAllowCustomerReviews(bool value) { allow_customer_reviews_ = value; }
        void setAvailableStartDate(const std::string& value) { available_start_date_ = value; }
        void setAvailableEndDate(const std::string& value) { available_end_date_ = value; }
        void setMarkAsNew(bool value) { mark_as_new_ = value; }
        void setDisableBuyButton(bool value) { disable_buy_button_ = value; }
        void setDisableWishlistButton(bool value) { disable_wishlist_button_ = value; }
        void setAvailableForPreOrder(bool value) { available_for_pre_order_ = value; }
        void setIsTaxExempt(bool value) { is_tax_exempt_ = value; }
        void setIsNotReturnable(bool value) { is_not_returnable_ = value; }
        void setIsRental(bool value) { is_rental_ = value; }
        void setIsService(bool value) { is_service_ = value; }

        // Attributes / SEO setters
        void setProductAttributes(const Json::Value& value) { product_attributes_ = value; }
        void setProductSpecifications(const Json::Value& value) { specifications_ = value; }
        void setMetaTitle(const std::string& value) { meta_title_ = value; }
        void setMetaKeywords(const std::string& value) { meta_keywords_ = value; }
        void setMetaDescription(const std::string& value) { meta_description_ = value; }
        void setLeadTime(const std::string& value) { lead_time_ = value; }


    private:
        std::string operation_;
        std::string business_account_id_;
        std::string id_;
        int tenant_id_ {0};

        // Basic fields
        std::string name_;
        std::string slug_;
        std::string short_description_;
        Json::Value full_description_;
        std::string admin_comment_;
        std::string merchant_id_;
        std::string merchant_name_;
        std::string sku_;
        Json::Value images_ {Json::arrayValue};
        int stock_quantity_ {0};
        double old_selling_price_ {0.0};
        double selling_price_ {0.0};
        double cost_price_ {0.0};

        // Shipping fields
        double weight_ {0.0};
        bool is_free_shipping_ {false};
        bool ship_separately_ {false};
        std::string uom_code_id_;
        std::string uom_name_;

        // Category / tags fields
        std::string category_id_;
        std::string category_name_;
        std::string category_slug_;
        Json::Value tag_list_ {Json::arrayValue};
        Json::Value variants_ {Json::arrayValue};
        int minimum_cart_quantity_ {0};
        int maximum_cart_quantity_ {0};
        std::string branch_id_;
        std::string branch_name_;
        std::string shelf_location_;

        // Flags
        bool is_published_ {false};
        bool allow_customer_reviews_ {false};
        std::string available_start_date_;
        std::string available_end_date_;
        bool mark_as_new_ {false};
        bool disable_buy_button_ {false};
        bool disable_wishlist_button_ {false};
        bool available_for_pre_order_ {false};
        bool is_tax_exempt_ {false};
        bool is_not_returnable_ {false};
        bool is_rental_ {false};
        bool is_service_ {false};

        // Attributes / SEO
        Json::Value product_attributes_ {Json::arrayValue};
        Json::Value specifications_ {Json::arrayValue};
        Json::Value bulk_pricing_ {Json::arrayValue};
        std::string meta_title_;
        std::string meta_keywords_;
        std::string meta_description_;
        std::string lead_time_;
        int rating_ {0};
        int reviews_count_ {0};
    };

    inline void SyncPosInventoryItemDto::fromJson(const Json::Value& json) {

        // Parse wrapper fields (top-level)
        if (json.isMember("Operation") && !json["Operation"].isNull()) {
            setOperation(json["Operation"].asString());
        }
        if (json.isMember("BusinessAccountId") && !json["BusinessAccountId"].isNull()) {
            setBusinessAccountId(json["BusinessAccountId"].asString());
        }
        if (json.isMember("Id") && !json["Id"].isNull()) {
            setId(json["Id"].asString());
        }
        if (json.isMember("TenantId") && !json["TenantId"].isNull()) {
            tenant_id_ = json["TenantId"].asInt();
        }

        // The entity fields may be nested under "data" or at the top level
        const Json::Value& data = json.isMember("data") ? json["data"] : json;

        // Basic fields
        if (data.isMember("Name") && !data["Name"].isNull()) {
            name_ = data["Name"].asString();
        }
        if (data.isMember("Slug") && !data["Slug"].isNull()) {
            slug_ = data["Slug"].asString();
        }
        if (data.isMember("ShortDescription") && !data["ShortDescription"].isNull()) {
            short_description_ = data["ShortDescription"].asString();
        }
        if (data.isMember("FullDescription") && !data["FullDescription"].isNull()) {
            // FullDescription is a JSON array — assign it directly, never call asString()
            full_description_ = data["FullDescription"];
        }
        if (data.isMember("AdminComment") && !data["AdminComment"].isNull()) {
            admin_comment_ = data["AdminComment"].asString();
        }
        if (data.isMember("MerchantId") && !data["MerchantId"].isNull()) {
            merchant_id_ = data["MerchantId"].asString();
        }
        if (data.isMember("MerchantName") && !data["MerchantName"].isNull()) {
            merchant_name_ = data["MerchantName"].asString();
        }
        if (data.isMember("Sku") && !data["Sku"].isNull()) {
            sku_ = data["Sku"].asString();
        }
        if (data.isMember("Images") && !data["Images"].isNull()) {
            images_ = data["Images"];
        }
        if (data.isMember("StockQuantity") && !data["StockQuantity"].isNull()) {
            stock_quantity_ = data["StockQuantity"].asInt();
        }
        if (data.isMember("OldSellingPrice") && !data["OldSellingPrice"].isNull()) {
            old_selling_price_ = data["OldSellingPrice"].asDouble();
        }
        if (data.isMember("SellingPrice") && !data["SellingPrice"].isNull()) {
            selling_price_ = data["SellingPrice"].asDouble();
        }
        if (data.isMember("CostPrice") && !data["CostPrice"].isNull()) {
            cost_price_ = data["CostPrice"].asDouble();
        }

        // Shipping fields
        if (data.isMember("Weight") && !data["Weight"].isNull()) {
            weight_ = data["Weight"].asDouble();
        }
        if (data.isMember("IsFreeShipping") && !data["IsFreeShipping"].isNull()) {
            is_free_shipping_ = data["IsFreeShipping"].asBool();
        }
        if (data.isMember("ShipSeparately") && !data["ShipSeparately"].isNull()) {
            ship_separately_ = data["ShipSeparately"].asBool();
        }
        if (data.isMember("UomCodeId") && !data["UomCodeId"].isNull()) {
            uom_code_id_ = data["UomCodeId"].asString();
        }
        if (data.isMember("UomName") && !data["UomName"].isNull()) {
            uom_name_ = data["UomName"].asString();
        }

        // Category / tags fields
        if (data.isMember("CategoryId") && !data["CategoryId"].isNull()) {
            category_id_ = data["CategoryId"].asString();
        }
        if (data.isMember("CategoryName") && !data["CategoryName"].isNull()) {
            category_name_ = data["CategoryName"].asString();
        }
        if (data.isMember("CategorySlug") && !data["CategorySlug"].isNull()) {
            category_slug_ = data["CategorySlug"].asString();
        }
        // Payload sends "Tags", not "TagList"
        if (data.isMember("Tags") && !data["Tags"].isNull()) {
            tag_list_ = data["Tags"];
        } else if (data.isMember("TagList") && !data["TagList"].isNull()) {
            tag_list_ = data["TagList"];
        }
        if (data.isMember("Variants") && !data["Variants"].isNull()) {
            variants_ = data["Variants"];
        }
        if (data.isMember("MinimumCartQuantity") && !data["MinimumCartQuantity"].isNull()) {
            minimum_cart_quantity_ = data["MinimumCartQuantity"].asInt();
        }
        if (data.isMember("MaximumCartQuantity") && !data["MaximumCartQuantity"].isNull()) {
            maximum_cart_quantity_ = data["MaximumCartQuantity"].asInt();
        }
        if (data.isMember("BranchId") && !data["BranchId"].isNull()) {
            branch_id_ = data["BranchId"].asString();
        }
        if (data.isMember("BranchName") && !data["BranchName"].isNull()) {
            branch_name_ = data["BranchName"].asString();
        }
        if (data.isMember("ShelfLocation") && !data["ShelfLocation"].isNull()) {
            shelf_location_ = data["ShelfLocation"].asString();
        }

        // Flags
        if (data.isMember("IsPublished") && !data["IsPublished"].isNull()) {
            is_published_ = data["IsPublished"].asBool();
        }
        if (data.isMember("AllowCustomerReviews") && !data["AllowCustomerReviews"].isNull()) {
            allow_customer_reviews_ = data["AllowCustomerReviews"].asBool();
        }
        if (data.isMember("AvailableStartDate") && !data["AvailableStartDate"].isNull()) {
            available_start_date_ = data["AvailableStartDate"].asString();
        }
        if (data.isMember("AvailableEndDate") && !data["AvailableEndDate"].isNull()) {
            available_end_date_ = data["AvailableEndDate"].asString();
        }
        if (data.isMember("MarkAsNew") && !data["MarkAsNew"].isNull()) {
            mark_as_new_ = data["MarkAsNew"].asBool();
        }
        if (data.isMember("DisableBuyButton") && !data["DisableBuyButton"].isNull()) {
            disable_buy_button_ = data["DisableBuyButton"].asBool();
        }
        if (data.isMember("DisableWishlistButton") && !data["DisableWishlistButton"].isNull()) {
            disable_wishlist_button_ = data["DisableWishlistButton"].asBool();
        }
        if (data.isMember("AvailableForPreOrder") && !data["AvailableForPreOrder"].isNull()) {
            available_for_pre_order_ = data["AvailableForPreOrder"].asBool();
        }
        if (data.isMember("IsTaxExempt") && !data["IsTaxExempt"].isNull()) {
            is_tax_exempt_ = data["IsTaxExempt"].asBool();
        }
        if (data.isMember("IsNotReturnable") && !data["IsNotReturnable"].isNull()) {
            is_not_returnable_ = data["IsNotReturnable"].asBool();
        }
        if (data.isMember("IsRental") && !data["IsRental"].isNull()) {
            is_rental_ = data["IsRental"].asBool();
        }
        if (data.isMember("IsService") && !data["IsService"].isNull()) {
            is_service_ = data["IsService"].asBool();
        }

        // Attributes / SEO
        if (data.isMember("ProductAttributes") && !data["ProductAttributes"].isNull()) {
            product_attributes_ = data["ProductAttributes"];
        }
        // Payload sends "Specifications", not "ProductSpecifications"
        if (data.isMember("Specifications") && !data["Specifications"].isNull()) {
            specifications_ = data["Specifications"];
        } else if (data.isMember("ProductSpecifications") && !data["ProductSpecifications"].isNull()) {
            specifications_ = data["ProductSpecifications"];
        }
        if (data.isMember("BulkPricing") && !data["BulkPricing"].isNull()) {
            bulk_pricing_ = data["BulkPricing"];
        }
        if (data.isMember("Rating") && !data["Rating"].isNull()) {
            rating_ = data["Rating"].asInt();
        }
        if (data.isMember("ReviewsCount") && !data["ReviewsCount"].isNull()) {
            reviews_count_ = data["ReviewsCount"].asInt();
        }
        if (data.isMember("MetaTitle") && !data["MetaTitle"].isNull()) {
            meta_title_ = data["MetaTitle"].asString();
        }
        if (data.isMember("MetaKeywords") && !data["MetaKeywords"].isNull()) {
            meta_keywords_ = data["MetaKeywords"].asString();
        }
        if (data.isMember("MetaDescription") && !data["MetaDescription"].isNull()) {
            meta_description_ = data["MetaDescription"].asString();
        }

        if (data.isMember("LeadTime") && !data["LeadTime"].isNull()) {
            lead_time_ = data["LeadTime"].asString();
        }


    }



}
#endif //DATASUPPORTAPI_SYNCENTITYDATADTO_H
