idf_component_register(
       SRCS
              "main.cpp"
              "Wireless/wifi_manager.c"
              "Wireless/http_server.c"
       PRIV_REQUIRES
              fatfs
              spi_flash
              esp_timer
              esp_wifi
              nvs_flash
              esp_http_server
              spiffs
              json
       INCLUDE_DIRS
              "."
              "Wireless"
)

spiffs_create_partition_image(spiffs_data "Wireless/spiffs" FLASH_IN_PROJECT)
