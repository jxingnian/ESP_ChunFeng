/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_websocket_client.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 扣子聊天组件版本信息
 */
#define ESP_COZE_CHAT_VERSION "1.0.0"

/**
 * @brief 扣子聊天句柄类型
 */
typedef struct esp_coze_chat_handle_s *esp_coze_chat_handle_t;

/**
 * @brief 扣子聊天事件类型
 */
typedef enum {
    ESP_COZE_EVENT_CONNECTED,           /*!< WebSocket连接成功 */
    ESP_COZE_EVENT_DISCONNECTED,       /*!< WebSocket连接断开 */
    ESP_COZE_EVENT_ERROR,              /*!< 发生错误 */
    ESP_COZE_EVENT_CONVERSATION_START, /*!< 对话开始 */
    ESP_COZE_EVENT_CONVERSATION_END,   /*!< 对话结束 */
    ESP_COZE_EVENT_AUDIO_START,        /*!< 音频开始 */
    ESP_COZE_EVENT_AUDIO_END,          /*!< 音频结束 */
    ESP_COZE_EVENT_MESSAGE_RECEIVED,   /*!< 收到消息 */
    ESP_COZE_EVENT_ASR_RESULT,         /*!< ASR识别结果 */
    ESP_COZE_EVENT_TTS_AUDIO,          /*!< TTS音频数据 */
    ESP_COZE_EVENT_SPEECH_START,       /*!< 语音开始 */
    ESP_COZE_EVENT_SPEECH_END,         /*!< 语音结束 */
} esp_coze_event_type_t;

/**
 * @brief 扣子聊天音频格式
 */
typedef enum {
    ESP_COZE_AUDIO_FORMAT_PCM,         /*!< PCM格式 */
    ESP_COZE_AUDIO_FORMAT_WAV,         /*!< WAV格式 */
    ESP_COZE_AUDIO_FORMAT_MP3,         /*!< MP3格式 */
} esp_coze_audio_format_t;

/**
 * @brief 扣子聊天音频采样率
 */
typedef enum {
    ESP_COZE_SAMPLE_RATE_8000 = 8000,   /*!< 8kHz采样率 */
    ESP_COZE_SAMPLE_RATE_16000 = 16000, /*!< 16kHz采样率 */
    ESP_COZE_SAMPLE_RATE_24000 = 24000, /*!< 24kHz采样率 */
    ESP_COZE_SAMPLE_RATE_48000 = 48000, /*!< 48kHz采样率 */
} esp_coze_sample_rate_t;

/**
 * @brief 扣子聊天音频配置
 */
typedef struct {
    esp_coze_audio_format_t format;     /*!< 音频格式 */
    esp_coze_sample_rate_t sample_rate; /*!< 采样率 */
    uint8_t channels;                   /*!< 声道数 */
    uint8_t bits_per_sample;            /*!< 每样本位数 */
} esp_coze_audio_config_t;

/**
 * @brief 扣子聊天事件数据
 */
typedef struct {
    esp_coze_event_type_t event_type;   /*!< 事件类型 */
    void *data;                         /*!< 事件数据 */
    size_t data_len;                    /*!< 数据长度 */
    char *message;                      /*!< 消息内容 */
    int error_code;                     /*!< 错误码 */
} esp_coze_event_data_t;

/**
 * @brief 扣子聊天事件回调函数类型
 *
 * @param handle 扣子聊天句柄
 * @param event_data 事件数据
 * @param user_data 用户数据
 */
typedef void (*esp_coze_event_callback_t)(esp_coze_chat_handle_t handle,
                                          esp_coze_event_data_t *event_data,
                                          void *user_data);

/**
 * @brief 扣子聊天配置结构
 */
typedef struct {
    char *ws_base_url;                  /*!< WebSocket基础URL，默认为wss://ws.coze.cn/v1/chat */
    char *access_token;                 /*!< 访问令牌 */
    char *bot_id;                       /*!< 智能体ID */
    char *workflow_id;                  /*!< 工作流ID（可选） */
    char *device_id;                    /*!< 设备ID（可选） */
    char *conversation_id;              /*!< 会话ID（可选） */
    esp_coze_audio_config_t audio_config; /*!< 音频配置 */
    esp_coze_event_callback_t event_callback; /*!< 事件回调函数 */
    void *user_data;                    /*!< 用户数据 */
    uint32_t connect_timeout_ms;        /*!< 连接超时时间（毫秒） */
    uint32_t keepalive_idle_timeout_ms; /*!< 保活空闲超时（毫秒） */
    bool auto_reconnect;                /*!< 是否自动重连 */
    uint8_t max_reconnect_attempts;     /*!< 最大重连次数 */
} esp_coze_chat_config_t;

/**
 * @brief 扣子聊天会话信息
 */
typedef struct {
    char *conversation_id;              /*!< 会话ID */
    char *user_id;                      /*!< 用户ID */
    uint64_t created_at;                /*!< 创建时间戳 */
    bool is_active;                     /*!< 是否活跃 */
} esp_coze_session_info_t;

/**
 * @brief 扣子聊天统计信息
 */
