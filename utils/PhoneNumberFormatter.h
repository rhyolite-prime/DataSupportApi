#pragma once

#include <string>

namespace data_support::utils {

    class PhoneNumberFormatter {
    public:
        static std::string formatPhoneNumber(const std::string& phoneNumber);
        static std::string formatPhoneNumberToLocal(const std::string& phoneNumber);
    };

}
