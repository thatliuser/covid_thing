#include "email.hxx"
#include "http.hxx"

#include <cstdlib>
#include <ctime>
#include <wincrypt.h>

#pragma comment(lib, "crypt32.lib")

namespace {
    constexpr char API_HOST[] = "gmail.googleapis.com";

    std::string b64_encode(const std::string& input) {
        DWORD length;
        CryptBinaryToStringA(
            reinterpret_cast<const BYTE*>(input.c_str()), input.size(),
            CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &length
        );
        char* buf = new char[length];
        CryptBinaryToStringA(
            reinterpret_cast<const BYTE*>(input.c_str()), input.size(), 
            CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, buf, &length
        );

        std::string ret = { buf, length };
        delete[] buf;
        return ret;
    }

    std::string get_mime_date() {
        auto raw_time = std::time(nullptr);
        std::tm time;
        localtime_s(&time, &raw_time);

        char buf[500];
        std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S -0800", &time);

        return buf;
    }

    const char* get_suffix(int day_of_month) {
        static const char* suffixes[] = { "th", "st", "nd", "rd" };
        int index = day_of_month % 100;
        if ((index / 10) == 1) index = 0;
        index %= 10;
        if (index > 3) index = 0;
        return suffixes[index];
    }

    std::string get_body_date() {
        auto raw_time = std::time(nullptr);
        std::tm time;
        localtime_s(&time, &raw_time);

        char buf[500];
        std::strftime(buf, sizeof(buf), "%A, %B %d", &time);
        std::string ret = buf;
        ret += get_suffix(time.tm_mday);
        std::strftime(buf, sizeof(buf), ", %Y", &time);
        ret += buf;

        return ret;
    }

    std::string get_message_id(const std::string& token) {
        auto raw_time = std::time(nullptr);
        std::tm time;
        localtime_s(&time, &raw_time);

        char buf[500];
        std::strftime(buf, sizeof(buf), "%Y%m%d%H%M", &time);
        std::string ret = buf;
        ret += token + "@mail.gmail.com";
        return ret;
    }

    std::string get_mime(const std::string& name, const std::string& address, const std::string& token) {
        std::string mime = 
            "MIME-Version: 1.0\n"
            "Date: " + get_mime_date() +
            "\nSubject: COVID Daily Symptom Survey Results\n"
            "From: <" + address +
            ">\nTo: <" + address +
            ">\nContent-Type: text/html; charset=\"UTF-8\"\n"
            "Message-ID: <" + get_message_id(token) + ">"
            #include "strings/email_one.txt"
            + name +
            #include "strings/email_two.txt"
            + get_body_date() +
            #include "strings/email_three.txt"
            ;
        return mime;
    }

    std::string get_email_body(const std::string& name, const std::string& address, const std::string& token) {
        return "{ \"raw\": \"" + b64_encode(get_mime(name, address, token)) + "\" }";
    }
}

std::string email::get_address(const std::string& token) {
    std::string url = "/gmail/v1/users/me/profile?access_token=" + token;
    http::Request request;
    request.verb = L"GET";

    http::Response response = http::Client::get().send_request(API_HOST, url.c_str(), request);

    return response.get_json_value("emailAddress");
}

void email::send_payload(const std::string& name, const std::string& address, const std::string& token) {
    http::Request request;
    std::string body = get_email_body(name, address, token);

    std::string url = "/gmail/v1/users/me/messages/send?access_token=" + token;

    request.verb = L"POST";
    request.headers = 
        L"Content-Type: application/json\r\n"
        L"Authorization: Bearer " + util::widen(token.c_str());
    request.body = { body.c_str(), body.size() + 1 };

    http::Response response = http::Client::get().send_request(API_HOST, url.c_str(), request);
}