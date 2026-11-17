//
// MagmaNet — unit tests for the voucher/HMAC primitives.
//
// These deliberately avoid the database: they cover the pieces that are easy
// to break silently (code normalisation, hash stability, token expiry).
//

#include <drogon/drogon_test.h>

#include <string>

#include "utils/HmacUtils.h"

using data_support::utils::HmacUtils;

DROGON_TEST(MagmaVoucherCodeNormalisation)
{
    CHECK(HmacUtils::normaliseVoucherCode("k7qm-3xzp-9wtd") == "K7QM3XZP9WTD");
    CHECK(HmacUtils::normaliseVoucherCode(" K7QM 3XZP 9WTD ") == "K7QM3XZP9WTD");
    // Crockford aliases: I/L -> 1, O -> 0, U -> V.
    CHECK(HmacUtils::normaliseVoucherCode("ILOU") == "110V");
    CHECK(HmacUtils::normaliseVoucherCode("").empty());
}

DROGON_TEST(MagmaVoucherCodeShape)
{
    const std::string code = HmacUtils::generateVoucherCode(3, 4);
    CHECK(code.size() == 14);  // 3 groups of 4 plus 2 dashes
    CHECK(code[4] == '-');
    CHECK(code[9] == '-');

    const std::string normalised = HmacUtils::normaliseVoucherCode(code);
    CHECK(normalised.size() == 12);
    for (char c : normalised) {
        CHECK(std::string("0123456789ABCDEFGHJKMNPQRSTVWXYZ").find(c) != std::string::npos);
    }

    // Two generations must not collide in practice.
    CHECK(HmacUtils::generateVoucherCode() != HmacUtils::generateVoucherCode());
}

DROGON_TEST(MagmaVoucherHashIsStable)
{
    const std::string key = "test-pepper";
    const std::string a = HmacUtils::hmacSha256Hex(key, "K7QM3XZP9WTD");
    const std::string b = HmacUtils::hmacSha256Hex(key, "K7QM3XZP9WTD");
    const std::string c = HmacUtils::hmacSha256Hex("other-pepper", "K7QM3XZP9WTD");

    CHECK(a == b);
    CHECK(a != c);
    CHECK(a.size() == 64);
    CHECK(HmacUtils::hmacSha256Hex(key, "K7QM3XZP9WTE") != a);
}

DROGON_TEST(MagmaSecureEquals)
{
    CHECK(HmacUtils::secureEquals("abc123", "abc123"));
    CHECK(!HmacUtils::secureEquals("abc123", "abc124"));
    CHECK(!HmacUtils::secureEquals("abc", "abcd"));
    CHECK(HmacUtils::secureEquals("", ""));
}

DROGON_TEST(MagmaPortalTokenRoundTrip)
{
    const std::string key = "token-key";
    const std::string token = HmacUtils::signToken(key, "biz-1", "sub-1", "AGENT", 3600);

    const auto payload = HmacUtils::verifyToken(key, token);
    CHECK(payload.valid);
    CHECK(payload.businessId == "biz-1");
    CHECK(payload.subjectId == "sub-1");
    CHECK(payload.role == "AGENT");
    CHECK(payload.expiresAt > HmacUtils::nowEpochSeconds());

    const auto wrongKey = HmacUtils::verifyToken("another-key", token);
    CHECK(!wrongKey.valid);

    // Expired tokens must be rejected even with the right key.
    const std::string expired = HmacUtils::signToken(key, "biz-1", "sub-1", "AGENT", -10);
    CHECK(!HmacUtils::verifyToken(key, expired).valid);

    // Tampering with the payload invalidates the signature.
    std::string tampered = token;
    tampered[0] = tampered[0] == 'A' ? 'B' : 'A';
    CHECK(!HmacUtils::verifyToken(key, tampered).valid);
}

DROGON_TEST(MagmaHexRoundTrip)
{
    const std::string hex = HmacUtils::randomHex(8);
    CHECK(hex.size() == 16);
    const auto bytes = HmacUtils::fromHex(hex);
    CHECK(bytes.size() == 8);
    CHECK(HmacUtils::toHex(bytes) == hex);
    CHECK(HmacUtils::fromHex("not-hex").empty());
}

DROGON_TEST(MagmaBase32IsCrockford)
{
    const std::vector<unsigned char> bytes = {0x00, 0x00, 0x00, 0x00, 0x00};
    const std::string encoded = HmacUtils::base32Encode(bytes);
    CHECK(encoded.find('I') == std::string::npos);
    CHECK(encoded.find('L') == std::string::npos);
    CHECK(encoded.find('O') == std::string::npos);
    CHECK(encoded.find('U') == std::string::npos);
}
