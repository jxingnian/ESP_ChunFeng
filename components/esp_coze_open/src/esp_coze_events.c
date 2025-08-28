/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-27 17:00:00
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-27 17:00:00
 * @FilePath: \esp-brookesia-chunfeng\components\esp_coze_open\src\esp_coze_events.c
 * @Description: 扣子聊天事件管理实现
 *
 */
#include "esp_coze_events.h"
#include "esp_coze_chat.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_timer.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "ESP_COZE_EVENTS";

// 外部声明WebSocket发送函数
extern esp_err_t esp_coze_websocket_send_text(const char *data);

/**
 * @brief 发送chat.update事件
 */
esp_err_t esp_coze_send_chat_update_event(const esp_coze_session_config_t *session_config)
{
    return esp_coze_send_custom_chat_update_event(NULL, session_config);
}

/**
 * @brief 发送自定义chat.update事件
 */
esp_err_t esp_coze_send_custom_chat_update_event(const char *event_id,
                                                 const esp_coze_session_config_t *session_config)
{
    if (!session_config) {
        ESP_LOGE(TAG, "会话配置不能为空");
        return ESP_ERR_INVALID_ARG;
    }

    // 生成事件ID
    char generated_id[64];
    if (event_id) {
        strncpy(generated_id, event_id, sizeof(generated_id) - 1);
        generated_id[sizeof(generated_id) - 1] = '\0';
    } else {
        esp_err_t ret = esp_coze_generate_event_id(generated_id, sizeof(generated_id));
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "生成事件ID失败");
            return ret;
        }
    }

    // 创建事件结构体
    esp_coze_chat_update_event_t event = {
        .id = generated_id,
        .event_type = ESP_COZE_EVENT_TYPE_CHAT_UPDATE,
        .data = (esp_coze_session_config_t *)session_config
    };

    // 创建JSON
    cJSON *event_json = esp_coze_create_chat_update_event_json(&event);
    if (!event_json) {
        ESP_LOGE(TAG, "创建事件JSON失败");
        return ESP_FAIL;
    }

    // 转换为字符串
    char *json_string = cJSON_Print(event_json);
    cJSON_Delete(event_json);

    if (!json_string) {
        ESP_LOGE(TAG, "转换JSON为字符串失败");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "发送chat.update事件: %s", json_string);

    // 发送WebSocket消息
    esp_err_t ret = esp_coze_websocket_send_text(json_string);

    free(json_string);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "发送WebSocket消息失败");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "chat.update事件发送成功，ID: %s", generated_id);
    return ESP_OK;
}

/**
 * @brief 创建默认的会话配置
 */
esp_coze_session_config_t *esp_coze_create_default_session_config(void)
{
    esp_coze_session_config_t *config = calloc(1, sizeof(esp_coze_session_config_t));
    if (!config) {
        ESP_LOGE(TAG, "分配会话配置内存失败");
        return NULL;
    }

    // 创建默认对话配置
    config->chat_config = esp_coze_create_simple_chat_config("default_user", NULL, true);

    // 创建默认输入音频配置
    config->input_audio = esp_coze_create_default_input_audio_config();

    // 创建默认输出音频配置
    config->output_audio = esp_coze_create_default_output_audio_config();

    // 创建默认ASR配置
    config->asr_config = esp_coze_create_default_asr_config();

    // 设置默认值
    config->need_play_prologue = false;
    config->prologue_content = NULL;

    return config;
}

/**
 * @brief 释放会话配置内存
 */
