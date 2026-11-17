//
// MagmaNet — HMAC, token and one-time-code utilities.
//

#include "HmacUtils.h"

#include <openssl/hmac.h>
#include <openssl/rand.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <sstream>
#include <stdexcept>

namespace data_support::utils {

    namespace {
        constexpr char kCrockford[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
        constexpr char kBase64Url[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    }

    std::vector<unsigned char> HmacUtils::hmacSha256(const std::string &key, const std::string &data) {
        unsigned char digest[EVP_MAX_MD_SIZE];
        unsigned int length = 0;

        if (HMAC(EVP_sha256(),
                 key.data(), static_cast<int>(key.size()),
                 reinterpret_cast<const unsigned char *>(data.data()), data.size(),
                 digest, &length) == nullptr) {
            return {};
        }
        return std::vector<unsigned char>(digest, digest + length);
    }

    std::string HmacUtils::hmacSha256Hex(const std::string &key, const std::string &data) {
        return toHex(hmacSha256(key, data));
    }

    std::vector<unsigned char> HmacUtils::randomBytes(size_t length) {
        std::vector<unsigned char> buffer(length);
        if (length > 0 && RAND_bytes(buffer.data(), static_cast<int>(length)) != 1) {
            throw std::runtime_error("Unable to obtain secure random bytes");
        }
        return buffer;
    }

    std::string HmacUtils::randomHex(size_t bytes) {
        return toHex(randomBytes(bytes));
    }

    std::string HmacUtils::toHex(const std::vector<unsigned char> &data) {
        static constexpr char hex[] = "0123456789abcdef";
        std::string out;
        out.reserve(data.size() * 2);
        for (unsigned char c : data) {
            out.push_back(hex[(c >> 4) & 0x0F]);
            out.push_back(hex[c & 0x0F]);
        }
        return out;
    }

    std::vector<unsigned char> HmacUtils::fromHex(const std::string &hex) {
        std::vector<unsigned char> out;
        if (hex.size() % 2 != 0) {
            return out;
        }
        auto nibble = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        out.reserve(hex.size() / 2);
        for (size_t i = 0; i < hex.size(); i += 2) {
            int hi = nibble(hex[i]);
            int lo = nibble(hex[i + 1]);
            if (hi < 0 || lo < 0) {
                return {};
            }
            out.push_back(static_cast<unsigned char>((hi << 4) | lo));
        }
        return out;
    }

    std::string HmacUtils::base32Encode(const std::vector<unsigned char> &data) {
        std::string out;
        int buffer = 0;
        int bitsLeft = 0;

        for (unsigned char byte : data) {
            buffer = (buffer << 8) | byte;
            bitsLeft += 8;
            while (bitsLeft >= 5) {
                bitsLeft -= 5;
                out.push_back(kCrockford[(buffer >> bitsLeft) & 0x1F]);
            }
        }
        if (bitsLeft > 0) {
            out.push_back(kCrockford[(buffer << (5 - bitsLeft)) & 0x1F]);
        }
        return out;
    }

    std::string HmacUtils::generateVoucherCode(size_t groups, size_t groupSize) {
        const size_t totalChars = groups * groupSize;
        // 5 bits per character
        const size_t bytesNeeded = (totalChars * 5 + 7) / 8;
        const std::string encoded = base32Encode(randomBytes(bytesNeeded));

        std::string code;
        code.reserve(totalChars + groups);
        for (size_t i = 0; i < totalChars; ++i) {
            if (i > 0 && i % groupSize == 0) {
                code.push_back('-');
            }
            code.push_back(encoded[i % encoded.size()]);
        }
        return code;
    }

    std::string HmacUtils::normaliseVoucherCode(const std::string &code) {
        std::string out;
        for (char c : code) {
            if (c == '-' || c == ' ' || c == '\t') {
                continue;
            }
            out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        }
        // Crockford ambiguity: O -> 0, I/L -> 1
        for (char &c : out) {
            if (c == 'O') c = '0';
            if (c == 'I' || c == 'L') c = '1';
            if (c == 'U') c = 'V';
        }
        return out;
    }

    bool HmacUtils::secureEquals(const std::string &a, const std::string &b) {
        if (a.size() != b.size()) {
            return false;
        }
        unsigned char diff = 0;
        for (size_t i = 0; i < a.size(); ++i) {
            diff |= static_cast<unsigned char>(a[i] ^ b[i]);
        }
        return diff == 0;
    }

    std::string HmacUtils::base64UrlEncode(const std::string &raw) {
        std::string out;
        int value = 0;
        int bits = -6;
        for (unsigned char c : raw) {
            value = (value << 8) + c;
            bits += 8;
            while (bits >= 0) {
                out.push_back(kBase64Url[(value >> bits) & 0x3F]);
                bits -= 6;
            }
        }
        if (bits > -6) {
            out.push_back(kBase64Url[(value << (-bits)) & 0x3F]);
        }
        return out;
    }

    std::string HmacUtils::base64UrlDecode(const std::string &encoded) {
        std::vector<int> lookup(256, -1);
        for (int i = 0; i < 64; ++i) {
            lookup[static_cast<unsigned char>(kBase64Url[i])] = i;
        }
        std::string out;
        int value = 0;
        int bits = -8;
        for (unsigned char c : encoded) {
            if (lookup[c] == -1) {
                break;
            }
            value = (value << 6) + lookup[c];
            bits += 6;
            if (bits >= 0) {
                out.push_back(static_cast<char>((value >> bits) & 0xFF));
                bits -= 8;
            }
        }
        return out;
    }

    int64_t HmacUtils::nowEpochSeconds() {
        return std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    std::string HmacUtils::signToken(const std::string &key,
                                     const std::string &businessId,
                                     const std::string &subjectId,
                                     const std::string &role,
                                     int ttlSeconds) {
        const std::string payload = businessId + "|" + subjectId + "|" + role + "|" +
                                    std::to_string(nowEpochSeconds() + ttlSeconds);
        const std::string encoded = base64UrlEncode(payload);
        return encoded + "." + hmacSha256Hex(key, encoded);
    }

    HmacUtils::TokenPayload HmacUtils::verifyToken(const std::string &key, const std::string &token) {
        TokenPayload result;
        const auto dot = token.find('.');
        if (dot == std::string::npos) {
            return result;
        }
        const std::string encoded = token.substr(0, dot);
        const std::string signature = token.substr(dot + 1);
        const std::string expected = hmacSha256Hex(key, encoded);

        if (!secureEquals(expected, signature)) {
            return result;
        }

        const std::string payload = base64UrlDecode(encoded);
        std::vector<std::string> parts;
        std::stringstream ss(payload);
        std::string item;
        while (std::getline(ss, item, '|')) {
            parts.push_back(item);
        }
        if (parts.size() != 4) {
            return result;
        }
        try {
            const int64_t expiry = std::stoll(parts[3]);
            if (expiry < nowEpochSeconds()) {
                return result;
            }
            result.valid = true;
            result.businessId = parts[0];
            result.subjectId = parts[1];
            result.role = parts[2];
            result.expiresAt = expiry;
        } catch (const std::exception &) {
            return result;
        }
        return result;
    }

} // namespace data_support::utils
