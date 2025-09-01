/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-09 18:34:37
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-09-01 12:03:24
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

// 静态任务栈和控制块 - 简单版本
#define LVGL_TASK_STACK_SIZE (1024*8/sizeof(StackType_t))  // 8KB栈
static EXT_RAM_BSS_ATTR StackType_t lvgl_task_stack[LVGL_TASK_STACK_SIZE];  // PSRAM栈
static StaticTask_t lvgl_task_buffer;  // 内部RAM控制块

// LVGL定时器处理任务
static void lvgl_timer_task(void *pvParameters)
{   
    ESP_LOGI(TAG, "LVGL定时器任务启动");
    
    // 检查栈是否在PSRAM中
    void *stack_ptr = &pvParameters;
    if (esp_ptr_external_ram(stack_ptr)) {
        ESP_LOGI(TAG, "LVGL任务栈在PSRAM中 ✅");
    } else {
        ESP_LOGI(TAG, "LVGL任务栈在内部RAM中");
    }
    ESP_LOGI(TAG, "LVGL任务栈地址: %p", stack_ptr);
    
    // 等待LVGL完全初始化
    vTaskDelay(pdMS_TO_TICKS(100));
    
    while (1) {
        // 处理LVGL定时器
        lv_timer_handler();
        
        // 延时，控制LVGL定时器处理频率
        vTaskDelay(pdMS_TO_TICKS(40));
    }
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
    
    // 创建LVGL定时器处理任务 - 简单静态任务
    TaskHandle_t task_handle = xTaskCreateStatic(
        lvgl_timer_task,           // 任务函数
        "lvgl_timer",              // 任务名称
        LVGL_TASK_STACK_SIZE,      // 栈大小
        NULL,                      // 任务参数
        5,                         // 任务优先级
        lvgl_task_stack,           // 栈数组(PSRAM)
        &lvgl_task_buffer          // 任务控制块(内部RAM)
    );
    
    if (task_handle == NULL) {
        ESP_LOGE(TAG, "创建LVGL定时器任务失败");
        return;
    }
    ESP_LOGI(TAG, "LVGL定时器任务创建成功");
    
    // 后台初始化其他组件（不影响动画播放）
    wifi_init_softap();     //WIFI
    // ui_init();
    
    // 初始化Lottie管理器
    if (lottie_manager_init()) {
        
        // 计算需要的内存
        size_t required_memory = 128 * 128 * 4; // ARGB8888
        
        // 播放动画：文件路径，宽度，高度
        bool play_success = lottie_manager_play("/spiffs/WiFi_Connecting.json", 64, 64);
        
        if (!play_success) {
            lottie_manager_play("/spiffs/WiFi_Connecting.json", 64, 64);
        }
    } else {
        ESP_LOGE(TAG, "Lottie管理器初始化失败");
    }

    // 主循环 - 现在主要用于系统监控和其他后台任务
    static uint32_t memory_check_counter = 0;
    while (1)
    {
        // 主循环延时可以更长，因为LVGL定时器已由专门任务处理
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1秒
    }
}
