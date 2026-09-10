#include "PhoneNumberFormatter.h"

#include <regex>

namespace data_support::utils {

    std::string PhoneNumberFormatter::formatPhoneNumber(const std::string& phoneNumber) {
        if (phoneNumber.empty())
            return phoneNumber;
    
        if (phoneNumber.starts_with("0")) {
            return "+233" + phoneNumber.substr(1);
        }

        if (phoneNumber.starts_with("233")) {
            return "+" + phoneNumber;
        }

        if (phoneNumber.starts_with("+233")) {
            return phoneNumber;
        }

        return phoneNumber;
    }

    std::string PhoneNumberFormatter::formatPhoneNumberToLocal(const std::string& phoneNumber) {
        if (phoneNumber.empty())
            return phoneNumber;
    
        if (phoneNumber.starts_with("0")) {
            return phoneNumber;
        }
    
        if (phoneNumber.starts_with("+233")) {
            return "0" + phoneNumber.substr(4);
        }
    
        if (phoneNumber.starts_with("233")) {
            return "0" + phoneNumber.substr(3);
        }
    
        return phoneNumber;
    }

    bool PhoneNumberFormatter::isValidPhoneNumber(const std::string& phone) {
        // Basic phone validation (can be customized based on country)
        const std::regex pattern(R"(^\+?[0-9]{10,15}$)");
        return std::regex_match(phone, pattern);
    }

    bool PhoneNumberFormatter::isValidPin(const std::string& pin) {
        // PIN should be exactly 4 digits
        const std::regex pattern(R"(^[0-9]{4}$)");
        return std::regex_match(pin, pattern);
    }

}
