#ifndef EMAIL_HXX
#define EMAIL_HXX

#include "shared.hxx"

namespace email {
    std::string get_address(const std::string& token);
    void send_payload(const std::string& name, const std::string& address, const std::string& token);
}

#endif