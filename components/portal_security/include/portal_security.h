#pragma once

#include "esp_http_server.h"

namespace portal_security {
// Call only after the previous HTTP server has stopped and before registering routes.
void ResetRoutes();
esp_err_t RegisterRoute(httpd_handle_t server, const httpd_uri_t* route);
// Reject connections arriving through the station interface, including unknown URLs.
esp_err_t OpenSession(httpd_handle_t server, int socket_fd);
}
