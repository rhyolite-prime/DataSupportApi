//
// Created by Emmanuel Addo-Odame
//

#include "CryptoUtils.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <stdexcept>
#include <cstring>

namespace data_support::utils {

    // Helper for Base64 encoding
    std::string CryptoUtils::base64Encode(const unsigned char* buffer, size_t length) {
        BIO *bio, *b64;
        BUF_MEM *bufferPtr;

        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        BIO_write(bio, buffer, length);
        BIO_flush(bio);
        BIO_get_mem_ptr(bio, &bufferPtr);

        std::string encoded(bufferPtr->data, bufferPtr->length);
        BIO_free_all(bio);
        return encoded;
    }

    // Helper for Base64 decoding
    std::vector<unsigned char> CryptoUtils::base64Decode(const std::string& input) {
        BIO *bio, *b64;
        std::vector<unsigned char> buffer(input.length());
        
        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new_mem_buf(input.c_str(), input.length());
        bio = BIO_push(b64, bio);
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        
        int decodedLength = BIO_read(bio, buffer.data(), input.length());
        if (decodedLength < 0) {
            decodedLength = 0;
        }
        buffer.resize(decodedLength);
        BIO_free_all(bio);
        return buffer;
    }

    std::string CryptoUtils::encrypt(const std::string& plaintext, const std::string& key) {
        if (key.length() < 32) {
            throw std::invalid_argument("Encryption key must be at least 32 bytes for AES-256.");
        }

        const int IV_LENGTH = 16;
        unsigned char iv[IV_LENGTH];
        if (!RAND_bytes(iv, IV_LENGTH)) {
            throw std::runtime_error("Failed to generate random IV.");
        }

        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            throw std::runtime_error("Failed to create EVP_CIPHER_CTX");
        }

        if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, reinterpret_cast<const unsigned char*>(key.data()), iv)) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to initialize encryption");
        }

        int len = 0;
        int ciphertext_len = 0;
        std::vector<unsigned char> ciphertext(plaintext.length() + EVP_MAX_BLOCK_LENGTH);

        if (1 != EVP_EncryptUpdate(ctx, ciphertext.data(), &len, reinterpret_cast<const unsigned char*>(plaintext.data()), plaintext.length())) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to encrypt update");
        }
        ciphertext_len = len;

        if (1 != EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len)) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to encrypt final");
        }
        ciphertext_len += len;
        EVP_CIPHER_CTX_free(ctx);

        std::vector<unsigned char> combined(IV_LENGTH + ciphertext_len);
        std::memcpy(combined.data(), iv, IV_LENGTH);
        std::memcpy(combined.data() + IV_LENGTH, ciphertext.data(), ciphertext_len);

        return base64Encode(combined.data(), combined.size());
    }

    std::string CryptoUtils::decrypt(const std::string& ciphertextBase64, const std::string& key) {
        if (key.length() < 32) {
            throw std::invalid_argument("Encryption key must be at least 32 bytes for AES-256.");
        }

        std::vector<unsigned char> combined = base64Decode(ciphertextBase64);
        const int IV_LENGTH = 16;
        if (combined.size() < IV_LENGTH) {
            throw std::runtime_error("Ciphertext is too short (missing IV).");
        }

        unsigned char iv[IV_LENGTH];
        std::memcpy(iv, combined.data(), IV_LENGTH);

        int ciphertext_len = combined.size() - IV_LENGTH;
        unsigned char* ciphertext = combined.data() + IV_LENGTH;

        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            throw std::runtime_error("Failed to create EVP_CIPHER_CTX");
        }

        if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, reinterpret_cast<const unsigned char*>(key.data()), iv)) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to initialize decryption");
        }

        int len = 0;
        int plaintext_len = 0;
        std::vector<unsigned char> plaintext(ciphertext_len);

        if (1 != EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext, ciphertext_len)) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to decrypt update");
        }
        plaintext_len = len;

        if (1 != EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len)) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to decrypt final");
        }
        plaintext_len += len;
        EVP_CIPHER_CTX_free(ctx);

        return std::string(reinterpret_cast<char*>(plaintext.data()), plaintext_len);
    }

}
