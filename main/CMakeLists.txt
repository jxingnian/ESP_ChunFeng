idf_component_register(
       SRCS
              "main.cpp"
              "Wireless/wifi_manager.c"
              "Wireless/http_server.c"
              # "MIC_Driver/MIC_Speech.c"
              # "Audio_Driver/PCM5101.c"
              "Audio/audio_hal.c"
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
       INCLUDE_DIRS
              "."
              "Wireless"
              "Audio"
)

spiffs_create_partition_image(spiffs_data "Wireless/spiffs" FLASH_IN_PROJECT)