void esp_coze_free_session_config(esp_coze_session_config_t *config)
{
    if (!config) {
        return;
    }

    // 释放对话配置
    if (config->chat_config) {
        if (config->chat_config->meta_data) {
            cJSON_Delete(config->chat_config->meta_data);
        }
        if (config->chat_config->custom_variables) {
            cJSON_Delete(config->chat_config->custom_variables);
        }
        if (config->chat_config->extra_params) {
            cJSON_Delete(config->chat_config->extra_params);
        }
        if (config->chat_config->parameters) {
            cJSON_Delete(config->chat_config->parameters);
        }
        if (config->chat_config->user_id) {
            free(config->chat_config->user_id);
        }
        if (config->chat_config->conversation_id) {
            free(config->chat_config->conversation_id);
        }
        free(config->chat_config);
    }

    // 释放输入音频配置
    if (config->input_audio) {
        free(config->input_audio);
    }

    // 释放输出音频配置
    if (config->output_audio) {
        if (config->output_audio->pcm_config) {
            if (config->output_audio->pcm_config->limit_config) {
                free(config->output_audio->pcm_config->limit_config);
            }
            free(config->output_audio->pcm_config);
        }
        if (config->output_audio->opus_config) {
            if (config->output_audio->opus_config->limit_config) {
                free(config->output_audio->opus_config->limit_config);
            }
            free(config->output_audio->opus_config);
        }
        if (config->output_audio->voice_id) {
            free(config->output_audio->voice_id);
        }
        if (config->output_audio->emotion_config) {
            free(config->output_audio->emotion_config);
        }
        free(config->output_audio);
    }

    // 释放事件订阅列表
    if (config->event_subscriptions) {
        for (int i = 0; i < config->subscription_count; i++) {
            if (config->event_subscriptions[i]) {
                free(config->event_subscriptions[i]);
            }
        }
        free(config->event_subscriptions);
    }

    // 释放开场白内容
    if (config->prologue_content) {
        free(config->prologue_content);
    }

    // 释放转检测配置
    if (config->turn_detection) {
        if (config->turn_detection->interrupt_config) {
            if (config->turn_detection->interrupt_config->keywords) {
                for (int i = 0; i < config->turn_detection->interrupt_config->keyword_count; i++) {
                    if (config->turn_detection->interrupt_config->keywords[i]) {
                        free(config->turn_detection->interrupt_config->keywords[i]);
                    }
                }
                free(config->turn_detection->interrupt_config->keywords);
            }
            free(config->turn_detection->interrupt_config);
        }
        free(config->turn_detection);
    }

    // 释放ASR配置
    if (config->asr_config) {
        if (config->asr_config->hot_words) {
            for (int i = 0; i < config->asr_config->hot_word_count; i++) {
                if (config->asr_config->hot_words[i]) {
                    free(config->asr_config->hot_words[i]);
                }
            }
            free(config->asr_config->hot_words);
        }
        if (config->asr_config->context) {
            free(config->asr_config->context);
        }
        free(config->asr_config);
    }

    // 释放声纹配置
    if (config->voice_print_config) {
        if (config->voice_print_config->group_id) {
            free(config->voice_print_config->group_id);
        }
        free(config->voice_print_config);
    }

    // 释放主结构体
    free(config);
}

/**
 * @brief 创建简单的对话配置
 */
esp_coze_chat_config_data_t *esp_coze_create_simple_chat_config(const char *user_id,
                                                                const char *conversation_id,
                                                                bool auto_save_history)
{
    esp_coze_chat_config_data_t *config = calloc(1, sizeof(esp_coze_chat_config_data_t));
    if (!config) {
        ESP_LOGE(TAG, "分配对话配置内存失败");
        return NULL;
    }

    if (user_id) {
        config->user_id = strdup(user_id);
    }

    if (conversation_id) {
        config->conversation_id = strdup(conversation_id);
    }

    config->auto_save_history = auto_save_history;

    return config;
}

/**
 * @brief 创建默认输入音频配置
 */
esp_coze_input_audio_config_t *esp_coze_create_default_input_audio_config(void)
{
    esp_coze_input_audio_config_t *config = calloc(1, sizeof(esp_coze_input_audio_config_t));
    if (!config) {
        ESP_LOGE(TAG, "分配输入音频配置内存失败");
        return NULL;
    }

    // 设置默认值
    config->format = ESP_COZE_AUDIO_FORMAT_WAV;
    config->codec = ESP_COZE_AUDIO_CODEC_PCM;
    config->sample_rate = 24000;
    config->channel = 1;
    config->bit_depth = 16;

    return config;
}

/**
 * @brief 创建默认输出音频配置
 */
