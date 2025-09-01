/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-31 23:19:25
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-09-01 19:08:06
 * @FilePath: \esp-chunfeng\components\lottie\lottie_manager.c
 * @Description: 简单的Lottie动画管理器实现
 */

#include "lottie_manager.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

static const char* TAG = "LOTTIE_MANAGER";

// 动画命令类型
typedef enum {
    LOTTIE_CMD_PLAY,
    LOTTIE_CMD_STOP,
    LOTTIE_CMD_HIDE,
    LOTTIE_CMD_SHOW,
    LOTTIE_CMD_SET_POS,
    LOTTIE_CMD_CENTER
} lottie_cmd_type_t;

// 动画命令结构
typedef struct {
    lottie_cmd_type_t type;
    union {
        struct {
            int anim_type;
        } play;
        struct {
            int anim_type;
        } stop;
        struct {
            int16_t x;
            int16_t y;
        } pos;
    } data;
} lottie_cmd_t;

// 动画配置结构
typedef struct {
    const char *file_path;
    uint16_t width;
    uint16_t height;
} lottie_anim_config_t;

// 动画配置表
static const lottie_anim_config_t anim_configs[] = {
    [LOTTIE_ANIM_WIFI_LOADING] = {"/spiffs/wifi_loading.json", 93, 85},
    [LOTTIE_ANIM_MIC] = {"/spiffs/mic.json", 80, 80},
    [LOTTIE_ANIM_SPEAK] = {"/spiffs/speak.json", 80, 80},
    // 可以继续添加更多动画配置...
};

#define ANIM_CONFIG_COUNT (sizeof(anim_configs) / sizeof(anim_configs[0]))

// 静态任务相关 - 参考main.c的实现
#define LOTTIE_TASK_STACK_SIZE (1024*30/sizeof(StackType_t))  // 8KB栈
static EXT_RAM_BSS_ATTR StackType_t lottie_task_stack[LOTTIE_TASK_STACK_SIZE];  // PSRAM栈
static StaticTask_t lottie_task_buffer;  // 内部RAM控制块

// 全局变量管理
static lv_obj_t *g_lottie_obj = NULL;
static uint8_t *g_lottie_buffer = NULL;
static bool g_initialized = false;
static int g_current_anim_type = -1;  // 当前播放的动画类型
static QueueHandle_t g_cmd_queue = NULL;
static TaskHandle_t g_anim_task = NULL;

// 实际执行动画播放的内部函数
static bool _lottie_play_internal(int anim_type)
{
    if (anim_type < 0 || anim_type >= ANIM_CONFIG_COUNT) {
        ESP_LOGE(TAG, "无效的动画类型: %d", anim_type);
        return false;
    }
    
    const lottie_anim_config_t *config = &anim_configs[anim_type];
    if (!config->file_path) {
        ESP_LOGE(TAG, "动画类型 %d 未配置", anim_type);
        return false;
    }
    
    ESP_LOGI(TAG, "播放动画类型: %d", anim_type);
    
    // 直接使用现有的lottie_manager_play函数
    bool result = lottie_manager_play(config->file_path, config->width, config->height);
    if (result) {
        g_current_anim_type = anim_type;
    }
    
    return result;
}

// 停止动画的内部函数
static void _lottie_stop_internal(int anim_type)
{
    // -1 表示停止所有动画，或者检查是否是当前播放的动画类型
    if (anim_type == -1 || anim_type == g_current_anim_type) {
        ESP_LOGI(TAG, "停止动画类型: %d (当前: %d)", anim_type, g_current_anim_type);
        
        // 直接使用现有的lottie_manager_stop函数
        lottie_manager_stop();
        g_current_anim_type = -1;
    } else {
        ESP_LOGW(TAG, "动画类型 %d 未在播放中，当前播放: %d", anim_type, g_current_anim_type);
    }
}

