//
// MagmaNet — RADIUS CoA / Disconnect client (RFC 5176).
//

#include "RadiusClient.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <array>
#include <atomic>
#include <cstring>
#include <thread>
#include <vector>

#include <drogon/drogon.h>

namespace data_support::utils {

    namespace {

        constexpr uint8_t kCodeDisconnectRequest = 40;
        constexpr uint8_t kCodeCoARequest = 43;
        constexpr uint8_t kCodeAck = 41;

        constexpr uint8_t kAttrUserName = 1;
        constexpr uint8_t kAttrNasIpAddress = 4;
        constexpr uint8_t kAttrAcctSessionId = 44;
        constexpr uint8_t kAttrMessageAuthenticator = 80;
        constexpr uint8_t kAttrVendorSpecific = 26;

        constexpr uint32_t kVendorMikrotik = 14988;
        constexpr uint8_t kVendorTypeRateLimit = 8;

        std::atomic<uint8_t> g_identifier{1};

        std::array<unsigned char, 16> md5Digest(const unsigned char *data, size_t length) {
            std::array<unsigned char, 16> digest{};
            unsigned int outLength = 0;
            EVP_MD_CTX *ctx = EVP_MD_CTX_new();
            if (ctx == nullptr) {
                return digest;
            }
            EVP_DigestInit_ex(ctx, EVP_md5(), nullptr);
            EVP_DigestUpdate(ctx, data, length);
            EVP_DigestFinal_ex(ctx, digest.data(), &outLength);
            EVP_MD_CTX_free(ctx);
            return digest;
        }

        std::array<unsigned char, 16> hmacMd5(const std::string &secret,
                                              const unsigned char *data,
                                              size_t length) {
            std::array<unsigned char, 16> digest{};
            unsigned int outLength = 0;
            if (HMAC(EVP_md5(),
                     secret.data(), static_cast<int>(secret.size()),
                     data, length,
                     digest.data(), &outLength) == nullptr) {
                return digest;
            }
            return digest;
        }

        void appendAttribute(std::vector<unsigned char> &packet,
                             uint8_t type,
                             const unsigned char *value,
                             size_t length) {
            packet.push_back(type);
            packet.push_back(static_cast<unsigned char>(length + 2));
            packet.insert(packet.end(), value, value + length);
        }

        void appendString(std::vector<unsigned char> &packet,
                          uint8_t type,
                          const std::string &value) {
            appendAttribute(packet, type,
                            reinterpret_cast<const unsigned char *>(value.data()),
                            value.size());
        }

        void appendUint32(std::vector<unsigned char> &packet, uint8_t type, uint32_t value) {
            unsigned char bytes[4];
            bytes[0] = static_cast<unsigned char>((value >> 24) & 0xFF);
            bytes[1] = static_cast<unsigned char>((value >> 16) & 0xFF);
            bytes[2] = static_cast<unsigned char>((value >> 8) & 0xFF);
            bytes[3] = static_cast<unsigned char>(value & 0xFF);
            appendAttribute(packet, type, bytes, sizeof(bytes));
        }

        void appendMikrotikRateLimit(std::vector<unsigned char> &packet, const std::string &rate) {
            // Vendor-Specific: type 26, len, vendor-id(4), vendor-type(1), vendor-len(1), value
            std::vector<unsigned char> value;
            value.resize(4);
            value[0] = static_cast<unsigned char>((kVendorMikrotik >> 24) & 0xFF);
            value[1] = static_cast<unsigned char>((kVendorMikrotik >> 16) & 0xFF);
            value[2] = static_cast<unsigned char>((kVendorMikrotik >> 8) & 0xFF);
            value[3] = static_cast<unsigned char>(kVendorMikrotik & 0xFF);
            value.push_back(kVendorTypeRateLimit);
            value.push_back(static_cast<unsigned char>(rate.size() + 2));
            value.insert(value.end(), rate.begin(), rate.end());
            appendAttribute(packet, kAttrVendorSpecific, value.data(), value.size());
        }

