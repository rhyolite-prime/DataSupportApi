//
// MagmaNet — support tickets, work orders, staff and the outbox worker.
//

#ifndef DATASUPPORTAPI_MAGMAOPSSERVICE_H
#define DATASUPPORTAPI_MAGMAOPSSERVICE_H

#include <drogon/drogon.h>
#include <json/json.h>

#include "dto/BaseApiResponse.h"

namespace data_support::services {

    class MagmaRadiusService;
    class MagmaSmsService;

    class MagmaOpsService {
    public:
        MagmaOpsService(MagmaRadiusService &radius, MagmaSmsService &sms);

        drogon::Task<dto::BaseApiResponse> listTickets(const std::string &businessId,
                                                       const std::string &status,
                                                       int pageNo, int pageSize);
        drogon::Task<dto::BaseApiResponse> getTicket(const std::string &businessId,
                                                     const std::string &ticketId);
        drogon::Task<dto::BaseApiResponse> createTicket(const std::string &businessId,
                                                        const Json::Value &body);
        drogon::Task<dto::BaseApiResponse> addTicketMessage(const std::string &businessId,
                                                            const std::string &ticketId,
                                                            const Json::Value &body);
        drogon::Task<dto::BaseApiResponse> updateTicket(const std::string &businessId,
                                                        const std::string &ticketId,
                                                        const Json::Value &body);

        drogon::Task<dto::BaseApiResponse> listWorkOrders(const std::string &businessId,
                                                          const std::string &status,
                                                          int pageNo, int pageSize);
        drogon::Task<dto::BaseApiResponse> createWorkOrder(const std::string &businessId,
                                                           const Json::Value &body);
        drogon::Task<dto::BaseApiResponse> updateWorkOrder(const std::string &businessId,
                                                           const std::string &workOrderId,
                                                           const Json::Value &body);

        drogon::Task<dto::BaseApiResponse> listStaff(const std::string &businessId);
        drogon::Task<dto::BaseApiResponse> createStaff(const std::string &businessId,
                                                       const Json::Value &body);
        drogon::Task<dto::BaseApiResponse> staffLogin(const std::string &businessId,
                                                      const Json::Value &body);

        drogon::Task<dto::BaseApiResponse> drainOutbox(int limit);
        drogon::Task<dto::BaseApiResponse> retryOutbox(const std::string &outboxId);
        drogon::Task<dto::BaseApiResponse> listAuditLog(const std::string &businessId,
                                                        int pageNo, int pageSize);

    private:
        drogon::Task<bool> processOutboxEvent(const std::string &eventType,
                                              const std::string &aggregateId,
                                              const Json::Value &payload);
        MagmaRadiusService &radius_;
        MagmaSmsService &sms_;
    };

} // namespace data_support::services

#endif //DATASUPPORTAPI_MAGMAOPSSERVICE_H
