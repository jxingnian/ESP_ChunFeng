idf_component_register(
       SRCS
              "main.cpp"
              "Wireless/wifi_manager.c"
              "Wireless/http_server.c"
              "Audio/audio_hal.c"
              "coze_chat/coze_chat_app.c"
       PRIV_REQUIRES
              fatfs
              spi_flash
              esp_timer
              esp_wifi
              nvs_flash
              esp_http_server
              spiffs
              json
              driver
              esp_http_client
              esp_websocket_client
              esp_coze_open
       INCLUDE_DIRS
              "."
              "Wireless"
              "Audio"
              "coze_chat"                       
       WHOLE_ARCHIVE TRUE
       EMBED_FILES nihao.pcm
)

spiffs_create_partition_image(spiffs_data "Wireless/spiffs" FLASH_IN_PROJECT)
