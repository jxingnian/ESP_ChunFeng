/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-09 18:34:37
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-09-11 14:11:41
 * @FilePath: \esp-chunfeng\main\main.c
 * @Description: esp32春风-AI占卜助手
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "wifi_manager.h"
#include "audio_hal.h"
#include "coze_chat.h"          // Coze 聊天组件核心头文件
#include "esp_coze_chat.h"      // Coze 聊天回调函数

#include "Display_SPD2010_Official.h"
#include "LVGL_Driver.h"
#include "ui.h"
#include "lottie_manager.h"
#include "lvgl.h"

extern float BAT_analogVolts;

static const char *TAG = "MAIN";

// 字幕缓冲区和定时器
#define SUBTITLE_BUFFER_SIZE 2048
#define SUBTITLE_UPDATE_INTERVAL_MS 200  // 200ms更新一次UI
static char subtitle_buffer[SUBTITLE_BUFFER_SIZE] = {0};
static size_t subtitle_buffer_len = 0;
static esp_timer_handle_t subtitle_timer = NULL;
static SemaphoreHandle_t subtitle_mutex = NULL;
static bool subtitle_update_pending = false;

/**
 * @brief 字幕定时器回调函数 - 批量更新UI
 */
static void subtitle_timer_callback(void* arg)
{
    if (!subtitle_update_pending) {
        return;
    }
    
    // 获取字幕缓冲区的内容
    if (xSemaphoreTake(subtitle_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (subtitle_buffer_len > 0 && g_subtitle_ta) {
            // 在LVGL线程中安全更新UI
            lv_lock();
            
            // 清空当前内容并设置新内容
            lv_textarea_set_text(g_subtitle_ta, subtitle_buffer);
            
            // 设置光标到最后位置并滚动到底部
            lv_textarea_set_cursor_pos(g_subtitle_ta, LV_TEXTAREA_CURSOR_LAST);
            lv_obj_scroll_to_y(g_subtitle_ta, LV_COORD_MAX, LV_ANIM_OFF);
            
            lv_unlock();
        }
        subtitle_update_pending = false;
        xSemaphoreGive(subtitle_mutex);
    }
}

/**
 * @brief 初始化字幕系统
 */
static esp_err_t init_subtitle_system(void)
{
    // 创建互斥锁
    subtitle_mutex = xSemaphoreCreateMutex();
    if (!subtitle_mutex) {
        ESP_LOGE(TAG, "创建字幕互斥锁失败");
        return ESP_ERR_NO_MEM;
    }
    
    // 创建定时器
    esp_timer_create_args_t timer_args = {
        .callback = subtitle_timer_callback,
        .arg = NULL,
        .name = "subtitle_timer"
    };
    
    esp_err_t ret = esp_timer_create(&timer_args, &subtitle_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "创建字幕定时器失败: %s", esp_err_to_name(ret));
        vSemaphoreDelete(subtitle_mutex);
        return ret;
    }
    
    // 启动定时器
    ret = esp_timer_start_periodic(subtitle_timer, SUBTITLE_UPDATE_INTERVAL_MS * 1000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "启动字幕定时器失败: %s", esp_err_to_name(ret));
        esp_timer_delete(subtitle_timer);
        vSemaphoreDelete(subtitle_mutex);
        return ret;
    }
    
    ESP_LOGI(TAG, "字幕系统初始化成功，更新间隔: %dms", SUBTITLE_UPDATE_INTERVAL_MS);
    return ESP_OK;
}

/**
 * @brief 销毁字幕系统
 */
static void deinit_subtitle_system(void)
{
    if (subtitle_timer) {
        esp_timer_stop(subtitle_timer);
        esp_timer_delete(subtitle_timer);
        subtitle_timer = NULL;
    }
    
    if (subtitle_mutex) {
        vSemaphoreDelete(subtitle_mutex);
        subtitle_mutex = NULL;
    }
    
    ESP_LOGI(TAG, "字幕系统已销毁");
}

/**
 * @brief 字幕文本处理回调函数（覆盖弱实现）
 * 
 * 这个函数会在收到 conversation.audio.sentence_start 事件时被调用
 * 高效实现：将字幕添加到缓冲区，由定时器批量更新UI
 * 
 * @param subtitle_text 字幕文本字符串
 * @param event_id 事件ID，可用于跟踪和去重
 */
