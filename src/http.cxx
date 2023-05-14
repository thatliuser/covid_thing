#include "http.hxx"

#include <type_traits>
#include <cstdlib>

using namespace http;

namespace {
    [[noreturn]]
    inline void exit_with_err(int err) {
        LPTSTR buf = nullptr;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
            nullptr, err, 0, reinterpret_cast<LPTSTR>(&buf), 0, nullptr
        );
        util::print("HTTP failed with code ", err, ", message: ", buf);
        pause_then_exit(err);
    }

    template <auto func>
    class server_check {
        static_assert(!std::is_same_v<decltype(func), decltype(func)>, "Func is not a function pointer!");
    };

    template <class Ret, class... Args, Ret(func)(Args...)>
    class server_check<func> {
    private:
        server_check(server_check<func>&&) = delete;
        server_check() = delete;

    public:
        inline server_check(Args... args) {
            Ret ret = func(args...);
            if (ret != NO_ERROR) {
                exit_with_err(ret);
            }
        }
    };

    template <auto func>
    class client_check {
        static_assert(!std::is_same_v<decltype(func), decltype(func)>, "Func is not a function pointer!");
    };

    template <class Ret, class... Args, Ret(func)(Args...)>
    class client_check<func> {
        Ret ret;

    public:
        inline client_check(Args... args) {
            this->ret = func(args...);
            if (this->ret == NULL) {
                util::println("HTTP failed with code ", GetLastError());
                pause_then_exit(GetLastError());
            }
        }

        inline operator Ret() {
            return this->ret;
        }
    };
}

// Request impl
Request::Request(HTTP_REQUEST* request) {
    this->id = request->RequestId;
    if (request->CookedUrl.QueryStringLength != 0) {
        this->query = util::narrow(request->CookedUrl.pQueryString);
    }
}

// Response impl
std::string Response::get_json_value(util::Slice<const char> key) {
    std::string raw_key = "\"";
    raw_key += key.pointer;
    raw_key += "\": \"";
    auto start = this->body.find(raw_key);
    if (start != this->body.npos) {
        start += raw_key.size();
        auto end = this->body.find('"', start);
        return { this->body.c_str() + start, this->body.c_str() + end };
    }
    return "";
}

// Server impl
Server::Server() {
    HTTPAPI_VERSION version = HTTPAPI_VERSION_1;
    server_check<HttpInitialize>(version, HTTP_INITIALIZE_SERVER, nullptr);
    server_check<HttpCreateHttpHandle>(&this->queue, 0);
}

Server::~Server() {
    server_check<HttpTerminate>(HTTP_INITIALIZE_SERVER, nullptr);
}

void Server::add_url(PCWSTR url) {
    server_check<HttpAddUrl>(this->queue, url, nullptr);
}

Request Server::get_request() {
    constexpr auto BUFFER_LENGTH = 4096;

    HTTP_REQUEST_ID id = HTTP_NULL_ID;
    char* buffer;
    HTTP_REQUEST* request;
    ULONG read = BUFFER_LENGTH;

    while (true) {
        buffer = new char[sizeof(HTTP_REQUEST) + read];
        request = reinterpret_cast<decltype(request)>(buffer);

        auto ret = HttpReceiveHttpRequest(
                this->queue, id, HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY,
                request, BUFFER_LENGTH, &read, nullptr
            );
        
        if (ret == NO_ERROR) {
            break;
        }
        if (ret == ERROR_MORE_DATA) {
            delete[] buffer;
            id = request->RequestId;
        }
        else {
            exit_with_err(ret);
        }
    }

    Request ret = { request };
    delete[] buffer;
    return ret;
}

bool Server::send_response(const Request& request, const Response& response) {
    HTTP_RESPONSE raw_response = { 0 };
    HTTP_DATA_CHUNK chunk;
    raw_response.ReasonLength = response.reason.length - 1;
    raw_response.pReason = response.reason.pointer;
    raw_response.StatusCode = response.status;

    // I sure love WinAPI!
    constexpr util::Slice<const char> type = "text/html";
    HTTP_KNOWN_HEADER& content_type = raw_response.Headers.KnownHeaders[HttpHeaderContentType];
    content_type.pRawValue = type.pointer;
    content_type.RawValueLength = type.length - 1;

    if (response.body.size() != 0) {
        raw_response.pEntityChunks = &chunk;
        ++raw_response.EntityChunkCount;

        chunk.DataChunkType = HttpDataChunkFromMemory;
        chunk.FromMemory.pBuffer = const_cast<char*>(response.body.c_str());
        chunk.FromMemory.BufferLength = response.body.size();
    }

    return (HttpSendHttpResponse(
        this->queue, request.id, 
        0, 
        &raw_response, nullptr, nullptr, 
        nullptr, 0, nullptr, nullptr
    ) == NO_ERROR);
}

// Client impl
Client::Client() {
    this->session = client_check<WinHttpOpen>(
        nullptr, WINHTTP_ACCESS_TYPE_NO_PROXY, 
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0
    );
}

Client::~Client() {
    client_check<WinHttpCloseHandle>(this->session);
}

Response Client::send_request(const char* host, const char* url, const Request& request) {
    std::wstring wide_host = util::widen(host);
    std::wstring wide_url = util::widen(url);
    HINTERNET connection = client_check<WinHttpConnect>(
        this->session, wide_host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0
    );
    HINTERNET raw_request = client_check<WinHttpOpenRequest>(
        connection, request.verb.pointer, wide_url.c_str(), nullptr, 
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE
    );

    size_t body_length = 
        (request.body.pointer != nullptr) ? 
            request.body.length - 1 : 0;

    client_check<WinHttpSendRequest>(
        raw_request, request.headers.c_str(), -1, 
        const_cast<char*>(request.body.pointer), body_length, 
        body_length, 0
    );

    client_check<WinHttpReceiveResponse>(raw_request, nullptr);
    DWORD size = 0;
    client_check<WinHttpQueryDataAvailable>(raw_request, &size);
    char* buf = new char[size + 1];
    buf[size] = 0;
    client_check<WinHttpReadData>(raw_request, buf, size, nullptr);

    http::Response response = {
        0,
        "",
        { buf, size }
    };

    delete[] buf;

    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(raw_request);

    return response;
}