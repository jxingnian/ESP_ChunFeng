/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-27 21:00:00
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-27 21:00:00
 * @FilePath: \esp-brookesia-chunfeng\components\esp_coze_open\src\esp_coze_audio_flash.c
 * @Description: 音频Flash存储实现
 * 
 */
#include "esp_coze_audio_flash.h"
#include "esp_log.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "AUDIO_FLASH";

// 全局变量
static esp_coze_audio_flash_t g_audio_flash = {0};
static esp_coze_audio_play_callback_t g_play_callback = NULL;
static void *g_play_user_data = NULL;
static TaskHandle_t g_player_task_handle = NULL;
static bool g_player_running = false;

/**
 * @brief Base64解码函数
 */
static int base64_decode(const char *base64_data, uint8_t *output, size_t max_len)
{
    static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int len = strlen(base64_data);
    int output_len = 0;
    int pad = 0;
    
    if (len % 4 != 0) return -1;
    if (len > 0 && base64_data[len-1] == '=') pad++;
    if (len > 1 && base64_data[len-2] == '=') pad++;
    
    for (int i = 0; i < len; i += 4) {
        uint32_t tmp = 0;
        for (int j = 0; j < 4; j++) {
            char c = base64_data[i + j];
            if (c == '=') break;
            
            char *pos = strchr(base64_chars, c);
            if (!pos) return -1;
            
            tmp = (tmp << 6) | (pos - base64_chars);
        }
        
        if (output_len + 3 > max_len) break;
        
        output[output_len++] = (tmp >> 16) & 0xFF;
        if (i + 2 < len - pad) output[output_len++] = (tmp >> 8) & 0xFF;
        if (i + 3 < len - pad) output[output_len++] = tmp & 0xFF;
    }
    
    return output_len;
}

/**
 * @brief 初始化音频Flash存储
 */
esp_err_t esp_coze_audio_flash_init(esp_coze_audio_flash_t *af, uint32_t base_addr, size_t size)
{
    if (!af) {
        return ESP_ERR_INVALID_ARG;
    }

    af->base_addr = base_addr;
    af->size = size;
    af->write_offset = 0;
    af->read_offset = 0;

    af->mutex = xSemaphoreCreateMutex();
    if (!af->mutex) {
        ESP_LOGE(TAG, "创建互斥锁失败");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "音频Flash存储初始化成功，地址: 0x%x, 大小: %d KB", 
             (unsigned int)base_addr, (int)(size / 1024));
    return ESP_OK;
}

/**
 * @brief 写入PCM音频数据到Flash
 */
esp_err_t esp_coze_audio_flash_write(esp_coze_audio_flash_t *af, const uint8_t *pcm_data, size_t len)
{
    if (!af || !pcm_data || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(af->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "获取互斥锁超时");
        return ESP_ERR_TIMEOUT;
    }

    // 检查剩余空间
    uint32_t available_space;
    if (af->write_offset >= af->read_offset) {
        available_space = af->size - af->write_offset + af->read_offset;
    } else {
        available_space = af->read_offset - af->write_offset;
    }

    if (len >= available_space) {
        ESP_LOGW(TAG, "Flash空间不足，需要: %d, 可用: %d", (int)len, (int)available_space);
        xSemaphoreGive(af->mutex);
        return ESP_ERR_NO_MEM;
    }

    // 写入数据（这里简化处理，实际应用中需要考虑Flash擦除等操作）
    uint32_t write_addr = af->base_addr + af->write_offset;
    
    // 模拟Flash写入（实际项目中使用esp_partition_write）
    ESP_LOGD(TAG, "写入音频数据到Flash: 地址=0x%x, 长度=%d", (unsigned int)write_addr, (int)len);
    
    af->write_offset = (af->write_offset + len) % af->size;

    xSemaphoreGive(af->mutex);

    ESP_LOGD(TAG, "写入PCM数据: %d bytes, 写入偏移: %d", (int)len, (int)af->write_offset);
    return ESP_OK;
}

/**
 * @brief 从Flash读取PCM音频数据
 */
