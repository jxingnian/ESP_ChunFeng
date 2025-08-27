/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-27 17:00:00
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-27 17:00:00
 * @FilePath: \esp-brookesia-chunfeng\components\esp_coze_open\src\esp_coze_chat_config.c
 * @Description: 扣子聊天会话配置功能实现
 * 
 */
#include "esp_coze_chat_config.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "esp_random.h"
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

static const char *TAG = "ESP_COZE_CHAT_CONFIG";

// 音频格式字符串映射
static const char* audio_format_strings[] = {
    "pcm", "wav", "ogg"
};

// 音频编码字符串映射
static const char* audio_codec_strings[] = {
    "pcm", "opus", "g711a", "g711u"
};

// 转检测类型字符串映射
static const char* turn_detection_type_strings[] = {
    "server_vad", "client_interrupt"
};

// 打断模式字符串映射
static const char* interrupt_mode_strings[] = {
    "keyword_contains", "keyword_prefix"
};

// 用户语言字符串映射
static const char* user_language_strings[] = {
    "common", "zh", "cant", "sc", "en", "ja", "ko", 
    "fr", "id", "es", "pt", "ms", "ru"
};

// 情感类型字符串映射
static const char* emotion_type_strings[] = {
    "happy", "sad", "angry", "surprised", "fear", 
    "hate", "excited", "coldness", "neutral"
};

/**
 * @brief 创建限流配置JSON对象
 */
static cJSON* create_limit_config_json(const esp_coze_limit_config_t *limit_config)
{
    if (!limit_config) {
        return NULL;
    }

    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "创建限流配置JSON失败");
        return NULL;
    }

    cJSON_AddNumberToObject(json, "period", limit_config->period);
    cJSON_AddNumberToObject(json, "max_frame_num", limit_config->max_frame_num);

    return json;
}

/**
 * @brief 创建PCM配置JSON对象
 */
static cJSON* create_pcm_config_json(const esp_coze_pcm_config_t *pcm_config)
{
    if (!pcm_config) {
        return NULL;
    }

    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "创建PCM配置JSON失败");
        return NULL;
    }

    cJSON_AddNumberToObject(json, "sample_rate", pcm_config->sample_rate);
    if (pcm_config->frame_size_ms > 0) {
        cJSON_AddNumberToObject(json, "frame_size_ms", pcm_config->frame_size_ms);
    }

    if (pcm_config->limit_config) {
        cJSON *limit_json = create_limit_config_json(pcm_config->limit_config);
        if (limit_json) {
            cJSON_AddItemToObject(json, "limit_config", limit_json);
        }
    }

    return json;
}

/**
 * @brief 创建Opus配置JSON对象
 */
static cJSON* create_opus_config_json(const esp_coze_opus_config_t *opus_config)
{
    if (!opus_config) {
        return NULL;
    }

    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "创建Opus配置JSON失败");
        return NULL;
    }

    cJSON_AddNumberToObject(json, "bitrate", opus_config->bitrate);
    cJSON_AddBoolToObject(json, "use_cbr", opus_config->use_cbr);
    if (opus_config->frame_size_ms > 0) {
        cJSON_AddNumberToObject(json, "frame_size_ms", opus_config->frame_size_ms);
    }

    if (opus_config->limit_config) {
        cJSON *limit_json = create_limit_config_json(opus_config->limit_config);
        if (limit_json) {
            cJSON_AddItemToObject(json, "limit_config", limit_json);
        }
    }

    return json;
}

/**
 * @brief 创建输入音频配置JSON对象
 */
static cJSON* create_input_audio_json(const esp_coze_input_audio_config_t *input_audio)
{
    if (!input_audio) {
        return NULL;
    }

    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "创建输入音频配置JSON失败");
        return NULL;
    }

    cJSON_AddStringToObject(json, "format", audio_format_strings[input_audio->format]);
    cJSON_AddStringToObject(json, "codec", audio_codec_strings[input_audio->codec]);
    cJSON_AddNumberToObject(json, "sample_rate", input_audio->sample_rate);
    cJSON_AddNumberToObject(json, "channel", input_audio->channel);
    cJSON_AddNumberToObject(json, "bit_depth", input_audio->bit_depth);

    return json;
}

