#include "shared.hxx"
#include "oauth.hxx"
#include "util.hxx"
#include "http.hxx"
#include "email.hxx"

#include <sstream>

int main() {
    std::ifstream refresh_file { REFRESH_FILE };
    std::ifstream name_file { "name.txt" };

    if (!name_file.is_open()) {
        util::println("Please provide a name.txt so the email is properly generated!");
        pause_then_exit(0);
    }
    std::stringstream name_buf;
    name_buf << name_file.rdbuf();
    std::string name = name_buf.str();
    
    std::string token = [&]() -> std::string {
        if (!refresh_file.is_open()) {
            util::println("No refresh token, asking for initial code");
            return oauth::get_initial_token();
        }
        else {
            util::println("Found refresh token, getting access");
            std::string refresh;
            refresh_file >> refresh;
            return oauth::get_token_from_refresh(refresh);
        }
    }();
    util::println("Got token: ", token);

    std::string address = email::get_address(token);
    util::println("Got email: ", address);

    email::send_payload(name, address, token);

    util::println("Success! Sent email.");
    pause_then_exit(0);
}