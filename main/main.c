/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-09 18:34:37
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-09-01 01:13:17
 * @FilePath: \esp-chunfeng\main\main.c
 * @Description: esp32春风-AI占卜助手
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "wifi_manager.h"
#include "audio_hal.h"
#include "coze_chat.h"          // Coze 聊天组件核心头文件

#include "Display_SPD2010_Official.h"
#include "LVGL_Driver.h"
#include "ui.h"
#include "lottie_manager.h"

extern float BAT_analogVolts;

static const char *TAG = "MAIN";

// LVGL demo任务
static void lvgl_demo_task(void *pvParameters)
{
    // 等待LVGL完全初始化
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 任务完成后删除自己
    vTaskDelete(NULL);
}

/**
 * @brief 初始化SPIFFS文件系统
 *
 * @return esp_err_t ESP_OK表示成功，否则为失败
 */
static esp_err_t spiffs_filesystem_init(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs_data",
        .max_files = 5,
        .format_if_mount_failed = false
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
            return ESP_FAIL;
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
            return ESP_FAIL;
        }
    }

    return ESP_OK;
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
    spiffs_filesystem_init();

    // 后台初始化其他组件（不影响动画播放）
    wifi_init_softap();     //WIFI
    // ui_init();
    
    // 动画播放前内存状态
    ESP_LOGI(TAG, "=== 动画播放前内存状态 ===");
    ESP_LOGI(TAG, "内部RAM: %zu KB 可用 / %zu KB 总量", 
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024,
             heap_caps_get_total_size(MALLOC_CAP_INTERNAL) / 1024);
    ESP_LOGI(TAG, "PSRAM: %zu KB 可用 / %zu KB 总量", 
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024,
             heap_caps_get_total_size(MALLOC_CAP_SPIRAM) / 1024);
    ESP_LOGI(TAG, "最大可分配内部RAM块: %zu KB", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) / 1024);
    ESP_LOGI(TAG, "最大可分配PSRAM块: %zu KB", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) / 1024);

    // 初始化Lottie管理器
    if (lottie_manager_init()) {
        ESP_LOGI(TAG, "播放128x128动画 (需要64KB PSRAM缓冲区)");
        
        // 计算需要的内存
        size_t required_memory = 128 * 128 * 4; // ARGB8888
        ESP_LOGI(TAG, "动画缓冲区需要: %zu KB", required_memory / 1024);
        
        // 播放动画：文件路径，宽度，高度
        bool play_success = lottie_manager_play("/spiffs/WiFi_Connecting.json", 128, 128);
        
        // 动画播放后内存状态
        ESP_LOGI(TAG, "=== 动画播放后内存状态 ===");
        ESP_LOGI(TAG, "内部RAM: %zu KB 可用", heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024);
        ESP_LOGI(TAG, "PSRAM: %zu KB 可用", heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
        ESP_LOGI(TAG, "动画播放结果: %s", play_success ? "成功" : "失败");
        
        if (!play_success) {
            ESP_LOGE(TAG, "动画播放失败，可能是内存不足，尝试更小尺寸...");
            ESP_LOGI(TAG, "尝试播放64x64动画 (需要16KB PSRAM缓冲区)");
            lottie_manager_play("/spiffs/WiFi_Connecting.json", 64, 64);
        }
    } else {
        ESP_LOGE(TAG, "Lottie管理器初始化失败");
    }

    // 主循环
    static uint32_t memory_check_counter = 0;
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_timer_handler();
        
        // 每5秒打印一次内存状态
        memory_check_counter++;
        if (memory_check_counter >= 500) {  // 500 * 10ms = 5秒
            memory_check_counter = 0;
            ESP_LOGI(TAG, "=== 运行时内存状态 ===");
            ESP_LOGI(TAG, "内部RAM: %zu KB 可用", heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024);
            ESP_LOGI(TAG, "PSRAM: %zu KB 可用", heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
            ESP_LOGI(TAG, "最大可分配内部RAM块: %zu KB", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) / 1024);
        }
    }
}
