//
// Created by Emmanuel Addo-Odame on 23/07/2026.
//

#ifndef DATASUPPORTAPI_FLEXYWITHDRAWALTOBANKDTO_H
#define DATASUPPORTAPI_FLEXYWITHDRAWALTOBANKDTO_H

#include <string>
#include <json/json.h>

namespace data_support::dto::smart_suite {

    class FlexyWithdrawalToBankDto {

    public:

        FlexyWithdrawalToBankDto() = default;
        explicit FlexyWithdrawalToBankDto(const Json::Value& json) { fromJson(json); }

        void fromJson(const Json::Value& json);

        // Getters
        [[nodiscard]] const std::string& getAmount() const { return amount_; }
        [[nodiscard]] const std::string& getBankName() const { return bank_name_; }
        [[nodiscard]] const std::string& getBankBranch() const { return bank_branch_; }
        [[nodiscard]] const std::string& getBankBranchCode() const { return bank_branch_code_; }
        [[nodiscard]] const std::string& getBankAccountNumber() const { return bank_account_number_; }
        [[nodiscard]] const std::string& getBankAccountName() const { return bank_account_name_; }
        [[nodiscard]] const std::string& getPrimaryCallbackUrl() const { return primary_callback_url_; }
        [[nodiscard]] const std::string& getDescription() const { return description_; }
        [[nodiscard]] const std::string& getAccountType() const { return account_type_; }
        [[nodiscard]] const std::string& getSenderMsisdn() const { return sender_msisdn_; }
        [[nodiscard]] const std::string& getRecipientPhoneNo() const { return recipient_phone_number_; }
        [[nodiscard]] const std::string& getPin() const { return pin_; }
        [[nodiscard]] const std::string& getIntegrationChannel() const { return integration_channel_; }
        [[nodiscard]] const std::string& getIntegratingService() const { return integrating_service_; }
        [[nodiscard]] const std::string& getTransactionType() const { return transaction_type_; }

        // Setters
        void setAmount(const std::string& value) { amount_ = value; }
        void setBankName(const std::string& value) { bank_name_ = value; }
        void setBankBranch(const std::string& value) { bank_branch_ = value; }
        void setBankBranchCode(const std::string& value) { bank_branch_code_ = value; }
        void setBankAccountNumber(const std::string& value) { bank_account_number_ = value; }
        void setBankAccountName(const std::string& value) { bank_account_name_ = value; }
        void setPrimaryCallbackUrl(const std::string& value) { primary_callback_url_ = value; }
        void setDescription(const std::string& value) { description_ = value; }
        void setAccountType(const std::string& value) { account_type_ = value; }
        void setSenderMsisdn(const std::string& value) { sender_msisdn_ = value; }
        void setRecipientPhoneNo(const std::string& value) { recipient_phone_number_ = value; }
        void setPin(const std::string& value) { pin_ = value; }
        void setIntegrationChannel(const std::string& value) { integration_channel_ = value; }
        void setIntegratingService(const std::string& value) { integrating_service_ = value; }
        void setTransactionType(const std::string& value) { transaction_type_ = value; }

    private:
        std::string amount_;
        std::string bank_name_;
        std::string bank_branch_;
        std::string bank_branch_code_;
        std::string bank_account_number_;
        std::string bank_account_name_;
        std::string primary_callback_url_;
        std::string description_;
        std::string account_type_;
        std::string recipient_phone_number_;
        std::string sender_msisdn_;
        std::string pin_;
        std::string integration_channel_;
        std::string integrating_service_;
        std::string transaction_type_;
    };

    inline void FlexyWithdrawalToBankDto::fromJson(const Json::Value& json) {

        if (json.isMember("amount") && !json["amount"].isNull()) {
            setAmount(json["amount"].asString());
        }

        if (json.isMember("bankName") && !json["bankName"].isNull()) {
            setBankName(json["bankName"].asString());
        }

        if (json.isMember("bankBranch") && !json["bankBranch"].isNull()) {
            setBankBranch(json["bankBranch"].asString());
        }

        if (json.isMember("bankBranchCode") && !json["bankBranchCode"].isNull()) {
            setBankBranchCode(json["bankBranchCode"].asString());
        }

        if (json.isMember("bankAccountNumber") && !json["bankAccountNumber"].isNull()) {
            setBankAccountNumber(json["bankAccountNumber"].asString());
        }

        if (json.isMember("bankAccountName") && !json["bankAccountName"].isNull()) {
            setBankAccountName(json["bankAccountName"].asString());
        }

        if (json.isMember("primaryCallbackUrl") && !json["primaryCallbackUrl"].isNull()) {
            setPrimaryCallbackUrl(json["primaryCallbackUrl"].asString());
        }

        if (json.isMember("description") && !json["description"].isNull()) {
            setDescription(json["description"].asString());
        }

        if (json.isMember("accountType") && !json["accountType"].isNull()) {
            setAccountType(json["accountType"].asString());
        }

        if (json.isMember("senderMsisdn") && !json["senderMsisdn"].isNull()) {
            setSenderMsisdn(json["senderMsisdn"].asString());
        }

        if (json.isMember("recipientPhoneNumber") && !json["recipientPhoneNumber"].isNull()) {
            setRecipientPhoneNo(json["recipientPhoneNumber"].asString());
        }

        if (json.isMember("pin") && !json["pin"].isNull()) {
            setPin(json["pin"].asString());
        }

        if (json.isMember("integrationChannel") && !json["integrationChannel"].isNull()) {
            setIntegrationChannel(json["integrationChannel"].asString());
        }

        if (json.isMember("integratingService") && !json["integratingService"].isNull()) {
            setIntegratingService(json["integratingService"].asString());
        }

        if (json.isMember("transactionType") && !json["transactionType"].isNull()) {
            setTransactionType(json["transactionType"].asString());
        }

    }

}
#endif //DATASUPPORTAPI_FLEXYWITHDRAWALTOBANKDTO_H