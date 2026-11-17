//
// MagmaNet — HMAC, token and one-time-code utilities.
//

#ifndef DATASUPPORTAPI_HMACUTILS_H
#define DATASUPPORTAPI_HMACUTILS_H

#include <string>
#include <vector>
#include <cstdint>

namespace data_support::utils {

    class HmacUtils {
    public:
        /** HMAC-SHA256 of `data` keyed with `key`, returned as lowercase hex. */
        static std::string hmacSha256Hex(const std::string &key, const std::string &data);

        /** HMAC-SHA256 raw bytes. */
        static std::vector<unsigned char> hmacSha256(const std::string &key, const std::string &data);

        /** Cryptographically secure random bytes. */
        static std::vector<unsigned char> randomBytes(size_t length);

        /** Random hex string of `bytes` random bytes (2*bytes chars). */
        static std::string randomHex(size_t bytes);

        /** Crockford base32 encoding (no I, L, O, U) — used for voucher codes. */
        static std::string base32Encode(const std::vector<unsigned char> &data);

        /** Human-friendly voucher code, e.g. "K7QM-3XZP-9WTD" (groups of 4). */
        static std::string generateVoucherCode(size_t groups = 3, size_t groupSize = 4);

        /** Normalise a user-entered voucher code (upper case, strip separators). */
        static std::string normaliseVoucherCode(const std::string &code);

        /** Constant-time comparison. */
        static bool secureEquals(const std::string &a, const std::string &b);

        /** Hex encode / decode. */
        static std::string toHex(const std::vector<unsigned char> &data);
        static std::vector<unsigned char> fromHex(const std::string &hex);

        /**
         * Stateless signed token: base64url(payload).hexHmac
         * payload = businessId|subjectId|role|expiryEpochSeconds
         */
        static std::string signToken(const std::string &key,
                                     const std::string &businessId,
                                     const std::string &subjectId,
                                     const std::string &role,
                                     int ttlSeconds);

        struct TokenPayload {
            bool valid = false;
            std::string businessId;
            std::string subjectId;
            std::string role;
            int64_t expiresAt = 0;
        };

        static TokenPayload verifyToken(const std::string &key, const std::string &token);

        static std::string base64UrlEncode(const std::string &raw);
        static std::string base64UrlDecode(const std::string &encoded);

        static int64_t nowEpochSeconds();
    };

} // namespace data_support::utils

#endif //DATASUPPORTAPI_HMACUTILS_H