/**
 * @brief 创建情感配置JSON对象
 */
static cJSON* create_emotion_config_json(const esp_coze_emotion_config_t *emotion_config)
{
    if (!emotion_config) {
        return NULL;
    }

    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "创建情感配置JSON失败");
        return NULL;
    }

    cJSON_AddStringToObject(json, "emotion", emotion_type_strings[emotion_config->emotion]);
    cJSON_AddNumberToObject(json, "emotion_scale", emotion_config->emotion_scale);

    return json;
}

/**
 * @brief 创建输出音频配置JSON对象
 */
static cJSON* create_output_audio_json(const esp_coze_output_audio_config_t *output_audio)
{
    if (!output_audio) {
        return NULL;
    }

    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "创建输出音频配置JSON失败");
        return NULL;
    }

    cJSON_AddStringToObject(json, "codec", audio_codec_strings[output_audio->codec]);

    if (output_audio->pcm_config) {
        cJSON *pcm_json = create_pcm_config_json(output_audio->pcm_config);
        if (pcm_json) {
            cJSON_AddItemToObject(json, "pcm_config", pcm_json);
        }
    }

    if (output_audio->opus_config) {
        cJSON *opus_json = create_opus_config_json(output_audio->opus_config);
        if (opus_json) {
            cJSON_AddItemToObject(json, "opus_config", opus_json);
        }
    }

    if (output_audio->speech_rate != 0) {
        cJSON_AddNumberToObject(json, "speech_rate", output_audio->speech_rate);
    }

    if (output_audio->voice_id) {
        cJSON_AddStringToObject(json, "voice_id", output_audio->voice_id);
    }

    if (output_audio->emotion_config) {
        cJSON *emotion_json = create_emotion_config_json(output_audio->emotion_config);
        if (emotion_json) {
            cJSON_AddItemToObject(json, "emotion_config", emotion_json);
        }
    }

    return json;
}

/**
 * @brief 创建打断配置JSON对象
 */
static cJSON* create_interrupt_config_json(const esp_coze_interrupt_config_t *interrupt_config)
{
    if (!interrupt_config) {
        return NULL;
    }

    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "创建打断配置JSON失败");
        return NULL;
    }

    cJSON_AddStringToObject(json, "mode", interrupt_mode_strings[interrupt_config->mode]);

    if (interrupt_config->keywords && interrupt_config->keyword_count > 0) {
        cJSON *keywords_array = cJSON_CreateArray();
        if (keywords_array) {
            for (int i = 0; i < interrupt_config->keyword_count; i++) {
                if (interrupt_config->keywords[i]) {
                    cJSON_AddItemToArray(keywords_array, cJSON_CreateString(interrupt_config->keywords[i]));
                }
            }
            cJSON_AddItemToObject(json, "keywords", keywords_array);
        }
    }

    return json;
}

/**
 * @brief 创建转检测配置JSON对象
 */
static cJSON* create_turn_detection_json(const esp_coze_turn_detection_config_t *turn_detection)
{
    if (!turn_detection) {
        return NULL;
    }

    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "创建转检测配置JSON失败");
        return NULL;
    }

    cJSON_AddStringToObject(json, "type", turn_detection_type_strings[turn_detection->type]);

    if (turn_detection->prefix_padding_ms > 0) {
        cJSON_AddNumberToObject(json, "prefix_padding_ms", turn_detection->prefix_padding_ms);
    }

    if (turn_detection->silence_duration_ms > 0) {
        cJSON_AddNumberToObject(json, "silence_duration_ms", turn_detection->silence_duration_ms);
    }

    if (turn_detection->interrupt_config) {
        cJSON *interrupt_json = create_interrupt_config_json(turn_detection->interrupt_config);
        if (interrupt_json) {
            cJSON_AddItemToObject(json, "interrupt_config", interrupt_json);
        }
    }

    return json;
}

