#include "PhoneNumberFormatter.h"

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

}