void esp_coze_on_subtitle_text(const char *subtitle_text, const char *event_id)
{
    if (!subtitle_text || !event_id || !subtitle_mutex) {
        return;
    }
    
    ESP_LOGI(TAG, "🎬 收到字幕: \"%s\" (事件ID: %s)", subtitle_text, event_id);

    // 高效处理：添加到缓冲区，避免频繁UI操作
    if (xSemaphoreTake(subtitle_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        size_t text_len = strlen(subtitle_text);
        
        // 检查缓冲区空间
        if (subtitle_buffer_len + text_len + 1 < SUBTITLE_BUFFER_SIZE) {
            // 添加字幕文本和换行符
            strncat(subtitle_buffer, subtitle_text, SUBTITLE_BUFFER_SIZE - subtitle_buffer_len - 1);
            strcat(subtitle_buffer, "\n");
            subtitle_buffer_len = strlen(subtitle_buffer);
            
            // 标记需要更新UI
            subtitle_update_pending = true;
        } else {
            // 缓冲区满了，清空旧内容并添加新内容
            ESP_LOGW(TAG, "字幕缓冲区已满，清空旧内容");
            snprintf(subtitle_buffer, SUBTITLE_BUFFER_SIZE, "%s\n", subtitle_text);
            subtitle_buffer_len = strlen(subtitle_buffer);
            subtitle_update_pending = true;
        }
        
        xSemaphoreGive(subtitle_mutex);
    }
}

/**
 * @brief WiFi获得IP后的回调函数
 * @param ip_info IP信息结构体指针
 */
static void on_wifi_got_ip(esp_netif_ip_info_t *ip_info)
{
    ESP_LOGI(TAG, "WiFi连接成功，获得IP地址: " IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "网关: " IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "子网掩码: " IPSTR, IP2STR(&ip_info->netmask));
    
    // 在这里可以添加你需要的逻辑
    // 比如：停止WiFi加载动画，显示连接成功界面等
    
    // 初始化Coze聊天功能
    coze_chat_app_init();
    
    lottie_manager_stop_anim(LOTTIE_ANIM_WIFI_LOADING);
    // lottie_manager_stop_anim(LOTTIE_ANIM_THINK);

    // 初始化 UI 并创建字幕文本区域
    lv_lock();
    ui_init();
    if (g_subtitle_ta == NULL && ui_Subtitle != NULL) {
        g_subtitle_ta = lv_textarea_create(ui_Subtitle);
        lv_obj_set_size(g_subtitle_ta, lv_pct(100), lv_pct(100));
        lv_obj_set_align(g_subtitle_ta, LV_ALIGN_CENTER);
        lv_obj_set_scrollbar_mode(g_subtitle_ta, LV_SCROLLBAR_MODE_AUTO);
        lv_textarea_set_max_length(g_subtitle_ta, 4096);
        lv_textarea_set_one_line(g_subtitle_ta, false);
        lv_textarea_set_password_mode(g_subtitle_ta, false);
        lv_obj_set_style_text_font(g_subtitle_ta, &ui_font_pingfang26, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_textarea_set_text(g_subtitle_ta, "");
        // 默认显示最后一行
        lv_textarea_set_cursor_pos(g_subtitle_ta, LV_TEXTAREA_CURSOR_LAST);
    }
    lv_unlock();
}

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
#define LVGL_TASK_STACK_SIZE (1024*64/sizeof(StackType_t))  // 8KB栈
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
    
    // 初始化Lottie管理器
    if (lottie_manager_init()) {
        lottie_manager_play_anim(LOTTIE_ANIM_WIFI_LOADING);
        // lottie_manager_play_anim(LOTTIE_ANIM_THINK);
    } else {
        ESP_LOGE(TAG, "Lottie管理器初始化失败");
    }

    // 等待LVGL完全初始化
    vTaskDelay(pdMS_TO_TICKS(100));
    print_memory_info();
    while (1) {
        // LVGL定时器内部有加锁
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
    
    // 初始化字幕系统
    ret = init_subtitle_system();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "初始化字幕系统失败: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "字幕系统初始化成功");
    }
    
    // 注册WiFi IP获取回调函数
    esp_err_t callback_ret = wifi_register_got_ip_callback(on_wifi_got_ip);
    if (callback_ret != ESP_OK) {
        ESP_LOGE(TAG, "注册WiFi IP获取回调函数失败: %s", esp_err_to_name(callback_ret));
    } else {
        ESP_LOGI(TAG, "WiFi IP获取回调函数注册成功");
    }
    
    // 后台初始化其他组件（不影响动画播放）
    wifi_init_softap();     //WIFI

    while (1)
    {
        // 主循环延时可以更长，因为LVGL定时器已由专门任务处理
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1秒
    }
}
