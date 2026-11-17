//
// MagmaNet — captive portal DTOs.
//

#ifndef DATASUPPORTAPI_MAGMANETCHECKOUTDTO_H
#define DATASUPPORTAPI_MAGMANETCHECKOUTDTO_H

#include <json/json.h>
#include <string>

namespace data_support::dto::magma_net {

    struct CheckoutIntentDto {
        std::string zoneCode, planCode, mac, ip, msisdn;
        std::string paymentMethod = "MOMO_MTN";
        std::string email, customerName;

        static CheckoutIntentDto fromJson(const Json::Value &json) {
            CheckoutIntentDto dto;
            dto.zoneCode = json.get("zoneCode", "").asString();
            dto.planCode = json.get("planCode", "").asString();
            dto.mac = json.get("mac", "").asString();
            dto.ip = json.get("ip", "").asString();
            dto.msisdn = json.get("msisdn", "").asString();
            dto.paymentMethod = json.get("paymentMethod", "MOMO_MTN").asString();
            dto.email = json.get("email", "").asString();
            dto.customerName = json.get("customerName", "").asString();
            return dto;
        }
    };

    struct VoucherRedeemDto {
        std::string code, zoneCode, mac, ip, msisdn;

        static VoucherRedeemDto fromJson(const Json::Value &json) {
            VoucherRedeemDto dto;
            dto.code = json.get("code", "").asString();
            dto.zoneCode = json.get("zoneCode", "").asString();
            dto.mac = json.get("mac", "").asString();
            dto.ip = json.get("ip", "").asString();
            dto.msisdn = json.get("msisdn", "").asString();
            return dto;
        }
    };

    struct OtpRequestDto {
        std::string msisdn, zoneCode, purpose = "LOGIN";
    };

    struct OtpVerifyDto {
        std::string msisdn, code, purpose = "LOGIN";
    };

} // namespace data_support::dto::magma_net

#endif //DATASUPPORTAPI_MAGMANETCHECKOUTDTO_H
