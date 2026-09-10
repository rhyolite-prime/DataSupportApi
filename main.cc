#include <drogon/drogon.h>
int main() {

    printf("Starting DataSupportApi...\n");
    //Set HTTP listener address and port

    //Load config file
    drogon::app().loadConfigFile("config.json");
    //drogon::app().loadConfigFile("../config.json");

    drogon::app().registerPostHandlingAdvice(
      [](const drogon::HttpRequestPtr &req,
         const drogon::HttpResponsePtr &resp) {
        // Array of allowed origins
        const std::vector<std::string> allowedOrigins = {
            "http://localhost:3009",
            "https://pos.rhyoliteprime.com",
            "https://wssd-api.graphicnewsplus.com",
            "https://rhyoliteprime.com",
            "https://www.rhyoliteprime.com",
        };

        auto origin = req->getHeader("Origin");
        if (std::find(allowedOrigins.begin(), allowedOrigins.end(), origin) != allowedOrigins.end()) {
          resp->addHeader("Access-Control-Allow-Origin", origin);
          resp->addHeader("Access-Control-Allow-Credentials", "true");
        }

        // Add CORS headers to every response

        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, Referer, User-Agent, "
                        "Accept, X-Requested-With, Origin");
      });

    //Run HTTP framework,the method will block in the internal event loop
    drogon::app().run();
    return 0;
}
