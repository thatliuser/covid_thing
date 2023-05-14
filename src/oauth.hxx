#ifndef OAUTH_HXX
#define OAUTH_HXX

#include "shared.hxx"

namespace oauth {
    std::string get_initial_token();
    std::string get_token_from_refresh(const std::string& refresh);
}

#endif