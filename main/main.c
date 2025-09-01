/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-09 18:34:37
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-09-01 16:20:55
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
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "wifi_manager.h"
#include "audio_hal.h"
#include "coze_chat.h"          // Coze 聊天组件核心头文件

#include "Display_SPD2010_Official.h"
#include "LVGL_Driver.h"
#include "ui.h"
#include "lottie_manager.h"

extern float BAT_analogVolts;

static const char *TAG = "MAIN";

/**
 * @brief 打印内存使用情况
 */
static void print_memory_info(void)
{
    // 获取堆内存信息
    size_t free_heap = esp_get_free_heap_size();
    size_t min_free_heap = esp_get_minimum_free_heap_size();
    size_t total_heap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
    
    // 获取PSRAM内存信息（如果存在）
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t total_psram = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    
    // 获取内部RAM信息
    size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t total_internal = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    
    ESP_LOGI(TAG, "========== 内存使用情况 ==========");
    ESP_LOGI(TAG, "总堆内存: %u 字节 (%.2f KB)", total_heap, total_heap / 1024.0);
    ESP_LOGI(TAG, "可用堆内存: %u 字节 (%.2f KB)", free_heap, free_heap / 1024.0);
    ESP_LOGI(TAG, "已用堆内存: %u 字节 (%.2f KB)", total_heap - free_heap, (total_heap - free_heap) / 1024.0);
    ESP_LOGI(TAG, "最小可用堆内存: %u 字节 (%.2f KB)", min_free_heap, min_free_heap / 1024.0);
    ESP_LOGI(TAG, "堆内存使用率: %.1f%%", ((float)(total_heap - free_heap) / total_heap) * 100);
    
    ESP_LOGI(TAG, "内部RAM总量: %u 字节 (%.2f KB)", total_internal, total_internal / 1024.0);
    ESP_LOGI(TAG, "内部RAM可用: %u 字节 (%.2f KB)", free_internal, free_internal / 1024.0);
    ESP_LOGI(TAG, "内部RAM已用: %u 字节 (%.2f KB)", total_internal - free_internal, (total_internal - free_internal) / 1024.0);
    ESP_LOGI(TAG, "内部RAM使用率: %.1f%%", ((float)(total_internal - free_internal) / total_internal) * 100);
    
    if (total_psram > 0) {
        ESP_LOGI(TAG, "PSRAM总量: %u 字节 (%.2f KB)", total_psram, total_psram / 1024.0);
        ESP_LOGI(TAG, "PSRAM可用: %u 字节 (%.2f KB)", free_psram, free_psram / 1024.0);
        ESP_LOGI(TAG, "PSRAM已用: %u 字节 (%.2f KB)", total_psram - free_psram, (total_psram - free_psram) / 1024.0);
        ESP_LOGI(TAG, "PSRAM使用率: %.1f%%", ((float)(total_psram - free_psram) / total_psram) * 100);
    } else {
        ESP_LOGI(TAG, "PSRAM: 未配置或不可用");
    }
    
    // 获取任务信息
    UBaseType_t task_count = uxTaskGetNumberOfTasks();
    ESP_LOGI(TAG, "当前运行任务数: %u", task_count);
    ESP_LOGI(TAG, "================================");
}

// 静态任务栈和控制块 - 简单版本
#define LVGL_TASK_STACK_SIZE (1024*30/sizeof(StackType_t))  // 8KB栈
static EXT_RAM_BSS_ATTR StackType_t lvgl_task_stack[LVGL_TASK_STACK_SIZE];  // PSRAM栈
static StaticTask_t lvgl_task_buffer;  // 内部RAM控制块

// LVGL定时器处理任务
static void lvgl_timer_task(void *pvParameters)
{   
    I2C_Init();
    LCD_Init_Official();
    
    // 初始化LVGL驱动
    esp_err_t lvgl_ret = lvgl_driver_init();
    if (lvgl_ret != ESP_OK) {
        ESP_LOGE("MAIN", "Failed to initialize LVGL driver: %s", esp_err_to_name(lvgl_ret));
        return;
    }

    // ui_init();
    
    // 初始化Lottie管理器
    if (lottie_manager_init()) {
        // 播放动画：文件路径，宽度，高度
        bool play_success = lottie_manager_play("/spiffs/wifi_loading.json", 93, 85);
    } else {
        ESP_LOGE(TAG, "Lottie管理器初始化失败");
    }

    // 等待LVGL完全初始化
    vTaskDelay(pdMS_TO_TICKS(100));
    print_memory_info();
    while (1) {
        // 处理LVGL定时器
        lv_timer_handler();
        
        // 延时，控制LVGL定时器处理频率
        vTaskDelay(pdMS_TO_TICKS(100));
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
    print_memory_info();
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

    while (1)
    {
        // 主循环延时可以更长，因为LVGL定时器已由专门任务处理
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1秒
    }
}