typedef struct {
    uint32_t total_messages_sent;       /*!< 发送消息总数 */
    uint32_t total_messages_received;   /*!< 接收消息总数 */
    uint32_t total_audio_bytes_sent;    /*!< 发送音频字节总数 */
    uint32_t total_audio_bytes_received; /*!< 接收音频字节总数 */
    uint32_t connection_count;          /*!< 连接次数 */
    uint32_t reconnection_count;        /*!< 重连次数 */
    uint64_t session_duration_ms;       /*!< 会话持续时间（毫秒） */
} esp_coze_stats_t;

/**
 * @brief 初始化扣子聊天客户端
 *
 * @param config 配置参数
 * @param handle 返回的句柄
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_INVALID_ARG: 无效参数
 *         - ESP_ERR_NO_MEM: 内存不足
 */
esp_err_t esp_coze_chat_init(const esp_coze_chat_config_t *config, esp_coze_chat_handle_t *handle);

/**
 * @brief 启动扣子聊天连接
 *
 * @param handle 扣子聊天句柄
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_INVALID_ARG: 无效句柄
 *         - ESP_FAIL: 连接失败
 */
esp_err_t esp_coze_chat_start(esp_coze_chat_handle_t handle);

/**
 * @brief 停止扣子聊天连接
 *
 * @param handle 扣子聊天句柄
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_INVALID_ARG: 无效句柄
 */
esp_err_t esp_coze_chat_stop(esp_coze_chat_handle_t handle);

/**
 * @brief 销毁扣子聊天客户端
 *
 * @param handle 扣子聊天句柄
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_INVALID_ARG: 无效句柄
 */
esp_err_t esp_coze_chat_destroy(esp_coze_chat_handle_t handle);

/**
 * @brief 发送音频数据
 *
 * @param handle 扣子聊天句柄
 * @param audio_data 音频数据
 * @param data_len 数据长度
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_INVALID_ARG: 无效参数
 *         - ESP_FAIL: 发送失败
 */
esp_err_t esp_coze_chat_send_audio(esp_coze_chat_handle_t handle, const uint8_t *audio_data, size_t data_len);

/**
 * @brief 发送文本消息
 *
 * @param handle 扣子聊天句柄
 * @param message 文本消息
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_INVALID_ARG: 无效参数
 *         - ESP_FAIL: 发送失败
 */
esp_err_t esp_coze_chat_send_text(esp_coze_chat_handle_t handle, const char *message);

/**
 * @brief 开始语音输入
 *
 * @param handle 扣子聊天句柄
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_INVALID_ARG: 无效句柄
 *         - ESP_FAIL: 操作失败
 */
esp_err_t esp_coze_chat_start_speech_input(esp_coze_chat_handle_t handle);

/**
 * @brief 停止语音输入
 *
 * @param handle 扣子聊天句柄
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_INVALID_ARG: 无效句柄
 *         - ESP_FAIL: 操作失败
 */
esp_err_t esp_coze_chat_stop_speech_input(esp_coze_chat_handle_t handle);

/**
 * @brief 中断当前对话
 *
 * @param handle 扣子聊天句柄
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_INVALID_ARG: 无效句柄
 *         - ESP_FAIL: 操作失败
 */
esp_err_t esp_coze_chat_interrupt_conversation(esp_coze_chat_handle_t handle);

/**
 * @brief 更新会话配置
 *
 * @param handle 扣子聊天句柄
 * @param config 新的配置参数
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_INVALID_ARG: 无效参数
 *         - ESP_FAIL: 更新失败
 */
esp_err_t esp_coze_chat_update_config(esp_coze_chat_handle_t handle, const esp_coze_chat_config_t *config);

/**
 * @brief 获取会话信息
 *
 * @param handle 扣子聊天句柄
 * @param session_info 返回的会话信息
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_INVALID_ARG: 无效参数
 *         - ESP_FAIL: 获取失败
 */
esp_err_t esp_coze_chat_get_session_info(esp_coze_chat_handle_t handle, esp_coze_session_info_t *session_info);

/**
 * @brief 获取连接状态
 *
 * @param handle 扣子聊天句柄
 * @return bool
 *         - true: 已连接
 *         - false: 未连接
 */
bool esp_coze_chat_is_connected(esp_coze_chat_handle_t handle);

/**
 * @brief 获取统计信息
 *
 * @param handle 扣子聊天句柄
 * @param stats 返回的统计信息
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_INVALID_ARG: 无效参数
 */
esp_err_t esp_coze_chat_get_stats(esp_coze_chat_handle_t handle, esp_coze_stats_t *stats);

/**
 * @brief 重置统计信息
 *
 * @param handle 扣子聊天句柄
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_INVALID_ARG: 无效句柄
 */
esp_err_t esp_coze_chat_reset_stats(esp_coze_chat_handle_t handle);

/**
 * @brief 设置日志级别
 *
 * @param level 日志级别
 * @return esp_err_t
 *         - ESP_OK: 成功
 */
esp_err_t esp_coze_chat_set_log_level(esp_log_level_t level);

/**
 * @brief 获取版本信息
 *
 * @return const char* 版本字符串
 */
const char *esp_coze_chat_get_version(void);

#ifdef __cplusplus
}
#endif
