/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-31 23:19:25
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-09-01 01:10:00
 * @FilePath: \ESP_ChunFeng\components\lottie\lottie_manager.c
 * @Description: 简单的Lottie动画管理器实现
 */

#include "lottie_manager.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>

static const char* TAG = "LOTTIE_MANAGER";

// 简单的全局变量管理
static lv_obj_t *g_lottie_obj = NULL;
static uint8_t *g_lottie_buffer = NULL;
static bool g_initialized = false;

bool lottie_manager_init(void)
{
    if (g_initialized) {
        ESP_LOGW(TAG, "已经初始化过了");
        return true;
    }
    
    ESP_LOGI(TAG, "初始化 Lottie 管理器");
    
    // 检查屏幕是否有效
    lv_obj_t *screen = lv_screen_active();
    if (screen == NULL) {
        ESP_LOGE(TAG, "无法获取活动屏幕");
        return false;
    }
    
    g_initialized = true;
    ESP_LOGI(TAG, "Lottie 管理器初始化完成");
    return true;
}

bool lottie_manager_play(const char *file_path, uint16_t width, uint16_t height)
{
    if (!g_initialized) {
        ESP_LOGE(TAG, "管理器未初始化");
        return false;
    }
    
    if (!file_path) {
        ESP_LOGE(TAG, "文件路径无效");
        return false;
    }
    
    ESP_LOGI(TAG, "播放动画: %s (%dx%d)", file_path, width, height);
    
    // 先清理之前的动画
    lottie_manager_stop();
    
    // 创建新的Lottie对象
    g_lottie_obj = lv_lottie_create(lv_screen_active());
    if (!g_lottie_obj) {
        ESP_LOGE(TAG, "创建 Lottie 对象失败");
        return false;
    }
    
    // 分配PSRAM缓冲区
    size_t buffer_size = width * height * 4; // ARGB8888
    g_lottie_buffer = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    if (!g_lottie_buffer) {
        ESP_LOGE(TAG, "PSRAM缓冲区分配失败 (需要 %zu 字节)", buffer_size);
        lv_obj_del(g_lottie_obj);
        g_lottie_obj = NULL;
        return false;
    }
    
    // 设置缓冲区
    lv_lottie_set_buffer(g_lottie_obj, width, height, g_lottie_buffer);
    
    // 设置数据源
    lv_lottie_set_src_file(g_lottie_obj, file_path);
    
    // 居中显示
    lv_obj_center(g_lottie_obj);
    
    ESP_LOGI(TAG, "动画播放成功");
    return true;
}

void lottie_manager_stop(void)
{
    if (g_lottie_obj) {
        ESP_LOGI(TAG, "停止动画");
        lv_obj_del(g_lottie_obj);
        g_lottie_obj = NULL;
    }
    
    if (g_lottie_buffer) {
        heap_caps_free(g_lottie_buffer);
        g_lottie_buffer = NULL;
    }
}

void lottie_manager_hide(void)
{
    if (g_lottie_obj) {
        lv_obj_add_flag(g_lottie_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

void lottie_manager_show(void)
{
    if (g_lottie_obj) {
        lv_obj_clear_flag(g_lottie_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

void lottie_manager_set_pos(int16_t x, int16_t y)
{
    if (g_lottie_obj) {
        lv_obj_set_pos(g_lottie_obj, x, y);
    }
}

void lottie_manager_center(void)
{
    if (g_lottie_obj) {
        lv_obj_center(g_lottie_obj);
    }
}