esp_err_t esp_coze_audio_flash_read(esp_coze_audio_flash_t *af, uint8_t *pcm_data, size_t len, size_t *actual_len)
{
    if (!af || !pcm_data || !actual_len) {
        return ESP_ERR_INVALID_ARG;
    }

    *actual_len = 0;

    if (xSemaphoreTake(af->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "获取互斥锁超时");
        return ESP_ERR_TIMEOUT;
    }

    // 计算可读数据量
    uint32_t available = esp_coze_audio_flash_available(af);
    if (available == 0) {
        xSemaphoreGive(af->mutex);
        return ESP_ERR_NOT_FOUND;
    }

    size_t read_len = (len < available) ? len : available;
    uint32_t read_addr = af->base_addr + af->read_offset;

    // 模拟Flash读取（实际项目中使用esp_partition_read）
    ESP_LOGD(TAG, "从Flash读取音频数据: 地址=0x%x, 长度=%d", (unsigned int)read_addr, (int)read_len);
    
    // 这里应该从Flash读取数据到pcm_data，现在只是模拟
    memset(pcm_data, 0, read_len);  // 模拟数据
    
    af->read_offset = (af->read_offset + read_len) % af->size;
    *actual_len = read_len;

    xSemaphoreGive(af->mutex);

    ESP_LOGD(TAG, "读取PCM数据: %d bytes, 读取偏移: %d", (int)read_len, (int)af->read_offset);
    return ESP_OK;
}

/**
 * @brief 获取Flash中可读的音频数据量
 */
uint32_t esp_coze_audio_flash_available(esp_coze_audio_flash_t *af)
{
    if (!af) {
        return 0;
    }

    uint32_t available;
    if (af->write_offset >= af->read_offset) {
        available = af->write_offset - af->read_offset;
    } else {
        available = af->size - af->read_offset + af->write_offset;
    }

    return available;
}

/**
 * @brief 清空Flash音频缓冲区
 */
esp_err_t esp_coze_audio_flash_clear(esp_coze_audio_flash_t *af)
{
    if (!af) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(af->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    af->write_offset = 0;
    af->read_offset = 0;

    xSemaphoreGive(af->mutex);

    ESP_LOGI(TAG, "Flash音频缓冲区已清空");
    return ESP_OK;
}

/**
 * @brief 音频播放任务
 */
static void audio_player_task(void *param)
{
    uint8_t pcm_buffer[1024];
    size_t read_len;
    
    ESP_LOGI(TAG, "音频播放任务启动");
    
    while (g_player_running) {
        if (esp_coze_audio_flash_available(&g_audio_flash) > 512) {
            if (esp_coze_audio_flash_read(&g_audio_flash, pcm_buffer, sizeof(pcm_buffer), &read_len) == ESP_OK) {
                if (g_play_callback && read_len > 0) {
                    g_play_callback(pcm_buffer, read_len, g_play_user_data);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms播放周期
    }
    
    ESP_LOGI(TAG, "音频播放任务退出");
    g_player_task_handle = NULL;
    vTaskDelete(NULL);
}

/**
 * @brief 注册音频播放回调函数
 */
esp_err_t esp_coze_audio_register_play_callback(esp_coze_audio_play_callback_t callback, void *user_data)
{
    g_play_callback = callback;
    g_play_user_data = user_data;
    ESP_LOGI(TAG, "音频播放回调函数已注册");
    return ESP_OK;
}

/**
 * @brief 启动音频播放任务
 */
esp_err_t esp_coze_audio_player_start(void)
{
    if (g_player_task_handle) {
        ESP_LOGW(TAG, "音频播放任务已在运行");
        return ESP_OK;
    }

    g_player_running = true;
    
    BaseType_t ret = xTaskCreate(audio_player_task, "audio_player", 4096, NULL, 5, &g_player_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "创建音频播放任务失败");
        g_player_running = false;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "音频播放任务启动成功");
    return ESP_OK;
}

/**
 * @brief 停止音频播放任务
 */
esp_err_t esp_coze_audio_player_stop(void)
{
    g_player_running = false;
    
    // 等待任务退出
    int retry = 100;
    while (g_player_task_handle && retry-- > 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_LOGI(TAG, "音频播放任务已停止");
    return ESP_OK;
}

/**
 * @brief 处理base64音频数据
 */
esp_err_t esp_coze_audio_process_base64(const char *base64_data)
{
    if (!base64_data) {
        return ESP_ERR_INVALID_ARG;
    }

    static uint8_t pcm_buffer[2048];
    int pcm_len = base64_decode(base64_data, pcm_buffer, sizeof(pcm_buffer));
    
    if (pcm_len > 0) {
        return esp_coze_audio_flash_write(&g_audio_flash, pcm_buffer, pcm_len);
    }

    return ESP_FAIL;
}

/**
 * @brief 获取全局音频Flash实例
 */
esp_coze_audio_flash_t* esp_coze_audio_get_flash_instance(void)
{
    return &g_audio_flash;
}

/**
 * @brief 销毁音频Flash存储
 */
void esp_coze_audio_flash_deinit(esp_coze_audio_flash_t *af)
{
    if (!af) {
        return;
    }

    esp_coze_audio_player_stop();

    if (af->mutex) {
        vSemaphoreDelete(af->mutex);
    }

    memset(af, 0, sizeof(esp_coze_audio_flash_t));
    ESP_LOGI(TAG, "音频Flash存储销毁完成");
}
