#include "utils/UrlUtils.h"

namespace data_support::utils {

ParsedUrl UrlUtils::parseUrl(const std::string& url) {
    ParsedUrl result;
    result.host = url;
    result.path = "/";

    auto schemeEnd = url.find("://");
    if (schemeEnd != std::string::npos) {
        auto pathStart = url.find('/', schemeEnd + 3);
        if (pathStart != std::string::npos) {
            result.host = url.substr(0, pathStart); // e.g. "https://win-fsrrc8uidt9.dasccu.org"
            result.path = url.substr(pathStart);    // e.g. "/api/ussd"
        }
    }

    return result;
}

} // namespace data_support::utils
