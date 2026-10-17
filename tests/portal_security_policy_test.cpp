#include "portal_security_policy.h"
#include <cassert>

int main()
{
    using namespace portal_security;
    assert(AllowedHost("192.168.4.1"));
    assert(AllowedHost("192.168.4.1:80"));
    for (auto host : {"", "attacker.example", "192.168.4.1.attacker.example", "192.168.1.2"})
        assert(!AllowedHost(host));
    assert(AllowedOrigin(""));
    assert(AllowedOrigin("http://192.168.4.1"));
    assert(AllowedOrigin("http://192.168.4.1:80"));
    for (auto origin : {"null", "https://attacker.example", "http://192.168.4.1.attacker.example"})
        assert(!AllowedOrigin(origin));
    assert(EqualCredential("Basic correct", "Basic correct"));
    assert(!EqualCredential("Basic correcT", "Basic correct"));
    assert(!EqualCredential("", "Basic correct"));
}