/**
 * @brief 创建ASR配置JSON对象
 */
static cJSON* create_asr_config_json(const esp_coze_asr_config_t *asr_config)
{
    if (!asr_config) {
        return NULL;
    }

    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "创建ASR配置JSON失败");
        return NULL;
    }

    if (asr_config->hot_words && asr_config->hot_word_count > 0) {
        cJSON *hot_words_array = cJSON_CreateArray();
        if (hot_words_array) {
            for (int i = 0; i < asr_config->hot_word_count; i++) {
                if (asr_config->hot_words[i]) {
                    cJSON_AddItemToArray(hot_words_array, cJSON_CreateString(asr_config->hot_words[i]));
                }
            }
            cJSON_AddItemToObject(json, "hot_words", hot_words_array);
        }
    }

    if (asr_config->context) {
        cJSON_AddStringToObject(json, "context", asr_config->context);
    }

    cJSON_AddStringToObject(json, "user_language", user_language_strings[asr_config->user_language]);
    cJSON_AddBoolToObject(json, "enable_ddc", asr_config->enable_ddc);
    cJSON_AddBoolToObject(json, "enable_itn", asr_config->enable_itn);
    cJSON_AddBoolToObject(json, "enable_punc", asr_config->enable_punc);

    return json;
}

/**
 * @brief 创建声纹配置JSON对象
 */
static cJSON* create_voice_print_config_json(const esp_coze_voice_print_config_t *voice_print_config)
{
    if (!voice_print_config) {
        return NULL;
    }

    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "创建声纹配置JSON失败");
        return NULL;
    }

    if (voice_print_config->group_id) {
        cJSON_AddStringToObject(json, "group_id", voice_print_config->group_id);
    }

    cJSON_AddNumberToObject(json, "score", voice_print_config->score);
    cJSON_AddBoolToObject(json, "reuse_voice_info", voice_print_config->reuse_voice_info);

    return json;
}

/**
 * @brief 创建对话配置JSON对象
 */
static cJSON* create_chat_config_json(const esp_coze_chat_config_data_t *chat_config)
{
    if (!chat_config) {
        return NULL;
    }

    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "创建对话配置JSON失败");
        return NULL;
    }

    if (chat_config->meta_data) {
        cJSON_AddItemToObject(json, "meta_data", cJSON_Duplicate(chat_config->meta_data, 1));
    }

    if (chat_config->custom_variables) {
        cJSON_AddItemToObject(json, "custom_variables", cJSON_Duplicate(chat_config->custom_variables, 1));
    }

    if (chat_config->extra_params) {
        cJSON_AddItemToObject(json, "extra_params", cJSON_Duplicate(chat_config->extra_params, 1));
    }

    if (chat_config->user_id) {
        cJSON_AddStringToObject(json, "user_id", chat_config->user_id);
    }

    if (chat_config->conversation_id) {
        cJSON_AddStringToObject(json, "conversation_id", chat_config->conversation_id);
    }

    cJSON_AddBoolToObject(json, "auto_save_history", chat_config->auto_save_history);

    if (chat_config->parameters) {
        cJSON_AddItemToObject(json, "parameters", cJSON_Duplicate(chat_config->parameters, 1));
    }

    return json;
}

/**
 * @brief 创建会话配置JSON对象
 */
