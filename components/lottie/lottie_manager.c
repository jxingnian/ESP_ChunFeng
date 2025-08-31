/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-31 23:19:25
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-09-01 00:50:24
 * @FilePath: \ESP_ChunFeng\components\lottie\lottie_manager.c
 * @Description: Lottie动画管理器实现
 */

#include "lottie_manager.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>

static const char* TAG = "LOTTIE_MANAGER";

bool lottie_manager_init(void)
{
    
    ESP_LOGI("MAIN", "=== 开始 Lottie 动画初始化 ===");
    
    // 检查屏幕是否有效
    lv_obj_t * screen = lv_screen_active();
    if (screen == NULL) {
        ESP_LOGE("MAIN", "活动屏幕无效，无法创建 Lottie 对象");
        return false;
    }
    
    // 创建Lottie动画对象，父对象为当前活动屏幕
    lv_obj_t * lottie = lv_lottie_create(screen);
    if (lottie == NULL) {
        ESP_LOGE("MAIN", "创建 Lottie 对象失败");
        return false;
    }

    // 设置Lottie动画的数据源为本地JSON文件
    const char* lottie_file_path = "/spiffs/lv_example_lottie_approve.json";
    
    if (lottie_file_path != NULL) {
        lv_lottie_set_src_file(lottie, lottie_file_path);
    } else {
        ESP_LOGW("MAIN", "跳过 Lottie 数据源设置，仅测试对象创建");
    }
    
    // 设置缓冲区（在设置数据源之后）
    ESP_LOGI("MAIN", "开始设置 Lottie 渲染缓冲区...");

    uint8_t *buf = heap_caps_malloc(64 * 64 * 4, MALLOC_CAP_SPIRAM);
    if (buf == NULL) {
        ESP_LOGE("MAIN", "PSRAM缓冲区分配失败");
        return false;
    }
    
    // 设置Lottie控件的渲染缓冲区，宽高均为64像素
    lv_lottie_set_buffer(lottie, 64, 64, buf);
    ESP_LOGI("MAIN", "PSRAM缓冲区设置完成");
    
    // 居中显示Lottie动画对象
    ESP_LOGI("MAIN", "设置 Lottie 对象居中显示");
    lv_obj_center(lottie);
    
    ESP_LOGI("MAIN", "=== Lottie 动画初始化完成 ===");
    
    return true;
}
