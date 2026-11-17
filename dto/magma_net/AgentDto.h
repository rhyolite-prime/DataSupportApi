//
// MagmaNet — agent DTOs. AgentAuthDto mirrors WaecMerchantAuthDto: identifier
// (phone or email) plus a bcrypt-hashed PIN.
//

#ifndef DATASUPPORTAPI_MAGMANETAGENTDTO_H
#define DATASUPPORTAPI_MAGMANETAGENTDTO_H

#include <json/json.h>
#include <cstdlib>
#include <string>

namespace data_support::dto::magma_net {

    class AgentAuthDto {
    public:
        AgentAuthDto() = default;
        explicit AgentAuthDto(const Json::Value &json) { fromJson(json); }

        void fromJson(const Json::Value &json) {
            agentIdentifier = json.get("agentIdentifier", "").asString();
            pin = json.get("pin", "").asString();
        }

        [[nodiscard]] const std::string &getAgentIdentifier() const { return agentIdentifier; }
        [[nodiscard]] const std::string &getPin() const { return pin; }

        std::string agentIdentifier, pin;
    };

    class AgentDto {
    public:
        AgentDto() = default;
        explicit AgentDto(const Json::Value &json) { fromJson(json); }

        void fromJson(const Json::Value &json) {
            agentId = json.get("agentId", "").asString();
            name = json.get("name", "").asString();
            msisdn = json.get("msisdn", "").asString();
            email = json.get("email", "").asString();
            pin = json.get("pin", "").asString();
            tier = json.get("tier", "STANDARD").asString();
            territoryZoneId = json.get("territoryZoneId", "").asString();
            commissionRate = json.get("commissionRate", 0.0).asDouble();
            kycStatus = json.get("kycStatus", "PENDING").asString();
        }

        std::string agentId, name, msisdn, email, pin;
        std::string tier = "STANDARD", territoryZoneId, kycStatus = "PENDING";
        double commissionRate = 0;
    };

    struct AgentSettlementDto {
        double amount = 0;
        std::string msisdn, provider = "mtn-gh", reference, agentId;

        static AgentSettlementDto fromJson(const Json::Value &json) {
            AgentSettlementDto dto;
            const auto amount = json.get("amount", Json::Value(0.0));
            if (amount.isString()) {
                try { dto.amount = std::stod(amount.asString()); } catch (...) { dto.amount = 0; }
            } else {
                dto.amount = amount.asDouble();
            }
            dto.msisdn = json.get("msisdn", "").asString();
            dto.provider = json.get("provider", "mtn-gh").asString();
            dto.reference = json.get("reference", "").asString();
            dto.agentId = json.get("agentId", "").asString();
            return dto;
        }
    };

    struct AgentPinResetDto {
        std::string agentIdentifier, otp, newPin;

        static AgentPinResetDto fromJson(const Json::Value &json) {
            AgentPinResetDto dto;
            dto.agentIdentifier = json.get("agentIdentifier", "").asString();
            dto.otp = json.get("otp", "").asString();
            dto.newPin = json.get("newPin", "").asString();
            return dto;
        }
    };

    struct AgentStockPurchaseDto {
        std::string agentId, planCode, zoneId;
        int quantity = 0;

        static AgentStockPurchaseDto fromJson(const Json::Value &json) {
            AgentStockPurchaseDto dto;
            dto.agentId = json.get("agentId", "").asString();
            dto.planCode = json.get("planCode", "").asString();
            dto.zoneId = json.get("zoneId", "").asString();
            const auto quantity = json.get("quantity", Json::Value(0));
            dto.quantity = quantity.isString() ? std::atoi(quantity.asString().c_str())
                                               : quantity.asInt();
            return dto;
        }
    };

} // namespace data_support::dto::magma_net

#endif //DATASUPPORTAPI_MAGMANETAGENTDTO_H
