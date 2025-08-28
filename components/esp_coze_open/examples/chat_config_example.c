/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-27 17:00:00
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-27 17:00:00
 * @FilePath: \esp-brookesia-chunfeng\components\esp_coze_open\examples\chat_config_example.c
 * @Description: 扣子聊天会话配置使用示例
 *
 */
#include "esp_coze_chat.h"
#include "esp_coze_events.h"
#include "esp_coze_chat_config.h"
#include "esp_log.h"

static const char *TAG = "COZE_CONFIG_EXAMPLE";

/**
 * @brief 示例1：使用默认配置发送chat.update事件
 */
void example_send_default_chat_update(void)
{
    ESP_LOGI(TAG, "=== 示例1: 使用默认配置发送chat.update事件 ===");

    // 创建默认会话配置
    esp_coze_session_config_t *config = esp_coze_create_default_session_config();
    if (!config) {
        ESP_LOGE(TAG, "创建默认会话配置失败");
        return;
    }

    // 发送chat.update事件
    esp_err_t ret = esp_coze_send_chat_update_event(config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "发送默认chat.update事件成功");
    } else {
        ESP_LOGE(TAG, "发送默认chat.update事件失败: %s", esp_err_to_name(ret));
    }

    // 释放内存
    esp_coze_free_session_config(config);
}

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
    config->chat_config = esp_coze_create_simple_chat_config("user123", "conv456", true);

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
        config->output_audio->voice_id = strdup("voice_001");

        // PCM配置
        config->output_audio->pcm_config = calloc(1, sizeof(esp_coze_pcm_config_t));
        if (config->output_audio->pcm_config) {
            config->output_audio->pcm_config->sample_rate = 24000;
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
        config->asr_config->hot_words = calloc(2, sizeof(char *));
        if (config->asr_config->hot_words) {
            config->asr_config->hot_words[0] = strdup("扣子");
            config->asr_config->hot_words[1] = strdup("人工智能");
        }

        config->asr_config->context = strdup("这是一个关于人工智能的对话");
    }

    // 设置开场白
    config->need_play_prologue = true;
    config->prologue_content = strdup("你好，我是扣子智能助手，很高兴为您服务！");

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
 * @brief 示例3：创建包含元数据的对话配置
 */
void example_create_chat_config_with_metadata(void)
{
    ESP_LOGI(TAG, "=== 示例3: 创建包含元数据的对话配置 ===");

    esp_coze_session_config_t *config = calloc(1, sizeof(esp_coze_session_config_t));
    if (!config) {
        ESP_LOGE(TAG, "分配会话配置内存失败");
        return;
    }

    // 创建对话配置
    config->chat_config = calloc(1, sizeof(esp_coze_chat_config_data_t));
    if (config->chat_config) {
        config->chat_config->user_id = strdup("user789");
        config->chat_config->conversation_id = strdup("conv123");
        config->chat_config->auto_save_history = true;

        // 添加元数据
        config->chat_config->meta_data = cJSON_CreateObject();
        if (config->chat_config->meta_data) {
            cJSON_AddStringToObject(config->chat_config->meta_data, "client_type", "esp32");
            cJSON_AddStringToObject(config->chat_config->meta_data, "version", "1.0.0");
            cJSON_AddStringToObject(config->chat_config->meta_data, "device_id", "esp32_001");
        }

        // 添加自定义变量
        config->chat_config->custom_variables = cJSON_CreateObject();
        if (config->chat_config->custom_variables) {
            cJSON_AddStringToObject(config->chat_config->custom_variables, "user_name", "张三");
            cJSON_AddStringToObject(config->chat_config->custom_variables, "location", "北京");
        }

        // 添加额外参数
        config->chat_config->extra_params = cJSON_CreateObject();
        if (config->chat_config->extra_params) {
            cJSON_AddStringToObject(config->chat_config->extra_params, "latitude", "39.9042");
            cJSON_AddStringToObject(config->chat_config->extra_params, "longitude", "116.4074");
        }
    }

    // 发送事件
    esp_err_t ret = esp_coze_send_chat_update_event(config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "发送包含元数据的chat.update事件成功");
    } else {
        ESP_LOGE(TAG, "发送包含元数据的chat.update事件失败: %s", esp_err_to_name(ret));
    }

    // 释放内存
    esp_coze_free_session_config(config);
}

/**
 * @brief 完整的使用示例
 */
void coze_chat_config_examples(void)
{
    ESP_LOGI(TAG, "开始扣子聊天配置示例");

    // 初始化扣子聊天客户端（假设已经有自定义配置）
    esp_coze_chat_config_t chat_config = {
        .ws_base_url = "wss://ws.coze.cn/v1/chat",
        .access_token = "your_access_token",
        .bot_id = "your_bot_id",
        .device_id = "esp32_device_001",
        .conversation_id = NULL
    };

    esp_err_t ret = esp_coze_chat_init_with_config(&chat_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "初始化扣子聊天客户端失败: %s", esp_err_to_name(ret));
        return;
    }

    // 启动连接
    ret = esp_coze_chat_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "启动扣子聊天服务失败: %s", esp_err_to_name(ret));
        esp_coze_chat_destroy();
        return;
    }

    // 等待连接建立（实际应用中可能需要更复杂的状态检查）
    vTaskDelay(pdMS_TO_TICKS(2000));

    // 运行示例
    example_send_default_chat_update();
    vTaskDelay(pdMS_TO_TICKS(1000));

    example_send_custom_chat_update();
    vTaskDelay(pdMS_TO_TICKS(1000));

    example_create_chat_config_with_metadata();

    ESP_LOGI(TAG, "扣子聊天配置示例完成");

    // 清理
    esp_coze_chat_disconnect();
    esp_coze_chat_destroy();
}
