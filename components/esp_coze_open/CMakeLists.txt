idf_component_register(
    SRCS "src/esp_coze_chat.c" "src/esp_coze_chat_config.c" "src/esp_coze_events.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_websocket_client esp_http_client json mbedtls esp_timer esp_common
)


