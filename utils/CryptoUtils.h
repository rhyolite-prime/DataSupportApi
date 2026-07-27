//
// Created by Emmanuel Addo-Odame
//

#ifndef DATASUPPORTAPI_CRYPTOUTILS_H
#define DATASUPPORTAPI_CRYPTOUTILS_H

#include <string>
#include <vector>

namespace data_support::utils {

    class CryptoUtils {
    public:
        /**
         * Encrypts plaintext using AES-256-CBC.
         * Generates a random 16-byte IV, prepends it to the ciphertext,
         * and returns the combined string encoded in Base64.
         */
        static std::string encrypt(const std::string& plaintext, const std::string& key);

        /**
         * Decrypts a Base64-encoded AES-256-CBC ciphertext.
         * Extracts the 16-byte IV from the beginning before decrypting.
         */
        static std::string decrypt(const std::string& ciphertextBase64, const std::string& key);

    private:
        static std::string base64Encode(const unsigned char* buffer, size_t length);
        static std::vector<unsigned char> base64Decode(const std::string& input);
    };

}

#endif //DATASUPPORTAPI_CRYPTOUTILS_H
