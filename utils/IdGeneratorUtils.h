//
// Created by Emmanuel Addo-Odame on 23/07/2026.
//

#ifndef DATASUPPORTAPI_IDGENERATORUTILS_H
#define DATASUPPORTAPI_IDGENERATORUTILS_H

#include <string>


namespace data_support::utils {

    class IdGeneratorUtils {
    public:
        /**
         * @brief Generates a random GUID in the format 8-4-4-4-12
         * @return A random GUID string
         */
        static std::string generateGuid();

        /**
         * @brief Generates a random 6-digit number as a string
         * @return A random 6-digit number string (100000-999999)
         */
        static std::string generateRandomSixDigit();

        /**
         * @brief Generates a random alphanumeric string of a given length
         * @param length The length of the string to generate
         * @return A random alphanumeric string
         */
        static std::string generateAlphanumericId(size_t length = 6);

        static std::string generateTimeStampReference();
    };

}

#endif //DATASUPPORTAPI_IDGENERATORUTILS_H