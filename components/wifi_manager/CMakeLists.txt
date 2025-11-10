idf_component_register(SRCS "wifi_manager.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_netif esp_wifi nvs_flash  esp_http_server)


target_add_binary_data(${COMPONENT_LIB} "page/home.html" TEXT)
target_add_binary_data(${COMPONENT_LIB} "page/confirmation.html" TEXT)

target_add_binary_data(${COMPONENT_LIB} "page/index.js" TEXT)
target_add_binary_data(${COMPONENT_LIB} "page/styles.css" TEXT)