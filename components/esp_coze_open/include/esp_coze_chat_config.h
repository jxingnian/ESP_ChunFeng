/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-27 17:00:00
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-27 17:00:00
 * @FilePath: \esp-brookesia-chunfeng\components\esp_coze_open\include\esp_coze_chat_config.h
 * @Description: 扣子聊天会话配置相关定义
 *
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 音频格式枚举
 */
typedef enum {
    ESP_COZE_AUDIO_FORMAT_PCM = 0,
    ESP_COZE_AUDIO_FORMAT_WAV,
    ESP_COZE_AUDIO_FORMAT_OGG
} esp_coze_audio_format_t;

/**
 * @brief 音频编码枚举
 */
typedef enum {
    ESP_COZE_AUDIO_CODEC_PCM = 0,
    ESP_COZE_AUDIO_CODEC_OPUS,
    ESP_COZE_AUDIO_CODEC_G711A,
    ESP_COZE_AUDIO_CODEC_G711U
} esp_coze_audio_codec_t;

/**
 * @brief 转检测类型枚举
 */
typedef enum {
    ESP_COZE_TURN_DETECTION_SERVER_VAD = 0,
    ESP_COZE_TURN_DETECTION_CLIENT_INTERRUPT
} esp_coze_turn_detection_type_t;

/**
 * @brief 打断模式枚举
 */
typedef enum {
    ESP_COZE_INTERRUPT_KEYWORD_CONTAINS = 0,
    ESP_COZE_INTERRUPT_KEYWORD_PREFIX
} esp_coze_interrupt_mode_t;

/**
 * @brief 用户语言枚举
 */
typedef enum {
    ESP_COZE_USER_LANG_COMMON = 0,  // 大模型语音识别，可自动识别中英粤
    ESP_COZE_USER_LANG_ZH,          // 中文
    ESP_COZE_USER_LANG_CANT,        // 粤语
    ESP_COZE_USER_LANG_SC,          // 川渝
    ESP_COZE_USER_LANG_EN,          // 英语
    ESP_COZE_USER_LANG_JA,          // 日语
    ESP_COZE_USER_LANG_KO,          // 韩语
    ESP_COZE_USER_LANG_FR,          // 法语
    ESP_COZE_USER_LANG_ID,          // 印尼语
    ESP_COZE_USER_LANG_ES,          // 西班牙语
    ESP_COZE_USER_LANG_PT,          // 葡萄牙语
    ESP_COZE_USER_LANG_MS,          // 马来语
    ESP_COZE_USER_LANG_RU           // 俄语
} esp_coze_user_language_t;

/**
 * @brief 情感类型枚举
 */
typedef enum {
    ESP_COZE_EMOTION_HAPPY = 0,     // 开心
    ESP_COZE_EMOTION_SAD,           // 悲伤
    ESP_COZE_EMOTION_ANGRY,         // 生气
    ESP_COZE_EMOTION_SURPRISED,     // 惊讶
    ESP_COZE_EMOTION_FEAR,          // 恐惧
    ESP_COZE_EMOTION_HATE,          // 厌恶
    ESP_COZE_EMOTION_EXCITED,       // 激动
    ESP_COZE_EMOTION_COLDNESS,      // 冷漠
    ESP_COZE_EMOTION_NEUTRAL        // 中性
} esp_coze_emotion_type_t;

/**
 * @brief 限流配置结构体
 */
typedef struct {
    int period;             ///< 周期的时长，单位为秒
    int max_frame_num;      ///< 周期内最大返回帧数量
} esp_coze_limit_config_t;

/**
 * @brief PCM音频配置结构体
 */
typedef struct {
    int sample_rate;                        ///< 采样率
    float frame_size_ms;                    ///< 每个PCM包的时长，单位ms
    esp_coze_limit_config_t *limit_config;  ///< 限流配置
} esp_coze_pcm_config_t;

/**
 * @brief Opus音频配置结构体
 */
typedef struct {
    int bitrate;                            ///< 码率
    bool use_cbr;                           ///< 是否使用CBR编码
    float frame_size_ms;                    ///< 帧长
    int sample_rate;                        ///< 采样率
    esp_coze_limit_config_t *limit_config;  ///< 限流配置
} esp_coze_opus_config_t;

/**
 * @brief 输入音频配置结构体
 */
typedef struct {
    esp_coze_audio_format_t format;     ///< 音频格式
    esp_coze_audio_codec_t codec;       ///< 音频编码
    int sample_rate;                    ///< 采样率
    int channel;                        ///< 声道数
    int bit_depth;                      ///< 位深
} esp_coze_input_audio_config_t;

/**
 * @brief 情感配置结构体
 */
