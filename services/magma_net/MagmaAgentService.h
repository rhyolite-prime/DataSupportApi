//
// MagmaNet — agents, stock, commission wallet and settlement.
//

#ifndef DATASUPPORTAPI_MAGMAAGENTSERVICE_H
#define DATASUPPORTAPI_MAGMAAGENTSERVICE_H

#include <drogon/drogon.h>
#include <json/json.h>

#include "dto/BaseApiResponse.h"
#include "dto/magma_net/AgentDto.h"

namespace data_support::services {

    class MagmaVoucherService;
    class MagmaPayService;
    class MagmaSmsService;

    class MagmaAgentService {
    public:
        MagmaAgentService(MagmaVoucherService &vouchers, MagmaPayService &payments,
                          MagmaSmsService &sms);

        drogon::Task<dto::BaseApiResponse> createAgent(const std::string &businessId,
                                                       const dto::magma_net::AgentDto &dto);
        drogon::Task<dto::BaseApiResponse> updateAgent(const std::string &businessId,
                                                       const std::string &agentId,
                                                       const dto::magma_net::AgentDto &dto);
        drogon::Task<dto::BaseApiResponse> listAgents(const std::string &businessId,
                                                      int pageNo, int pageSize,
                                                      const std::string &status);
        drogon::Task<dto::BaseApiResponse> agentLogin(const std::string &businessId,
                                                      const dto::magma_net::AgentAuthDto &dto);
        drogon::Task<dto::BaseApiResponse> changeAgentPin(const std::string &businessId,
                                                          const dto::magma_net::AgentAuthDto &dto,
                                                          const std::string &newPin);
        drogon::Task<dto::BaseApiResponse> requestAgentPinReset(
            const std::string &businessId, const std::string &agentIdentifier);
        drogon::Task<dto::BaseApiResponse> resetAgentPin(
            const std::string &businessId, const dto::magma_net::AgentPinResetDto &dto);
        drogon::Task<dto::BaseApiResponse> getAgentDashboard(const std::string &businessId,
                                                             const std::string &agentId);
        drogon::Task<dto::BaseApiResponse> getAgentTransactions(const std::string &businessId,
                                                                const std::string &agentId,
                                                                int pageNo, int pageSize);
        drogon::Task<dto::BaseApiResponse> getAgentStatement(const std::string &businessId,
                                                             const std::string &agentId,
                                                             const std::string &from,
                                                             const std::string &to);
        drogon::Task<dto::BaseApiResponse> purchaseBatch(
            const std::string &businessId, const dto::magma_net::AgentStockPurchaseDto &dto);
        drogon::Task<dto::BaseApiResponse> initializeSettlement(
            const std::string &businessId, const dto::magma_net::AgentSettlementDto &dto);
        drogon::Task<dto::BaseApiResponse> completeSettlement(
            const std::string &businessId, const std::string &reference);
        drogon::Task<dto::BaseApiResponse> completeSettlementByReference(
            const std::string &reference);
        drogon::Task<double> getBalance(const std::string &businessId,
                                        const std::string &agentId);

    private:
        drogon::Task<std::string> resolveAgentId(const std::string &businessId,
                                                 const std::string &identifier);
        MagmaVoucherService &vouchers_;
        MagmaPayService &payments_;
        MagmaSmsService &sms_;
    };

} // namespace data_support::services

#endif //DATASUPPORTAPI_MAGMAAGENTSERVICE_H
