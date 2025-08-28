/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-27 17:00:00
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-27 17:00:00
 * @FilePath: \esp-brookesia-chunfeng\components\esp_coze_open\include\esp_coze_events.h
 * @Description: 扣子聊天事件管理
 *
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_coze_chat_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 事件类型枚举
 */
typedef enum {
    ESP_COZE_EVENT_CHAT_UPDATE = 0,     ///< chat.update事件
    ESP_COZE_EVENT_CHAT_UPDATED,        ///< chat.updated事件
    ESP_COZE_EVENT_ERROR,               ///< error事件
    ESP_COZE_EVENT_AUDIO_UPLOAD,        ///< 音频上传事件
    ESP_COZE_EVENT_MAX
} esp_coze_event_type_t;

/**
 * @brief 事件类型字符串定义
 */
#define ESP_COZE_EVENT_TYPE_CHAT_UPDATE     "chat.update"
#define ESP_COZE_EVENT_TYPE_CHAT_UPDATED    "chat.updated"
#define ESP_COZE_EVENT_TYPE_ERROR           "error"
#define ESP_COZE_EVENT_TYPE_AUDIO_UPLOAD    "audio.upload"

/**
 * @brief 发送chat.update事件
 *
 * @param session_config 会话配置
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_FAIL: 发送失败
 */
esp_err_t esp_coze_send_chat_update_event(const esp_coze_session_config_t *session_config);

/**
 * @brief 发送自定义chat.update事件
 *
 * @param event_id 自定义事件ID，如果为NULL则自动生成
 * @param session_config 会话配置
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_FAIL: 发送失败
 */
esp_err_t esp_coze_send_custom_chat_update_event(const char *event_id,
                                                 const esp_coze_session_config_t *session_config);

/**
 * @brief 创建默认的会话配置
 *
 * @return esp_coze_session_config_t* 默认配置指针，使用完毕后需要调用esp_coze_free_session_config释放
 */
esp_coze_session_config_t *esp_coze_create_default_session_config(void);

/**
 * @brief 释放会话配置内存
 *
 * @param config 会话配置指针
 */
void esp_coze_free_session_config(esp_coze_session_config_t *config);

/**
 * @brief 创建简单的对话配置
 *
 * @param user_id 用户ID
 * @param conversation_id 会话ID
 * @param auto_save_history 是否自动保存历史
 * @return esp_coze_chat_config_data_t* 对话配置指针
 */
esp_coze_chat_config_data_t *esp_coze_create_simple_chat_config(const char *user_id,
                                                                const char *conversation_id,
                                                                bool auto_save_history);

/**
 * @brief 创建默认输入音频配置
 *
 * @return esp_coze_input_audio_config_t* 输入音频配置指针
 */
esp_coze_input_audio_config_t *esp_coze_create_default_input_audio_config(void);

/**
 * @brief 创建默认输出音频配置
 *
 * @return esp_coze_output_audio_config_t* 输出音频配置指针
 */
esp_coze_output_audio_config_t *esp_coze_create_default_output_audio_config(void);

/**
 * @brief 创建默认ASR配置
 *
 * @return esp_coze_asr_config_t* ASR配置指针
 */
esp_coze_asr_config_t *esp_coze_create_default_asr_config(void);

/**
 * @brief 发送input_text.generate_audio语音合成事件
 *
 * @param event_id 事件ID，如果为NULL则自动生成
 * @param text 要合成的文本内容，长度限制1024字符
 * @return esp_err_t
 *         - ESP_OK: 发送成功
 *         - ESP_ERR_INVALID_ARG: 参数错误
 *         - ESP_ERR_NO_MEM: 内存不足
 *         - 其他: WebSocket发送错误
 */
esp_err_t esp_coze_send_text_generate_audio_event(const char *event_id, const char *text);

/**
 * @brief 发送conversation.chat.cancel打断事件
 *
 * @param event_id 事件ID，如果为NULL则自动生成
 * @return esp_err_t
 *         - ESP_OK: 发送成功
 *         - ESP_ERR_NO_MEM: 内存不足
 *         - 其他: WebSocket发送错误
 */
esp_err_t esp_coze_send_conversation_cancel_event(const char *event_id);

/**
 * @brief 发送input_audio_buffer.append事件
 *
 * @param event_id 事件ID，如果为NULL则自动生成
 * @param audio_data base64编码的音频数据
 * @param data_len 数据长度
 * @return esp_err_t
 *         - ESP_OK: 发送成功
 *         - ESP_ERR_INVALID_ARG: 参数错误
 *         - ESP_ERR_NO_MEM: 内存不足
 *         - 其他: WebSocket发送错误
 */
esp_err_t esp_coze_send_input_audio_buffer_append_event(const char *event_id, const uint8_t *audio_data, size_t data_len);

/**
 * @brief 发送input_audio_buffer.complete事件
 *
 * @param event_id 事件ID，如果为NULL则自动生成
 * @return esp_err_t
 *         - ESP_OK: 发送成功
 *         - ESP_ERR_NO_MEM: 内存不足
 *         - 其他: WebSocket发送错误
 */
esp_err_t esp_coze_send_input_audio_buffer_complete_event(const char *event_id);

#ifdef __cplusplus
}
#endif