typedef struct {
    esp_coze_emotion_type_t emotion;    ///< 情感类型
    float emotion_scale;                ///< 情感值
} esp_coze_emotion_config_t;

/**
 * @brief 输出音频配置结构体
 */
typedef struct {
    esp_coze_audio_codec_t codec;           ///< 音频编码
    esp_coze_pcm_config_t *pcm_config;     ///< PCM配置
    esp_coze_opus_config_t *opus_config;   ///< Opus配置
    int speech_rate;                        ///< 语速
    char *voice_id;                         ///< 音色ID
    esp_coze_emotion_config_t *emotion_config; ///< 情感配置
} esp_coze_output_audio_config_t;

/**
 * @brief 打断配置结构体
 */
typedef struct {
    esp_coze_interrupt_mode_t mode;     ///< 打断模式
    char **keywords;                    ///< 关键词数组
    int keyword_count;                  ///< 关键词数量
} esp_coze_interrupt_config_t;

/**
 * @brief 转检测配置结构体
 */
typedef struct {
    esp_coze_turn_detection_type_t type;        ///< 检测类型
    int prefix_padding_ms;                      ///< VAD检测前包含的音频量
    int silence_duration_ms;                    ///< 静音持续时间
    esp_coze_interrupt_config_t *interrupt_config; ///< 打断配置
} esp_coze_turn_detection_config_t;

/**
 * @brief ASR配置结构体
 */
typedef struct {
    char **hot_words;                   ///< 热词列表
    int hot_word_count;                 ///< 热词数量
    char *context;                      ///< 上下文信息
    esp_coze_user_language_t user_language; ///< 用户语言
    bool enable_ddc;                    ///< 是否启用语义顺滑
    bool enable_itn;                    ///< 是否开启文本规范化
    bool enable_punc;                   ///< 是否添加标点符号
} esp_coze_asr_config_t;

/**
 * @brief 声纹配置结构体
 */
typedef struct {
    char *group_id;                     ///< 声纹组ID
    int score;                          ///< 匹配阈值
    bool reuse_voice_info;              ///< 是否沿用历史声纹信息
} esp_coze_voice_print_config_t;

/**
 * @brief 对话配置结构体
 */
typedef struct {
    cJSON *meta_data;                   ///< 附加信息
    cJSON *custom_variables;            ///< 智能体变量
    cJSON *extra_params;                ///< 附加参数
    char *user_id;                      ///< 用户ID
    char *conversation_id;              ///< 会话ID
    bool auto_save_history;             ///< 是否保存对话记录
    cJSON *parameters;                  ///< 对话流自定义参数
} esp_coze_chat_config_data_t;

/**
 * @brief 完整的会话配置结构体
 */
typedef struct {
    esp_coze_chat_config_data_t *chat_config;              ///< 对话配置
    esp_coze_input_audio_config_t *input_audio;            ///< 输入音频配置
    esp_coze_output_audio_config_t *output_audio;          ///< 输出音频配置
    char **event_subscriptions;                            ///< 事件订阅列表
    int subscription_count;                                 ///< 订阅数量
    bool need_play_prologue;                                ///< 是否播放开场白
    char *prologue_content;                                 ///< 自定义开场白
    esp_coze_turn_detection_config_t *turn_detection;      ///< 转检测配置
    esp_coze_asr_config_t *asr_config;                     ///< ASR配置
    esp_coze_voice_print_config_t *voice_print_config;     ///< 声纹配置
} esp_coze_session_config_t;

/**
 * @brief chat.update事件数据结构体
 */
typedef struct {
    char *id;                                   ///< 事件ID
    char *event_type;                           ///< 事件类型，固定为"chat.update"
    esp_coze_session_config_t *data;            ///< 事件数据
} esp_coze_chat_update_event_t;

/**
 * @brief 创建会话配置JSON对象
 *
 * @param session_config 会话配置
 * @return cJSON* JSON对象指针，使用完毕后需要调用cJSON_Delete释放
 */
cJSON *esp_coze_create_session_config_json(const esp_coze_session_config_t *session_config);

/**
 * @brief 创建chat.update事件JSON
 *
 * @param event 事件结构体
 * @return cJSON* JSON对象指针，使用完毕后需要调用cJSON_Delete释放
 */
cJSON *esp_coze_create_chat_update_event_json(const esp_coze_chat_update_event_t *event);

/**
 * @brief 生成UUID字符串作为事件ID
 *
 * @param event_id 输出的事件ID字符串缓冲区
 * @param max_len 缓冲区最大长度（至少37字节）
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t esp_coze_generate_event_id(char *event_id, size_t max_len);

#ifdef __cplusplus
}
#endif
