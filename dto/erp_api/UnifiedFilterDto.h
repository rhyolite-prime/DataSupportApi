//
// Created by Emmanuel Addo-Odame on 10/09/2026.
//

#ifndef DATASUPPORTAPI_UNIFIEDFILTERDTO_H
#define DATASUPPORTAPI_UNIFIEDFILTERDTO_H
#include <json/json.h>
#include <string>

namespace data_support::dto::erp_api {

    class InventoryItemFilterDto {
    public:
        InventoryItemFilterDto() = default;

        void fromJson(const Json::Value& json);

        // Currency getters
        [[nodiscard]] int getPageNo() const { return pageNo_; }
        [[nodiscard]] int getPageSize() const { return pageSize_; }
        [[nodiscard]] const std::string& getCategoryId() const { return categoryId_; }
        [[nodiscard]] const std::string& getKeywords() const { return keywords_; }
        [[nodiscard]] const std::string& getCategorySlug() const { return categorySlug_; }
        [[nodiscard]] const std::string& getSortBy() const { return sortBy_; }


        void setPageNo(const int value) { pageNo_ = value; }
        void setPageSize(const int value) { pageSize_ = value; }
        void setCategoryId(const std::string& value) { categoryId_ = value; }
        void setKeywords(const std::string& value) { keywords_ = value; }
        void setCategorySlug(const std::string& value) { categorySlug_ = value; }
        void setSortBy(const std::string& value) { sortBy_ = value; }

    private:

        int pageNo_ {1};
        int pageSize_ {20};
        std::string categoryId_;
        std::string categorySlug_;
        std::string keywords_;
        std::string sortBy_;

    };

    inline void InventoryItemFilterDto::fromJson(const Json::Value& json) {


        if (json.isMember("pageNo") && !json["pageNo"].isNull()) {
            setPageNo(json["pageNo"].asInt());
        }

        if (json.isMember("pageSize") && !json["pageSize"].isNull()) {
            setPageSize(json["pageSize"].asInt());
        }

        if (json.isMember("categoryId") && !json["categoryId"].isNull()) {
            setCategoryId(json["categoryId"].asString());
        }

        if (json.isMember("keywords") && !json["keywords"].isNull()) {
            setKeywords(json["keywords"].asString());
        }

        if (json.isMember("categorySlug") && !json["categorySlug"].isNull()) {
            setCategorySlug(json["categorySlug"].asString());
        }

        if (json.isMember("sortBy") && !json["sortBy"].isNull()) {
            setCategorySlug(json["sortBy"].asString());
        }

    }

}
#endif //DATASUPPORTAPI_UNIFIEDFILTERDTO_H
