/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-09 18:34:37
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-31 23:37:49
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
#include "coze_chat_app.h"          // Coze 聊天组件核心头文件

#include "Display_SPD2010_Official.h"
#include "LVGL_Driver.h"
#include "ui.h"
#include "lottie_test.h"

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
    
    // 初始化并启动Lottie动画测试
    // lottie_test_init();
    
    ESP_LOGI("MAIN", "=== 开始 Lottie 动画初始化 ===");
    
    // 检查屏幕是否有效
    lv_obj_t * screen = lv_screen_active();
    if (screen == NULL) {
        ESP_LOGE("MAIN", "活动屏幕无效，无法创建 Lottie 对象");
        return;
    }
    ESP_LOGI("MAIN", "活动屏幕有效，准备创建 Lottie 对象");
    
    // 创建Lottie动画对象，父对象为当前活动屏幕
    lv_obj_t * lottie = lv_lottie_create(screen);
    if (lottie == NULL) {
        ESP_LOGE("MAIN", "创建 Lottie 对象失败");
        return;
    }
    ESP_LOGI("MAIN", "成功创建 Lottie 对象");

    // 设置Lottie动画的数据源为本地JSON文件
    // 修正文件路径 - 实际文件在 /spiffs/ 目录下
    const char* lottie_file_path = "/spiffs/lv_example_lottie_approve.json";
    ESP_LOGI("MAIN", "准备设置 Lottie 数据源: %s", lottie_file_path);
    
    // 检查文件是否存在的简单方法：尝试用 fopen 打开
    FILE* file = fopen(lottie_file_path, "r");
    if (file == NULL) {
        ESP_LOGE("MAIN", "无法打开 Lottie 文件: %s (错误: %s)", lottie_file_path, strerror(errno));
        ESP_LOGI("MAIN", "尝试其他路径...");
        
        // 尝试其他可能的路径
        const char* alt_paths[] = {
            "./lv_example_lottie_approve.json",
            "lv_example_lottie_approve.json",
            "/spiffs/lv_example_lottie_approve.json",
            "main/spiffs/lv_example_lottie_approve.json"
        };
        
        bool file_found = false;
        for (int i = 0; i < sizeof(alt_paths)/sizeof(alt_paths[0]); i++) {
            ESP_LOGI("MAIN", "尝试路径: %s", alt_paths[i]);
            file = fopen(alt_paths[i], "r");
            if (file != NULL) {
                ESP_LOGI("MAIN", "找到文件: %s", alt_paths[i]);
                lottie_file_path = alt_paths[i];
                file_found = true;
                fclose(file);
                break;
            } else {
                ESP_LOGW("MAIN", "路径无效: %s", alt_paths[i]);
            }
        }
        
        if (!file_found) {
            ESP_LOGE("MAIN", "所有路径都无法找到 Lottie 文件，跳过设置数据源");
            lottie_file_path = NULL;
        }
    } else {
        ESP_LOGI("MAIN", "成功找到 Lottie 文件: %s", lottie_file_path);
        fclose(file);
    }
    
    if (lottie_file_path != NULL) {
        ESP_LOGI("MAIN", "开始设置 Lottie 数据源...");
        lv_lottie_set_src_file(lottie, lottie_file_path);
        ESP_LOGI("MAIN", "Lottie 数据源设置完成");
    } else {
        ESP_LOGW("MAIN", "跳过 Lottie 数据源设置，仅测试对象创建");
    }
    
    // 设置缓冲区（在设置数据源之后）
    ESP_LOGI("MAIN", "开始设置 Lottie 渲染缓冲区...");
    
    #if LV_DRAW_BUF_ALIGN == 4 && LV_DRAW_BUF_STRIDE_ALIGN == 1
        // 如果没有特殊的对齐或步幅要求，直接声明一个缓冲区
        // 由于Lottie渲染为ARGB8888格式，每像素4字节，因此缓冲区大小为宽*高*4
        ESP_LOGI("MAIN", "使用静态缓冲区模式 (64x64, %d 字节)", 64 * 64 * 4);
        static uint8_t buf[64 * 64 * 4];
        // 设置Lottie控件的渲染缓冲区，宽高均为64像素
        lv_lottie_set_buffer(lottie, 64, 64, buf);
        ESP_LOGI("MAIN", "静态缓冲区设置完成");
    #else
        // 针对GPU或特殊对齐/步幅需求，使用LVGL的draw_buf定义宏
        // LV_DRAW_BUF_DEFINE会自动分配合适的缓冲区
        ESP_LOGI("MAIN", "使用 LVGL draw_buf 模式 (64x64, ARGB8888)");
        LV_DRAW_BUF_DEFINE(draw_buf, 64, 64, LV_COLOR_FORMAT_ARGB8888);
        // 设置Lottie控件的draw_buf
        lv_lottie_set_draw_buf(lottie, &draw_buf);
        ESP_LOGI("MAIN", "LVGL draw_buf 设置完成");
    #endif
    
    // 居中显示Lottie动画对象
    ESP_LOGI("MAIN", "设置 Lottie 对象居中显示");
    lv_obj_center(lottie);
    
    ESP_LOGI("MAIN", "=== Lottie 动画初始化完成 ===");

    // 主循环
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_timer_handler();
    }
}