esp_coze_output_audio_config_t *esp_coze_create_default_output_audio_config(void)
{
    esp_coze_output_audio_config_t *config = calloc(1, sizeof(esp_coze_output_audio_config_t));
    if (!config) {
        ESP_LOGE(TAG, "分配输出音频配置内存失败");
        return NULL;
    }

    // 设置默认值
    config->codec = ESP_COZE_AUDIO_CODEC_PCM;
    config->speech_rate = 0;  // 默认语速

    // 创建默认PCM配置
    config->pcm_config = calloc(1, sizeof(esp_coze_pcm_config_t));
    if (config->pcm_config) {
        config->pcm_config->sample_rate = 24000;
        config->pcm_config->frame_size_ms = 0;  // 不限制
        config->pcm_config->limit_config = NULL;
    }

    return config;
}

/**
 * @brief 创建默认ASR配置
 */
esp_coze_asr_config_t *esp_coze_create_default_asr_config(void)
{
    esp_coze_asr_config_t *config = calloc(1, sizeof(esp_coze_asr_config_t));
    if (!config) {
        ESP_LOGE(TAG, "分配ASR配置内存失败");
        return NULL;
    }

    // 设置默认值
    config->user_language = ESP_COZE_USER_LANG_COMMON;
    config->enable_ddc = true;
    config->enable_itn = true;
    config->enable_punc = true;
    config->hot_words = NULL;
    config->hot_word_count = 0;
    config->context = NULL;

    return config;
}

/**
 * @brief 发送input_text.generate_audio语音合成事件
 */
esp_err_t esp_coze_send_text_generate_audio_event(const char *event_id, const char *text)
{
    if (!text || strlen(text) == 0) {
        ESP_LOGE(TAG, "合成文本不能为空");
        return ESP_ERR_INVALID_ARG;
    }

    if (strlen(text) > 1024) {
        ESP_LOGE(TAG, "合成文本长度超过限制(1024字符)");
        return ESP_ERR_INVALID_ARG;
    }

    // 生成事件ID
    char generated_id[64];
    if (event_id) {
        strncpy(generated_id, event_id, sizeof(generated_id) - 1);
        generated_id[sizeof(generated_id) - 1] = '\0';
    } else {
        esp_err_t ret = esp_coze_generate_event_id(generated_id, sizeof(generated_id));
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "生成事件ID失败");
            return ret;
        }
    }

    // 创建JSON对象
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "创建JSON对象失败");
        return ESP_ERR_NO_MEM;
    }

    // 设置基本字段
    cJSON_AddStringToObject(json, "id", generated_id);
    cJSON_AddStringToObject(json, "event_type", "input_text.generate_audio");

    // 创建data对象
    cJSON *data = cJSON_CreateObject();
    if (!data) {
        ESP_LOGE(TAG, "创建data对象失败");
        cJSON_Delete(json);
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(data, "mode", "text");
    cJSON_AddStringToObject(data, "text", text);
    cJSON_AddItemToObject(json, "data", data);

    // 转换为字符串
    char *json_string = cJSON_Print(json);
    cJSON_Delete(json);

    if (!json_string) {
        ESP_LOGE(TAG, "转换JSON为字符串失败");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "发送语音合成事件: %s", json_string);

    // 发送WebSocket消息
    esp_err_t ret = esp_coze_websocket_send_text(json_string);
    free(json_string);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "语音合成事件发送成功，ID: %s", generated_id);
    } else {
        ESP_LOGE(TAG, "发送语音合成事件失败: %s", esp_err_to_name(ret));
    }

    return ret;
}

/**
 * @brief 发送conversation.chat.cancel打断事件
 */