        bool sendAndAwaitAck(const std::string &nasIp,
                             int nasPort,
                             const std::string &secret,
                             uint8_t code,
                             const std::string &acctSessionId,
                             const std::string &username,
                             const std::string &rateLimit,
                             int timeoutMs) {
            std::vector<unsigned char> packet;
            packet.reserve(256);

            const uint8_t identifier = g_identifier.fetch_add(1);

            packet.push_back(code);
            packet.push_back(identifier);
            packet.push_back(0); // length placeholder
            packet.push_back(0);
            for (int i = 0; i < 16; ++i) {
                packet.push_back(0); // request authenticator placeholder
            }

            appendString(packet, kAttrUserName, username);
            appendUint32(packet, kAttrNasIpAddress,
                         ntohl(inet_addr(nasIp.c_str())));
            appendString(packet, kAttrAcctSessionId, acctSessionId);
            if (!rateLimit.empty()) {
                appendMikrotikRateLimit(packet, rateLimit);
            }

            // Message-Authenticator placeholder (16 zero bytes)
            const size_t messageAuthenticatorOffset = packet.size() + 2;
            packet.push_back(kAttrMessageAuthenticator);
            packet.push_back(18);
            for (int i = 0; i < 16; ++i) {
                packet.push_back(0);
            }

            const uint16_t totalLength = static_cast<uint16_t>(packet.size());
            packet[2] = static_cast<unsigned char>((totalLength >> 8) & 0xFF);
            packet[3] = static_cast<unsigned char>(totalLength & 0xFF);

            // Request Authenticator = MD5(Code | ID | Length | 16 zeros | Attributes)
            std::vector<unsigned char> authenticatorInput;
            authenticatorInput.reserve(packet.size());
            authenticatorInput.insert(authenticatorInput.end(), packet.begin(), packet.begin() + 4);
            for (int i = 0; i < 16; ++i) {
                authenticatorInput.push_back(0);
            }
            authenticatorInput.insert(authenticatorInput.end(), packet.begin() + 20, packet.end());

            const auto requestAuthenticator =
                md5Digest(authenticatorInput.data(), authenticatorInput.size());
            std::memcpy(packet.data() + 4, requestAuthenticator.data(), 16);

            // Message-Authenticator = HMAC-MD5(secret, packet with MA zeroed)
            const auto messageAuthenticator =
                hmacMd5(secret, packet.data(), packet.size());
            std::memcpy(packet.data() + messageAuthenticatorOffset,
                        messageAuthenticator.data(), 16);

            const int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
            if (sock < 0) {
                return false;
            }

            timeval timeout{};
            timeout.tv_sec = timeoutMs / 1000;
            timeout.tv_usec = (timeoutMs % 1000) * 1000;
            ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

            sockaddr_in target{};
            target.sin_family = AF_INET;
            target.sin_port = htons(static_cast<uint16_t>(nasPort));
            if (::inet_pton(AF_INET, nasIp.c_str(), &target.sin_addr) != 1) {
                ::close(sock);
                return false;
            }

            if (::sendto(sock, packet.data(), packet.size(), 0,
                         reinterpret_cast<sockaddr *>(&target), sizeof(target)) < 0) {
                ::close(sock);
                return false;
            }

            unsigned char buffer[4096];
            const ssize_t received = ::recvfrom(sock, buffer, sizeof(buffer), 0, nullptr, nullptr);
            ::close(sock);

            if (received < 4) {
                return false;
            }
            if (buffer[1] != identifier) {
                return false;
            }
            return buffer[0] == kCodeAck;
        }

    } // namespace

    RadiusClient::RadiusClient(int timeoutMs) : timeoutMs_(timeoutMs) {}

    bool RadiusClient::disconnect(const std::string &nasIp,
                                  int nasPort,
                                  const std::string &secret,
                                  const std::string &acctSessionId,
                                  const std::string &username) {
        return sendAndAwaitAck(nasIp, nasPort, secret, kCodeDisconnectRequest,
                               acctSessionId, username, "", timeoutMs_);
    }

    bool RadiusClient::changeRateLimit(const std::string &nasIp,
                                       int nasPort,
                                       const std::string &secret,
                                       const std::string &acctSessionId,
                                       const std::string &username,
                                       const std::string &rateLimit) {
        return sendAndAwaitAck(nasIp, nasPort, secret, kCodeCoARequest,
                               acctSessionId, username, rateLimit, timeoutMs_);
    }

    void RadiusClient::fireAndForget(std::function<void()> work) {
        std::thread([work = std::move(work)]() {
            try {
                work();
            } catch (const std::exception &e) {
                LOG_ERROR << "[RadiusClient] async operation failed: " << e.what();
            }
        }).detach();
    }

} // namespace data_support::utils