cJSON* esp_coze_create_session_config_json(const esp_coze_session_config_t *session_config)
{
    if (!session_config) {
        ESP_LOGE(TAG, "会话配置为空");
        return NULL;
    }

    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "创建会话配置JSON失败");
        return NULL;
    }

    // 添加对话配置
    if (session_config->chat_config) {
        cJSON *chat_config_json = create_chat_config_json(session_config->chat_config);
        if (chat_config_json) {
            cJSON_AddItemToObject(json, "chat_config", chat_config_json);
        }
    }

    // 添加输入音频配置
    if (session_config->input_audio) {
        cJSON *input_audio_json = create_input_audio_json(session_config->input_audio);
        if (input_audio_json) {
            cJSON_AddItemToObject(json, "input_audio", input_audio_json);
        }
    }

    // 添加输出音频配置
    if (session_config->output_audio) {
        cJSON *output_audio_json = create_output_audio_json(session_config->output_audio);
        if (output_audio_json) {
            cJSON_AddItemToObject(json, "output_audio", output_audio_json);
        }
    }

    // 添加事件订阅
    if (session_config->event_subscriptions && session_config->subscription_count > 0) {
        cJSON *subscriptions_array = cJSON_CreateArray();
        if (subscriptions_array) {
            for (int i = 0; i < session_config->subscription_count; i++) {
                if (session_config->event_subscriptions[i]) {
                    cJSON_AddItemToArray(subscriptions_array, 
                        cJSON_CreateString(session_config->event_subscriptions[i]));
                }
            }
            cJSON_AddItemToObject(json, "event_subscriptions", subscriptions_array);
        }
    }

    // 添加开场白配置
    cJSON_AddBoolToObject(json, "need_play_prologue", session_config->need_play_prologue);
    if (session_config->prologue_content) {
        cJSON_AddStringToObject(json, "prologue_content", session_config->prologue_content);
    }

    // 添加转检测配置
    if (session_config->turn_detection) {
        cJSON *turn_detection_json = create_turn_detection_json(session_config->turn_detection);
        if (turn_detection_json) {
            cJSON_AddItemToObject(json, "turn_detection", turn_detection_json);
        }
    }

    // 添加ASR配置
    if (session_config->asr_config) {
        cJSON *asr_config_json = create_asr_config_json(session_config->asr_config);
        if (asr_config_json) {
            cJSON_AddItemToObject(json, "asr_config", asr_config_json);
        }
    }

    // 添加声纹配置
    if (session_config->voice_print_config) {
        cJSON *voice_print_json = create_voice_print_config_json(session_config->voice_print_config);
        if (voice_print_json) {
            cJSON_AddItemToObject(json, "voice_print_config", voice_print_json);
        }
    }

    return json;
}

/**
 * @brief 创建chat.update事件JSON
 */
cJSON* esp_coze_create_chat_update_event_json(const esp_coze_chat_update_event_t *event)
{
    if (!event || !event->id || !event->event_type) {
        ESP_LOGE(TAG, "chat.update事件参数无效");
        return NULL;
    }

    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "创建chat.update事件JSON失败");
        return NULL;
    }

    cJSON_AddStringToObject(json, "id", event->id);
    cJSON_AddStringToObject(json, "event_type", event->event_type);

    if (event->data) {
        cJSON *data_json = esp_coze_create_session_config_json(event->data);
        if (data_json) {
            cJSON_AddItemToObject(json, "data", data_json);
        }
    }

    return json;
}

/**
 * @brief 生成UUID字符串作为事件ID
 */
esp_err_t esp_coze_generate_event_id(char *event_id, size_t max_len)
{
    if (!event_id || max_len < 37) {  // UUID长度为36个字符 + 1个结束符
        return ESP_ERR_INVALID_ARG;
    }

    // 简单的伪随机UUID生成（实际项目中应使用更好的随机数生成器）
    uint32_t time_ms = esp_timer_get_time() / 1000;
    uint32_t random1 = esp_random();
    uint32_t random2 = esp_random();
    
    snprintf(event_id, max_len, "%08" PRIx32 "-%04" PRIx16 "-%04" PRIx16 "-%04" PRIx16 "-%08" PRIx32 "%04" PRIx16,
             time_ms,
             (uint16_t)(random1 & 0xFFFF),
             (uint16_t)((random1 >> 16) & 0xFFFF),
             (uint16_t)(random2 & 0xFFFF),
             (uint32_t)((random2 >> 16) & 0xFFFFFFFF),
             (uint16_t)(esp_random() & 0xFFFF));

    return ESP_OK;
}