esp_err_t esp_coze_send_conversation_cancel_event(const char *event_id)
{
    // 生成事件ID
    char generated_id[64];
    if (event_id) {
        strncpy(generated_id, event_id, sizeof(generated_id) - 1);
        generated_id[sizeof(generated_id) - 1] = '\0';
    } else {
        esp_err_t ret = esp_coze_generate_event_id(generated_id, sizeof(generated_id));
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "生成事件ID失败");
            return ret;
        }
    }

    // 创建JSON对象
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "创建JSON对象失败");
        return ESP_ERR_NO_MEM;
    }

    // 设置基本字段
    cJSON_AddStringToObject(json, "id", generated_id);
    cJSON_AddStringToObject(json, "event_type", "conversation.chat.cancel");

    // 转换为字符串
    char *json_string = cJSON_Print(json);
    cJSON_Delete(json);

    if (!json_string) {
        ESP_LOGE(TAG, "转换JSON为字符串失败");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "发送打断事件: %s", json_string);

    // 发送WebSocket消息
    esp_err_t ret = esp_coze_websocket_send_text(json_string);
    free(json_string);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "打断事件发送成功，ID: %s", generated_id);
    } else {
        ESP_LOGE(TAG, "发送打断事件失败: %s", esp_err_to_name(ret));
    }

    return ret;
}

/**
 * @brief 发送input_audio_buffer.append事件
 */
esp_err_t esp_coze_send_input_audio_buffer_append_event(const char *event_id, const uint8_t *audio_data, size_t data_len)
{
    if (!audio_data || data_len == 0) {
        ESP_LOGE(TAG, "音频数据不能为空");
        return ESP_ERR_INVALID_ARG;
    }

    // 生成事件ID
    char generated_id[64];
    if (event_id) {
        strncpy(generated_id, event_id, sizeof(generated_id) - 1);
        generated_id[sizeof(generated_id) - 1] = '\0';
    } else {
        esp_err_t ret = esp_coze_generate_event_id(generated_id, sizeof(generated_id));
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "生成事件ID失败");
            return ret;
        }
    }

    // 创建JSON对象
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "创建JSON对象失败");
        return ESP_ERR_NO_MEM;
    }

    // 设置基本字段
    cJSON_AddStringToObject(json, "id", generated_id);
    cJSON_AddStringToObject(json, "event_type", "input_audio_buffer.append");

    // 创建data对象
    cJSON *data = cJSON_CreateObject();
    if (!data) {
        ESP_LOGE(TAG, "创建data对象失败");
        cJSON_Delete(json);
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(data, "delta", (const char *)audio_data);
    cJSON_AddItemToObject(json, "data", data);

    // 转换为字符串
    char *json_string = cJSON_Print(json);
    cJSON_Delete(json);

    if (!json_string) {
        ESP_LOGE(TAG, "转换JSON为字符串失败");
        return ESP_ERR_NO_MEM;
    }

    // 发送WebSocket消息
    esp_err_t ret = esp_coze_websocket_send_text(json_string);
    free(json_string);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "发送音频片段失败: %s", esp_err_to_name(ret));
    }

    return ret;
}

/**
 * @brief 发送input_audio_buffer.complete事件
 */
esp_err_t esp_coze_send_input_audio_buffer_complete_event(const char *event_id)
{
    // 生成事件ID
    char generated_id[64];
    if (event_id) {
        strncpy(generated_id, event_id, sizeof(generated_id) - 1);
        generated_id[sizeof(generated_id) - 1] = '\0';
    } else {
        esp_err_t ret = esp_coze_generate_event_id(generated_id, sizeof(generated_id));
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "生成事件ID失败");
            return ret;
        }
    }

    // 创建JSON对象
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "创建JSON对象失败");
        return ESP_ERR_NO_MEM;
    }

    // 设置基本字段
    cJSON_AddStringToObject(json, "id", generated_id);
    cJSON_AddStringToObject(json, "event_type", "input_audio_buffer.complete");

    // 转换为字符串
    char *json_string = cJSON_Print(json);
    cJSON_Delete(json);

    if (!json_string) {
        ESP_LOGE(TAG, "转换JSON为字符串失败");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "发送音频提交事件: %s", json_string);

    // 发送WebSocket消息
    esp_err_t ret = esp_coze_websocket_send_text(json_string);
    free(json_string);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "音频提交事件发送成功，ID: %s", generated_id);
    } else {
        ESP_LOGE(TAG, "发送音频提交事件失败: %s", esp_err_to_name(ret));
    }

    return ret;
}