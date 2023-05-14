#include "oauth.hxx"
#include "http.hxx"
#include "util.hxx"

#include <cstring>

namespace {
    constexpr char API_HOST[] = "oauth2.googleapis.com";
    
    std::string get_code() {
        constexpr char LINK[] = 
            #include "strings/oauth.txt"
            ;
        constexpr char SUCCESS[] = 
            #include "strings/success.txt"
            ;
        constexpr char FAILURE[] = 
            #include "strings/failure.txt"
            ;

        ShellExecute(nullptr, "open", LINK, nullptr, nullptr, SW_SHOW);
        util::println("Please permit the service to use your Gmail account");
        
        auto& server = http::Server::get();

        http::Request request = server.get_request();

        auto query = request.query;

        constexpr char CODE_KEY[] = "code=";
        
        auto start = query.find(CODE_KEY);
        if (start != query.npos) {
            start += sizeof(CODE_KEY) - 1;
            server.send_response(request, {
                200,
                "OK",
                SUCCESS
            });

            auto end = query.find('&', start);
            if (end == query.npos) --end;
            return { query.c_str() + start, query.c_str() + end };
        }

        server.send_response(request, {
            200,
            "OK",
            FAILURE
        });
        return "";
    }
}

std::string oauth::get_initial_token() {
    #if 1
    constexpr wchar_t URI[] = L"http://[::1]:80/";
    http::Server::get().add_url(URI);
    util::println("Listening for requests to URI ", util::narrow(URI));

    std::string code = get_code();
    #else
    std::string code = "";
    #endif
    std::string url =
        #include "strings/initial_token.txt"
        + code + '/';
    http::Request request;
    request.verb = L"POST";
    request.body = "";
    http::Response response = http::Client::get().send_request(API_HOST, url.c_str(), request);

    std::string refresh = response.get_json_value("refresh_token");
    if (refresh != "") {
        std::ofstream file { REFRESH_FILE };
        file << refresh;
    }

    return response.get_json_value("access_token");
}

std::string oauth::get_token_from_refresh(const std::string& refresh) {
    std::string url =
        #include "strings/token_from_refresh.txt"
        + refresh;

    http::Request request;
    request.verb = L"POST";
    request.body = "";
    http::Response response = http::Client::get().send_request(API_HOST, url.c_str(), request);
    return response.get_json_value("access_token");
}