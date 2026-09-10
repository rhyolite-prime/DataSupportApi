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

}
#endif //DATASUPPORTAPI_SYNCENTITYDATADTO_H
