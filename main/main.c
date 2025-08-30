/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-09 18:34:37
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-30 16:17:03
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

#include "Display_SPD2010_Official.h"
#include "LVGL_Driver.h"
#include "ui.h"

extern float BAT_analogVolts;

// LVGL demo任务
static void lvgl_demo_task(void *pvParameters)
{
    // 等待LVGL完全初始化
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 任务完成后删除自己
    vTaskDelete(NULL);
}

void app_main()
{    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // 硬件初始化（使用官方组件版本）
    I2C_Init();
    LCD_Init_Official();
    
    // 初始化LVGL驱动
    esp_err_t lvgl_ret = lvgl_driver_init();
    if (lvgl_ret != ESP_OK) {
        ESP_LOGE("MAIN", "Failed to initialize LVGL driver: %s", esp_err_to_name(lvgl_ret));
        return;
    }

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
