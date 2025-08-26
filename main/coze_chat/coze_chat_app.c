/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-21 17:22:36
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-26 19:43:42
 * @FilePath: \esp-brookesia-chunfeng\main\coze_chat\coze_chat_app.c
 * @Description:
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
#include "audio_hal.h"

static const char *TAG = "COZE_CHAT_APP";

static esp_coze_chat_handle_t s_coze_handle = NULL;

static void coze_event_callback(esp_coze_chat_handle_t handle, esp_coze_event_data_t *event_data, void *user_data)
{
    switch (event_data->event_type) {
    case ESP_COZE_EVENT_CONNECTED:
        ESP_LOGI(TAG, "扣子服务器连接成功");
        break;

    case ESP_COZE_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "扣子服务器连接断开");
        break;

    case ESP_COZE_EVENT_ERROR:
        ESP_LOGE(TAG, "扣子服务器错误: %s", event_data->message ? event_data->message : "未知错误");
        break;

    case ESP_COZE_EVENT_CONVERSATION_START:
        ESP_LOGI(TAG, "对话开始");
        break;

    case ESP_COZE_EVENT_CONVERSATION_END:
        ESP_LOGI(TAG, "对话结束");
        break;

    case ESP_COZE_EVENT_AUDIO_START:
        ESP_LOGI(TAG, "音频开始");
        break;

    case ESP_COZE_EVENT_AUDIO_END:
        ESP_LOGI(TAG, "音频结束");
        break;

    case ESP_COZE_EVENT_MESSAGE_RECEIVED:
        ESP_LOGI(TAG, "收到消息: %s", event_data->message ? event_data->message : "");
        break;

    case ESP_COZE_EVENT_ASR_RESULT:
        ESP_LOGI(TAG, "ASR识别结果: %s", event_data->message ? event_data->message : "");
        break;

    case ESP_COZE_EVENT_TTS_AUDIO:
        ESP_LOGI(TAG, "收到TTS音频数据，长度: %d", event_data->data_len);
        // 这里可以播放音频数据
        break;

    case ESP_COZE_EVENT_SPEECH_START:
        ESP_LOGI(TAG, "语音开始");
        break;

    case ESP_COZE_EVENT_SPEECH_END:
        ESP_LOGI(TAG, "语音结束");
        break;

    default:
        ESP_LOGW(TAG, "未知事件类型: %d", event_data->event_type);
        break;
    }
}

static esp_err_t init_and_start_coze(void)
{
    esp_err_t ret;

    esp_coze_chat_config_t cfg = {
        .ws_base_url = "wss://ws.coze.cn/v1/chat",
        .access_token = "pat_A7Tk6tCS6qyAEvQg1hlkKAYjXq0TUUmUFUw65jiQjoU9Q4nEPvtMC9A08d75gBUn",
        .bot_id = "7507830126416560143",
        .workflow_id = NULL,
        .device_id = "esp32_device_001",
        .conversation_id = NULL,
        .audio_config = {
            .format = ESP_COZE_AUDIO_FORMAT_PCM,
            .sample_rate = ESP_COZE_SAMPLE_RATE_16000,
            .channels = 1,
            .bits_per_sample = 16,
        },
        .event_callback = coze_event_callback,
        .user_data = NULL,
        .connect_timeout_ms = 10000,
        .keepalive_idle_timeout_ms = 30000,
        .auto_reconnect = true,
        .max_reconnect_attempts = 5,
    };

    ret = esp_coze_chat_init(&cfg, &s_coze_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_coze_chat_init 失败: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_coze_chat_start(s_coze_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_coze_chat_start 失败: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Coze WebSocket 已连接成功");
    return ESP_OK;
}

esp_err_t coze_chat_app_init(void)
{
    ESP_ERROR_CHECK(init_and_start_coze());
    return ESP_OK;
}

esp_err_t coze_chat_app_send_text(const char *message)
{
    if (s_coze_handle == NULL) {
        ESP_LOGE(TAG, "扣子聊天客户端未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    return esp_coze_chat_send_text(s_coze_handle, message);
}

esp_err_t coze_chat_app_send_audio(const uint8_t *audio_data, size_t data_len)
{
    if (s_coze_handle == NULL) {
        ESP_LOGE(TAG, "扣子聊天客户端未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    return esp_coze_chat_send_audio(s_coze_handle, audio_data, data_len);
}

esp_err_t coze_chat_app_start_speech(void)
{
    if (s_coze_handle == NULL) {
        ESP_LOGE(TAG, "扣子聊天客户端未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    return esp_coze_chat_start_speech_input(s_coze_handle);
}

esp_err_t coze_chat_app_stop_speech(void)
{
    if (s_coze_handle == NULL) {
        ESP_LOGE(TAG, "扣子聊天客户端未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    return esp_coze_chat_stop_speech_input(s_coze_handle);
}

bool coze_chat_app_is_connected(void)
{
    if (s_coze_handle == NULL) {
        return false;
    }

    return esp_coze_chat_is_connected(s_coze_handle);
}

esp_err_t coze_chat_app_interrupt(void)
{
    if (s_coze_handle == NULL) {
        ESP_LOGE(TAG, "扣子聊天客户端未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    return esp_coze_chat_interrupt_conversation(s_coze_handle);
}

esp_err_t coze_chat_app_deinit(void)
{
    if (s_coze_handle != NULL) {
        esp_err_t ret = esp_coze_chat_destroy(s_coze_handle);
        s_coze_handle = NULL;
        return ret;
    }

    return ESP_OK;
}
