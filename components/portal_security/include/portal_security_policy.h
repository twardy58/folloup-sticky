#pragma once

#include <string_view>

namespace portal_security {
inline bool AllowedHost(std::string_view host)
{
    return host == "192.168.4.1" || host == "192.168.4.1:80";
}

inline bool AllowedOrigin(std::string_view origin)
{
    return origin.empty() || origin == "http://192.168.4.1" ||
           origin == "http://192.168.4.1:80";
}

inline bool EqualCredential(std::string_view provided, std::string_view expected)
{
    if (provided.size() != expected.size()) return false;
    unsigned char difference = 0;
    for (size_t i = 0; i < expected.size(); ++i) {
        difference |= static_cast<unsigned char>(provided[i] ^ expected[i]);
    }
    return difference == 0;
}
}
