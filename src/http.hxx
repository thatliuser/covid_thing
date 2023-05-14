#ifndef HTTP_HXX
#define HTTP_HXX

#include "shared.hxx"
#include "util.hxx"

#include <http.h>
#pragma comment(lib, "httpapi.lib")
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

namespace http {
    class Request {
    public:
        HTTP_REQUEST_ID id;
        std::string query;
        std::wstring headers;
        util::Slice<const wchar_t> verb;
        util::Slice<const char> body;

    public:
        Request(HTTP_REQUEST* request);
        Request() {}
            
        ~Request() {}
    };

    class Response {
    public:
        int status;
        util::Slice<const char> reason;
        std::string body;

    public:
        ~Response() {}

        std::string get_json_value(util::Slice<const char> key);
    };

    class Server {
    private:
        HANDLE queue;

    private:
        Server(Server&&) = delete;

        Server();
        ~Server();

    public:
        static inline Server& get() {
            static Server server;
            return server;
        }

        void add_url(PCWSTR url);

        Request get_request();
        bool send_response(const Request& request, const Response& response);
    };

    class Client {
    private:
        HINTERNET session;

    private:
        Client(Client&&) = delete;
        
        Client();
        ~Client();

    public:
        static inline Client& get() {
            static Client client;
            return client;
        }

        Response send_request(const char* host, const char* url, const Request& request);
    };
}


#endif