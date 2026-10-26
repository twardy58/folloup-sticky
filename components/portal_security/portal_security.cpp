#include "portal_security.h"
#include "portal_security_policy.h"

#include <array>
#include <cstring>
#include <string>
#include "esp_netif.h"
#include "lwip/sockets.h"
#include "mbedtls/base64.h"
#include "sdkconfig.h"

namespace portal_security {
namespace {
// Separate secrets: knowing the network password must not grant panel access.
constexpr char kPanelPassword[] = CONFIG_FOLLOWUP_PORTAL_PASSWORD;
static_assert(sizeof(kPanelPassword) >= 21 && sizeof(kPanelPassword) <= 64,
              "Generate a 20-63 character panel password with scripts/provision_setup.py");
std::array<httpd_uri_t, 24> s_routes{};
size_t s_route_count = 0;

std::string Header(httpd_req_t* request, const char* name)
{
    const size_t length = httpd_req_get_hdr_value_len(request, name);
    if (length == 0 || length > 256) return {};
    std::string value(length + 1, '\0');
    if (httpd_req_get_hdr_value_str(request, name, value.data(), value.size()) != ESP_OK)
        return {};
    value.resize(length);
    return value;
}

std::string ExpectedAuthorization()
{
    const std::string credential = std::string("admin:") + kPanelPassword;
    unsigned char encoded[128]{};
    size_t length = 0;
    if (mbedtls_base64_encode(encoded, sizeof(encoded), &length,
            reinterpret_cast<const unsigned char*>(credential.data()), credential.size()) != 0)
        return {};
    return "Basic " + std::string(reinterpret_cast<char*>(encoded), length);
}

esp_err_t GuardedHandler(httpd_req_t* request)
{
    const auto* original = static_cast<const httpd_uri_t*>(request->user_ctx);
    const auto host = Header(request, "Host");
    const size_t origin_length = httpd_req_get_hdr_value_len(request, "Origin");
    const auto origin = Header(request, "Origin");
    const size_t fetch_site_length = httpd_req_get_hdr_value_len(request, "Sec-Fetch-Site");
    const auto fetch_site = Header(request, "Sec-Fetch-Site");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    httpd_resp_set_hdr(request, "X-Content-Type-Options", "nosniff");
    httpd_resp_set_hdr(request, "Content-Security-Policy", "frame-ancestors 'none'");
    // Fail closed for oversized/malformed Origin; absence permits direct navigation.
    if (!AllowedHost(host) || !AllowedOrigin(origin) ||
        (origin_length != 0 && origin.empty()) ||
        (fetch_site_length != 0 && fetch_site.empty()) || fetch_site == "cross-site") {
        return httpd_resp_send_err(request, HTTPD_403_FORBIDDEN, "Forbidden");
    }
    const std::string expected = ExpectedAuthorization();
    if (expected.empty() || !EqualCredential(Header(request, "Authorization"), expected)) {
        httpd_resp_set_status(request, "401 Unauthorized");
        httpd_resp_set_hdr(request, "WWW-Authenticate", "Basic realm=\"Followup setup\"");
        return httpd_resp_send(request, "Authentication required", HTTPD_RESP_USE_STRLEN);
    }
    request->user_ctx = original->user_ctx;
    return original->handler(request);
}
}

void ResetRoutes() { s_route_count = 0; }

esp_err_t RegisterRoute(httpd_handle_t server, const httpd_uri_t* route)
{
    if (!server || !route || !route->handler) return ESP_ERR_INVALID_ARG;
    if (s_route_count == s_routes.size()) return ESP_ERR_NO_MEM;
    auto& original = s_routes[s_route_count];
    original = *route;
    httpd_uri_t guarded = *route;
    guarded.handler = GuardedHandler;
    guarded.user_ctx = &original;
    const esp_err_t result = httpd_register_uri_handler(server, &guarded);
    if (result == ESP_OK) ++s_route_count;
    return result;
}

esp_err_t OpenSession(httpd_handle_t, int socket_fd)
{
    sockaddr_storage local{};
    socklen_t length = sizeof(local);
    esp_netif_ip_info_t ap{};
    auto* netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (!netif || esp_netif_get_ip_info(netif, &ap) != ESP_OK ||
        ap.ip.addr == 0 ||
        getsockname(socket_fd, reinterpret_cast<sockaddr*>(&local), &length) != 0)
        return ESP_FAIL;

    uint32_t local_ipv4 = 0;
    if (local.ss_family == AF_INET && length >= sizeof(sockaddr_in)) {
        local_ipv4 = reinterpret_cast<const sockaddr_in*>(&local)->sin_addr.s_addr;
    }
#if CONFIG_LWIP_IPV6
    else if (local.ss_family == AF_INET6 && length >= sizeof(sockaddr_in6)) {
        const auto* addr = reinterpret_cast<const sockaddr_in6*>(&local);
        const auto* bytes =
            reinterpret_cast<const unsigned char*>(&addr->sin6_addr);
        static constexpr unsigned char prefix[12] =
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255};
        if (std::memcmp(bytes, prefix, sizeof(prefix)) != 0)
            return ESP_FAIL;
        std::memcpy(&local_ipv4, bytes + 12, sizeof(local_ipv4));
    }
#endif
    else {
        return ESP_FAIL;
    }
    return local_ipv4 == ap.ip.addr ? ESP_OK : ESP_FAIL;
}
}
