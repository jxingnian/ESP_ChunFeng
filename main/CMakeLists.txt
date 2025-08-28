idf_component_register(
       SRCS
              "main.c"
              "Wireless/wifi_manager.c"
              "Wireless/http_server.c"
              "Audio/audio_hal.c"
              "Audio/audio_player.c"
              "Audio/button_voice.c"
              "coze_chat/coze_chat_app.c"
              "LCD_Driver/Display_SPD2010.c"             
              "LCD_Driver/esp_lcd_spd2010/esp_lcd_spd2010.c"    
              "LVGL_Driver/LVGL_Driver.c"
              "Touch_Driver/Touch_SPD2010.c"
              "I2C_Driver/I2C_Driver.c"
              "EXIO/TCA9554PWR.c"
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
              esp_lcd
              lvgl_ui
       INCLUDE_DIRS
              "."
              "Wireless"
              "Audio"
              "coze_chat"              
              "LCD_Driver"             
              "LCD_Driver/esp_lcd_spd2010"         
              "LVGL_Driver"
              "Touch_Driver"
              "I2C_Driver"
              "EXIO"
       WHOLE_ARCHIVE TRUE
       EMBED_FILES nihao.pcm
)

spiffs_create_partition_image(spiffs_data "Wireless/spiffs" FLASH_IN_PROJECT)
