#pragma once

#include <string>

namespace data_support::utils {

/**
 * Utility for parsing URLs into host and path components.
 *
 * drogon::HttpClient::newHttpClient() must receive ONLY the scheme+host
 * (e.g. "https://example.com"), never a full URL with a path segment.
 * Passing a full URL causes drogon to emit a malformed Host header
 * (e.g. "Host: example.com/api/endpoint") which IIS rejects with
 * "400 Bad Request - Invalid Header".
 */
struct ParsedUrl {
    std::string host; // e.g. "https://win-fsrrc8uidt9.dasccu.org"
    std::string path; // e.g. "/api/ussd"
};

class UrlUtils {
public:
    /**
     * Splits a full URL into its host (scheme + authority) and path components.
     *
     * Examples:
     *   "https://example.com/api/ussd" -> { "https://example.com", "/api/ussd" }
     *   "https://example.com"          -> { "https://example.com", "/" }
     */
    static ParsedUrl parseUrl(const std::string& url);
};

} // namespace data_support::utils
