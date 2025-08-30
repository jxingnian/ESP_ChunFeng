/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-09 18:34:37
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-30 10:02:21
 * @FilePath: \esp-chunfeng\main\main.c
 * @Description: esp32春风-AI占卜助手
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "wifi_manager.h"
#include "audio_hal.h"
#include "coze_chat_app.h"          // Coze 聊天组件核心头文件

#include "I2C_Driver.h"
#include "Display_SPD2010.h"
#include "LVGL_Driver.h"
#include "ui.h"

extern float BAT_analogVolts;

void app_main()
{    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // 硬件初始化
    I2C_Init();
    LCD_Init();
    LVGL_Init();    // 先初始化LVGL库

    
    // 后台初始化其他组件（不影响动画播放）
    // wifi_init_softap();     //WIFI
    ui_init();
    
    // 主循环
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_timer_handler();
    }
}