// 动画处理静态任务 - 参考main.c的lvgl_timer_task
static void lottie_task(void *pvParameters)
{
    lottie_cmd_t cmd;
    
    ESP_LOGI(TAG, "动画处理任务启动");
    
    while (1) {
        if (xQueueReceive(g_cmd_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            switch (cmd.type) {
                case LOTTIE_CMD_PLAY:
                    _lottie_play_internal(cmd.data.play.anim_type);
                    break;
                    
                case LOTTIE_CMD_STOP:
                    _lottie_stop_internal(cmd.data.stop.anim_type);
                    break;
                    
                case LOTTIE_CMD_HIDE:
                    if (g_lottie_obj) {
                        lv_obj_add_flag(g_lottie_obj, LV_OBJ_FLAG_HIDDEN);
                    }
                    break;
                    
                case LOTTIE_CMD_SHOW:
                    if (g_lottie_obj) {
                        lv_obj_clear_flag(g_lottie_obj, LV_OBJ_FLAG_HIDDEN);
                    }
                    break;
                    
                case LOTTIE_CMD_SET_POS:
                    if (g_lottie_obj) {
                        lv_obj_set_pos(g_lottie_obj, cmd.data.pos.x, cmd.data.pos.y);
                    }
                    break;
                    
                case LOTTIE_CMD_CENTER:
                    if (g_lottie_obj) {
                        lv_obj_center(g_lottie_obj);
                    }
                    break;
                    
                default:
                    ESP_LOGW(TAG, "未知命令类型: %d", cmd.type);
                    break;
            }
        }
    }
}

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
    
    // 创建命令队列
    g_cmd_queue = xQueueCreate(10, sizeof(lottie_cmd_t));
    if (!g_cmd_queue) {
        ESP_LOGE(TAG, "创建命令队列失败");
        return false;
    }
    
    // 创建动画处理静态任务 - 参考main.c的实现
    g_anim_task = xTaskCreateStatic(
        lottie_task,                // 任务函数
        "lottie_task",              // 任务名称
        LOTTIE_TASK_STACK_SIZE,     // 栈大小
        NULL,                       // 任务参数
        4,                          // 优先级（比button_task低，比lvgl_timer低）
        lottie_task_stack,          // 栈数组(PSRAM)
        &lottie_task_buffer         // 任务控制块(内部RAM)
    );
    
    if (g_anim_task == NULL) {
        ESP_LOGE(TAG, "创建动画处理任务失败");
        vQueueDelete(g_cmd_queue);
        g_cmd_queue = NULL;
        return false;
    }
    
    g_initialized = true;
    ESP_LOGI(TAG, "Lottie 管理器初始化完成，动画任务已启动");
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

bool lottie_manager_play_anim(int anim_type)
{
    if (!g_initialized || !g_cmd_queue) {
        ESP_LOGE(TAG, "管理器未初始化");
        return false;
    }
    
    if (anim_type < 0 || anim_type >= ANIM_CONFIG_COUNT) {
        ESP_LOGE(TAG, "无效的动画类型: %d", anim_type);
        return false;
    }
    
    lottie_cmd_t cmd;
    cmd.type = LOTTIE_CMD_PLAY;
    cmd.data.play.anim_type = anim_type;
    
    if (xQueueSend(g_cmd_queue, &cmd, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "发送播放命令失败，动画类型: %d", anim_type);
        return false;
    }
    
    ESP_LOGI(TAG, "播放命令已发送，动画类型: %d", anim_type);
    return true;
}

void lottie_manager_stop_anim(int anim_type)
{
    if (!g_initialized || !g_cmd_queue) {
        ESP_LOGW(TAG, "管理器未初始化");
        return;
    }
    
    lottie_cmd_t cmd;
    cmd.type = LOTTIE_CMD_STOP;
    cmd.data.stop.anim_type = anim_type;
    
    if (xQueueSend(g_cmd_queue, &cmd, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "发送停止命令失败，动画类型: %d", anim_type);
    } else {
        ESP_LOGI(TAG, "停止命令已发送，动画类型: %d", anim_type);
    }
}