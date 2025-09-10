/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-21 17:22:36
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-09-01 19:29:21
 * @FilePath: \esp-chunfeng\main\coze_chat\coze_chat.c
 * @Description: Coze聊天应用程序实现文件，负责初始化和管理与Coze服务器的WebSocket连接
 *
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

#include "esp_coze_chat.h"
#include "esp_coze_events.h"
#include "esp_coze_chat_config.h"
#include "audio_hal.h"
#include "audio_player.h"
#include "button_voice.h"
#include "esp_heap_caps.h"

// 日志标签
static const char *TAG = "COZE_CHAT_APP";

// /**
//  * @brief 示例3：发送语音合成事件
//  */
// static void example_send_text_to_speech(void)
// {
//     ESP_LOGI(TAG, "=== 示例3: 发送语音合成事件 ===");

//     const char *text_to_synthesize = "亲，你怎么不说话了。";

//     esp_err_t ret = esp_coze_send_text_generate_audio_event("tts-test-001", text_to_synthesize);
//     if (ret != ESP_OK) {
//         ESP_LOGE(TAG, "发送语音合成事件失败: %s", esp_err_to_name(ret));
//         return;
//     }

//     ESP_LOGI(TAG, "发送语音合成事件成功，文本: %s", text_to_synthesize);
// }


/**
 * @brief 初始化并启动Coze聊天服务
 *
 * 配置Coze聊天参数，包括服务器地址、认证信息、音频配置等，
 * 然后初始化并启动与Coze服务器的WebSocket连接
 *
 * @return esp_err_t
 *         - ESP_OK: 初始化成功
 *         - 其他: 初始化失败的错误码
 */
static esp_err_t init_and_start_coze(void)
{
    esp_err_t ret;

    esp_coze_chat_config_t default_config = {
        .ws_base_url = ESP_COZE_DEFAULT_WS_BASE_URL,
        .access_token = ESP_COZE_DEFAULT_ACCESS_TOKEN,
        .bot_id = ESP_COZE_DEFAULT_BOT_ID,
        .device_id = ESP_COZE_DEFAULT_DEVICE_ID,
        .conversation_id = ESP_COZE_DEFAULT_CONVERSATION_ID
    };
    ret = esp_coze_chat_init_with_config(&default_config);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_coze_chat_init 失败: %s", esp_err_to_name(ret));
        return ret;
    }

    // 打印内存使用情况
    size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t internal_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t psram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);

    // ESP_LOGI(TAG, "内存使用情况:");
    // ESP_LOGI(TAG, "  内部RAM: %d KB 可用 / %d KB 总量", (int)(internal_free / 1024), (int)(internal_total / 1024));
    // ESP_LOGI(TAG, "  PSRAM: %d KB 可用 / %d KB 总量", (int)(psram_free / 1024), (int)(psram_total / 1024));

    // 初始化音频播放器 - 增大缓冲区到512KB
    ret = audio_player_init(2048 * 1024, 1024);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "音频播放器初始化失败: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_ERROR_CHECK(audio_player_start());

    // 初始化后再次检查内存
    internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    // ESP_LOGI(TAG, "音频播放器初始化后:");
    // ESP_LOGI(TAG, "  内部RAM: %d KB 可用", (int)(internal_free / 1024));
    // ESP_LOGI(TAG, "  PSRAM: %d KB 可用", (int)(psram_free / 1024));

    // 初始化按键语音输入
    ret = button_voice_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "按键语音输入初始化失败: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_ERROR_CHECK(esp_coze_chat_start());

    return ESP_OK;
}

/**
 * @brief 初始化Coze聊天应用程序
 *
 * 应用程序入口函数，负责启动整个Coze聊天功能
 *
 * @return esp_err_t
 *         - ESP_OK: 初始化成功
 *         - 其他: 初始化失败的错误码
 */
esp_err_t coze_chat_app_init(void)
{
    init_and_start_coze();
    return ESP_OK;
}

/**
 * @brief 简单的线性插值采样率转换：24kHz -> 16kHz
 * @param input 输入24kHz PCM数据
 * @param input_samples 输入样本数
 * @param output 输出16kHz PCM数据缓冲区
 * @param output_capacity 输出缓冲区容量
 * @return 实际输出的样本数
 */
static size_t resample_24k_to_16k(const int16_t *input, size_t input_samples, 
                                  int16_t *output, size_t output_capacity)
{
    if (!input || !output || input_samples == 0 || output_capacity == 0) {
        return 0;
    }
    
    // 24kHz -> 16kHz 转换比例是 2:3
    // 每3个24kHz样本对应2个16kHz样本
    size_t output_samples = (input_samples * 2) / 3;
    if (output_samples > output_capacity) {
        output_samples = output_capacity;
    }
    
    // 线性插值
    for (size_t i = 0; i < output_samples; i++) {
        // 计算在输入中的位置 (浮点)
        float input_pos = (float)i * 3.0f / 2.0f;
        size_t input_idx = (size_t)input_pos;
        float fraction = input_pos - input_idx;
        
        if (input_idx >= input_samples - 1) {
            // 超出范围，使用最后一个样本
            output[i] = input[input_samples - 1];
        } else {
            // 线性插值
            int16_t sample1 = input[input_idx];
            int16_t sample2 = input[input_idx + 1];
            output[i] = (int16_t)(sample1 + fraction * (sample2 - sample1));
        }
    }
    
    return output_samples;
}

// 实现音频回调：收到PCM投递到播放器（24kHz -> 16kHz转换）
void esp_coze_on_pcm_audio(const int16_t *pcm, size_t sample_count)
{
    if (!audio_player_running() || !pcm || sample_count == 0) return;
    
    // 静态缓冲区用于采样率转换
    static int16_t resampled_buffer[960]; // 16kHz缓冲区，足够大
    size_t resampled_count = resample_24k_to_16k(pcm, sample_count, 
                                                resampled_buffer, 
                                                sizeof(resampled_buffer)/sizeof(resampled_buffer[0]));
    if (resampled_count > 0) {
        audio_player_feed_pcm(resampled_buffer, resampled_count);
    }
}
