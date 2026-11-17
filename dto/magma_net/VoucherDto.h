//
// MagmaNet — voucher DTOs.
//

#ifndef DATASUPPORTAPI_MAGMANETVOUCHERDTO_H
#define DATASUPPORTAPI_MAGMANETVOUCHERDTO_H

#include <json/json.h>
#include <cstdlib>
#include <string>

namespace data_support::dto::magma_net {

    struct VoucherBatchDto {
        std::string planId, planCode, zoneId;
        int quantity = 0;
        int expiresInDays = 90;
        double unitPrice = 0;
        double wholesalePrice = 0;
        std::string ownerAgentId;

        static VoucherBatchDto fromJson(const Json::Value &json) {
            VoucherBatchDto dto;
            dto.planId = json.get("planId", "").asString();
            dto.planCode = json.get("planCode", "").asString();
            dto.zoneId = json.get("zoneId", "").asString();
            auto number = [&json](const char *key, double fallback) {
                if (!json.isMember(key) || json[key].isNull()) return fallback;
                if (json[key].isString()) {
                    try { return std::stod(json[key].asString()); } catch (...) { return fallback; }
                }
                return json[key].asDouble();
            };
            dto.quantity = static_cast<int>(number("quantity", 0));
            dto.expiresInDays = static_cast<int>(number("expiresInDays", 90));
            dto.unitPrice = number("unitPrice", 0.0);
            dto.wholesalePrice = number("wholesalePrice", 0.0);
            dto.ownerAgentId = json.get("ownerAgentId", "").asString();
            return dto;
        }
    };

    struct VoucherExportDto {
        std::string batchId;
        std::string format = "CSV";
    };

} // namespace data_support::dto::magma_net

#endif //DATASUPPORTAPI_MAGMANETVOUCHERDTO_H
