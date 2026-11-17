//
// MagmaNet — operations DTOs (tickets, work orders, costs, staff).
//

#ifndef DATASUPPORTAPI_MAGMANETOPSDTO_H
#define DATASUPPORTAPI_MAGMANETOPSDTO_H

#include <json/json.h>
#include <cstdlib>
#include <string>
#include <vector>

namespace data_support::dto::magma_net {

    struct TicketDto {
        std::string zoneId, subscriberId, agentId, category = "OTHER";
        std::string priority = "NORMAL", subject, description, assignedTo;
        int slaHours = 24;
    };

    struct TicketMessageDto {
        std::string ticketId, authorType = "STAFF", authorId, body;
    };

    struct WorkOrderDto {
        std::string zoneId, subscriberId, orderType = "INSTALL", assignedTo, scheduledAt;
        Json::Value checklist = Json::Value(Json::arrayValue);
        Json::Value materials = Json::Value(Json::arrayValue);
        double cost = 0;
    };

    struct ZoneCostDto {
        std::string zoneId, category = "BACKHAUL", vendor, reference, notes;
        double amount = 0;
        std::string periodStart, periodEnd;

        static ZoneCostDto fromJson(const Json::Value &json) {
            ZoneCostDto dto;
            dto.zoneId = json.get("zoneId", "").asString();
            dto.category = json.get("category", "BACKHAUL").asString();
            dto.vendor = json.get("vendor", "").asString();
            dto.reference = json.get("reference", "").asString();
            dto.notes = json.get("notes", "").asString();
            const auto amount = json.get("amount", Json::Value(0.0));
            if (amount.isString()) {
                try { dto.amount = std::stod(amount.asString()); } catch (...) { dto.amount = 0; }
            } else {
                dto.amount = amount.asDouble();
            }
            dto.periodStart = json.get("periodStart", "").asString();
            dto.periodEnd = json.get("periodEnd", "").asString();
            return dto;
        }
    };

    struct StaffDto {
        std::string fullName, msisdn, email, pin, role = "SUPPORT";
        std::vector<std::string> zoneIds;
    };

    struct StaffLoginDto {
        std::string msisdn, pin;
    };

} // namespace data_support::dto::magma_net

#endif //DATASUPPORTAPI_MAGMANETOPSDTO_H
