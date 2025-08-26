/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file basic_usage.c
 * @brief ESP扣子聊天组件基本使用示例
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_coze_chat.h"

static const char *TAG = "COZE_EXAMPLE";

static esp_coze_chat_handle_t s_coze_handle = NULL;

/**
 * @brief 扣子事件回调函数
 */
static void coze_event_callback(esp_coze_chat_handle_t handle, esp_coze_event_data_t *event_data, void *user_data)
{
    switch (event_data->event_type) {
    case ESP_COZE_EVENT_CONNECTED:
        ESP_LOGI(TAG, "✅ 扣子服务器连接成功");
        // 连接成功后发送欢迎消息
        esp_coze_chat_send_text(handle, "你好，我是ESP32设备！");
        break;

    case ESP_COZE_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "❌ 扣子服务器连接断开");
        break;

    case ESP_COZE_EVENT_ERROR:
        ESP_LOGE(TAG, "⚠️ 扣子服务器错误: %s",
                 event_data->message ? event_data->message : "未知错误");
        break;

    case ESP_COZE_EVENT_CONVERSATION_START:
        ESP_LOGI(TAG, "🎯 对话开始");
        break;

    case ESP_COZE_EVENT_CONVERSATION_END:
        ESP_LOGI(TAG, "🏁 对话结束");
        break;

    case ESP_COZE_EVENT_AUDIO_START:
        ESP_LOGI(TAG, "🎵 音频开始");
        break;

    case ESP_COZE_EVENT_AUDIO_END:
        ESP_LOGI(TAG, "🔇 音频结束");
        break;

    case ESP_COZE_EVENT_MESSAGE_RECEIVED:
        ESP_LOGI(TAG, "📨 收到消息: %s",
                 event_data->message ? event_data->message : "");
        break;

    case ESP_COZE_EVENT_ASR_RESULT:
        ESP_LOGI(TAG, "🎤 语音识别结果: %s",
                 event_data->message ? event_data->message : "");
        break;

    case ESP_COZE_EVENT_TTS_AUDIO:
        ESP_LOGI(TAG, "🔊 收到TTS音频数据，长度: %d字节", event_data->data_len);
        // 在这里可以播放音频数据
        // audio_play(event_data->data, event_data->data_len);
        break;

    case ESP_COZE_EVENT_SPEECH_START:
        ESP_LOGI(TAG, "🗣️ 开始说话");
        break;

    case ESP_COZE_EVENT_SPEECH_END:
        ESP_LOGI(TAG, "🤫 停止说话");
        break;

    default:
        ESP_LOGW(TAG, "❓ 未知事件类型: %d", event_data->event_type);
        break;
    }
}

/**
 * @brief 初始化WiFi（示例）
 */
static void wifi_init(void)
{
    // 这里应该实现WiFi连接逻辑
    // 为了简化示例，这里只是占位符
    ESP_LOGI(TAG, "WiFi初始化（请实现实际的WiFi连接逻辑）");
}

/**
 * @brief 扣子聊天测试任务
 */
static void coze_chat_test_task(void *param)
{
    // 配置扣子聊天客户端
    esp_coze_chat_config_t config = {
        .ws_base_url = "wss://ws.coze.cn/v1/chat",
        .access_token = "YOUR_ACCESS_TOKEN_HERE",  // 请替换为实际的访问令牌
        .bot_id = "YOUR_BOT_ID_HERE",              // 请替换为实际的智能体ID
        .device_id = "esp32_example_device",
        .conversation_id = NULL,
        .workflow_id = NULL,
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

    // 初始化扣子聊天客户端
    esp_err_t ret = esp_coze_chat_init(&config, &s_coze_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "扣子聊天客户端初始化失败: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "扣子聊天客户端初始化成功");

    // 启动连接
    ret = esp_coze_chat_start(s_coze_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "启动扣子聊天连接失败: %s", esp_err_to_name(ret));
        esp_coze_chat_destroy(s_coze_handle);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "正在连接到扣子服务器...");

    // 等待连接成功
    int retry_count = 0;
    while (!esp_coze_chat_is_connected(s_coze_handle) && retry_count < 30) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        retry_count++;
        ESP_LOGI(TAG, "等待连接... (%d/30)", retry_count);
    }

    if (!esp_coze_chat_is_connected(s_coze_handle)) {
        ESP_LOGE(TAG, "连接超时，请检查网络和配置");
        esp_coze_chat_destroy(s_coze_handle);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "连接成功！开始测试...");

    // 测试文本消息发送
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_coze_chat_send_text(s_coze_handle, "这是一条测试消息");

    // 测试语音输入控制
    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP_LOGI(TAG, "开始语音输入测试");
    esp_coze_chat_start_speech_input(s_coze_handle);

    // 模拟发送音频数据
    vTaskDelay(pdMS_TO_TICKS(2000));
    uint8_t dummy_audio[320]; // 16kHz, 1通道, 16位, 10ms音频数据
    memset(dummy_audio, 0, sizeof(dummy_audio));
    esp_coze_chat_send_audio(s_coze_handle, dummy_audio, sizeof(dummy_audio));

    // 停止语音输入
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_coze_chat_stop_speech_input(s_coze_handle);
    ESP_LOGI(TAG, "语音输入测试完成");

    // 获取统计信息
    esp_coze_stats_t stats;
    if (esp_coze_chat_get_stats(s_coze_handle, &stats) == ESP_OK) {
        ESP_LOGI(TAG, "📊 统计信息:");
        ESP_LOGI(TAG, "  发送消息数: %lu", stats.total_messages_sent);
        ESP_LOGI(TAG, "  接收消息数: %lu", stats.total_messages_received);
        ESP_LOGI(TAG, "  发送音频字节数: %lu", stats.total_audio_bytes_sent);
        ESP_LOGI(TAG, "  接收音频字节数: %lu", stats.total_audio_bytes_received);
        ESP_LOGI(TAG, "  连接次数: %lu", stats.connection_count);
        ESP_LOGI(TAG, "  重连次数: %lu", stats.reconnection_count);
        ESP_LOGI(TAG, "  会话持续时间: %llu ms", stats.session_duration_ms);
    }

    // 保持连接，处理后续消息
    ESP_LOGI(TAG, "测试完成，保持连接状态...");
    while (1) {
        // 每30秒发送一条心跳消息
        vTaskDelay(pdMS_TO_TICKS(30000));
        if (esp_coze_chat_is_connected(s_coze_handle)) {
            esp_coze_chat_send_text(s_coze_handle, "心跳消息 - ESP32设备正常运行");
        }
    }

    // 清理资源（实际上不会执行到这里）
    esp_coze_chat_destroy(s_coze_handle);
    vTaskDelete(NULL);
}

/**
 * @brief 应用程序入口点
 */
void app_main(void)
{
    ESP_LOGI(TAG, "ESP扣子聊天组件基本使用示例");
    ESP_LOGI(TAG, "版本: %s", esp_coze_chat_get_version());

    // 初始化NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 初始化WiFi
    wifi_init();

    // 设置日志级别
    esp_coze_chat_set_log_level(ESP_LOG_INFO);

    // 创建扣子聊天测试任务
    xTaskCreate(coze_chat_test_task, "coze_chat_test", 8192, NULL, 5, NULL);

    ESP_LOGI(TAG, "应用程序启动完成");
}
