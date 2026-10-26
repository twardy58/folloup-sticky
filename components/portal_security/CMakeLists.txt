idf_component_register(SRCS "portal_security.cpp" INCLUDE_DIRS "include"
    REQUIRES esp_http_server esp_netif lwip mbedtls)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
