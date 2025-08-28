/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-21 17:22:36
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-28 10:09:05
 * @FilePath: \esp-chunfeng\main\coze_chat\coze_chat_app.c
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

// 日志标签
static const char *TAG = "COZE_CHAT_APP";

/**
 * @brief 示例2：自定义配置发送chat.update事件
 */
void example_send_custom_chat_update(void)
{
    ESP_LOGI(TAG, "=== 示例2: 自定义配置发送chat.update事件 ===");

    // 创建会话配置
    esp_coze_session_config_t *config = calloc(1, sizeof(esp_coze_session_config_t));
    if (!config) {
        ESP_LOGE(TAG, "分配会话配置内存失败");
        return;
    }

    // 创建对话配置
    // 不设置conversation_id，让系统自动生成新的会话
    config->chat_config = esp_coze_create_simple_chat_config("user123", NULL, true);

    // 创建自定义输入音频配置
    config->input_audio = calloc(1, sizeof(esp_coze_input_audio_config_t));
    if (config->input_audio) {
        config->input_audio->format = ESP_COZE_AUDIO_FORMAT_PCM;
        config->input_audio->codec = ESP_COZE_AUDIO_CODEC_PCM;
        config->input_audio->sample_rate = 16000;  // 16kHz采样率
        config->input_audio->channel = 1;          // 单声道
        config->input_audio->bit_depth = 16;       // 16位深度
    }

    // 创建自定义输出音频配置
    config->output_audio = calloc(1, sizeof(esp_coze_output_audio_config_t));
    if (config->output_audio) {
        config->output_audio->codec = ESP_COZE_AUDIO_CODEC_PCM;
        config->output_audio->speech_rate = 10;    // 1.1倍速
        // 不设置voice_id，使用默认音色
        config->output_audio->voice_id = NULL;
        
        // PCM配置
        config->output_audio->pcm_config = calloc(1, sizeof(esp_coze_pcm_config_t));
        if (config->output_audio->pcm_config) {
            config->output_audio->pcm_config->sample_rate = 16000;
            config->output_audio->pcm_config->frame_size_ms = 20.0f;
        }
    }

    // 创建ASR配置
    config->asr_config = calloc(1, sizeof(esp_coze_asr_config_t));
    if (config->asr_config) {
        config->asr_config->user_language = ESP_COZE_USER_LANG_ZH;  // 中文
        config->asr_config->enable_ddc = true;
        config->asr_config->enable_itn = true;
        config->asr_config->enable_punc = true;
        
        // 添加热词
        config->asr_config->hot_word_count = 2;
        config->asr_config->hot_words = calloc(2, sizeof(char*));
        if (config->asr_config->hot_words) {
            config->asr_config->hot_words[0] = strdup("春风");
            config->asr_config->hot_words[1] = strdup("星年");
        }
        
        config->asr_config->context = strdup("这是一个AI占卜助手、擅长小六壬和梅花易数");
    }

    // 设置开场白
    config->need_play_prologue = true;
    config->prologue_content = strdup("你好，我是春风，擅长小六壬和梅花易数");

    // 发送自定义chat.update事件
    esp_err_t ret = esp_coze_send_custom_chat_update_event("custom-event-001", config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "发送自定义chat.update事件成功");
    } else {
        ESP_LOGE(TAG, "发送自定义chat.update事件失败: %s", esp_err_to_name(ret));
    }

    // 释放内存
    esp_coze_free_session_config(config);
}

/**
 * @brief 示例3：发送语音合成事件
 */
static void example_send_text_to_speech(void)
{
    ESP_LOGI(TAG, "=== 示例3: 发送语音合成事件 ===");
    
    const char *text_to_synthesize = "亲，你怎么不说话了。";
    
    esp_err_t ret = esp_coze_send_text_generate_audio_event("tts-test-001", text_to_synthesize);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "发送语音合成事件失败: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "发送语音合成事件成功，文本: %s", text_to_synthesize);
}

 
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

    // 初始化音频播放器
    ret = audio_player_init(64 * 1024, 1024);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "音频播放器初始化失败: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_ERROR_CHECK(audio_player_start());

    ESP_ERROR_CHECK(esp_coze_chat_start());
    
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    // 会话配置
    example_send_custom_chat_update();
    
    // 等待5秒后发送语音合成
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    example_send_text_to_speech();
    
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    
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

// 实现音频回调：收到PCM投递到播放器
void esp_coze_on_pcm_audio(const int16_t *pcm, size_t sample_count)
{
    if (!audio_player_running()) return;
    audio_player_feed_pcm(pcm, sample_count);
}
