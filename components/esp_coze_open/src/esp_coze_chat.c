/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-27 21:30:00
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-27 20:11:30
 * @FilePath: \esp-brookesia-chunfeng\components\esp_coze_open\src\esp_coze_chat.c
 * @Description: 扣子聊天客户端实现 - 环形缓冲区版本
 *
*/
#include "esp_coze_chat.h"
#include "esp_coze_ring_buffer.h"
#include "cJSON.h"
#include "mbedtls/base64.h"
#include "esp_coze_events.h"
#include "esp_coze_chat_config.h"

static const char *TAG = "ESP_COZE_CHAT";

static esp_coze_chat_handle_t *g_coze_handle = NULL;
static esp_coze_ring_buffer_t g_ring_buffer = {0};
static TaskHandle_t g_parser_task_handle = NULL;

// 数据解析任务栈 - 放在PSRAM
#define DATA_PARSER_STACK_SIZE (8192 / sizeof(StackType_t))
static EXT_RAM_BSS_ATTR StackType_t data_parser_stack[DATA_PARSER_STACK_SIZE];
static StaticTask_t data_parser_task_buffer;
static bool g_parser_running = false;

// 弱实现，应用层可覆盖
__attribute__((weak)) void esp_coze_on_pcm_audio(const int16_t *pcm, size_t sample_count)
{
    (void)pcm; (void)sample_count;
}

/**
* @brief 数据解析任务
*/
static void data_parser_task(void *param)
{
    uint8_t json_buffer[4096];
    size_t json_len;

    ESP_LOGI(TAG, "数据解析任务启动");

    while (g_parser_running) {
        // 等待数据信号
        if (xSemaphoreTake(g_ring_buffer.data_sem, pdMS_TO_TICKS(100)) == pdTRUE) {
            // 连续处理所有可用的JSON对象
            while (esp_coze_ring_buffer_read_json_object(&g_ring_buffer, json_buffer, sizeof(json_buffer), &json_len) == ESP_OK) {
                // ESP_LOGI(TAG, "解析JSON对象，长度: %d", (int)json_len);
                // ESP_LOGI(TAG, "JSON内容: %.*s", (int)json_len > 200 ? 200 : (int)json_len, (char*)json_buffer);

                // 解析JSON
                cJSON *json = cJSON_ParseWithLength((char *)json_buffer, json_len);
                if (json) {
                    cJSON *event_type_item = cJSON_GetObjectItem(json, "event_type");
                    if (event_type_item && cJSON_IsString(event_type_item)) {
                        const char *event_type = cJSON_GetStringValue(event_type_item);

                        if (strcmp(event_type, "conversation.audio.delta") == 0) {
                            // 处理音频数据：content 为 base64 编码的 PCM 16-bit 单声道
                            cJSON *data_item = cJSON_GetObjectItem(json, "data");
                            if (data_item) {
                                cJSON *content_item = cJSON_GetObjectItem(data_item, "content");
                                if (content_item && cJSON_IsString(content_item)) {
                                    const char *audio_base64 = cJSON_GetStringValue(content_item);
                                    if (audio_base64) {
                                        size_t b64_len = strlen(audio_base64);
                                        size_t raw_len = (b64_len * 3) / 4 + 4;
                                        uint8_t *raw = (uint8_t *)malloc(raw_len);
                                        if (raw) {
                                            size_t out_len = 0;
                                            int ret = mbedtls_base64_decode(raw, raw_len, &out_len,
                                                                            (const unsigned char *)audio_base64, b64_len);
                                            if (ret == 0 && out_len >= 2) {
                                                // 假定小端16位PCM
                                                size_t samples = out_len / 2;
                                                esp_coze_on_pcm_audio((const int16_t *)raw, samples);
                                            } else {
                                                ESP_LOGW(TAG, "Base64解码失败 ret=%d", ret);
                                            }
                                            free(raw);
                                        }
                                    }
                                }
                            }
                        } else {
                            // 其他事件打印详情
                            char *json_string = cJSON_Print(json);
                            if (json_string) {
                                ESP_LOGI(TAG, "收到事件: %s", event_type);
                                ESP_LOGI(TAG, "事件详情: %s", json_string);
                                free(json_string);
                            } else {
                                ESP_LOGI(TAG, "收到事件: %s (无法格式化详情)", event_type);
                            }
                        }
                    }
                    cJSON_Delete(json);
                } else {
                    ESP_LOGW(TAG, "JSON解析失败: %.*s", (int)json_len > 100 ? 100 : (int)json_len, (char *)json_buffer);
                }
            } // end while (连续处理JSON对象)
        } else {
            // 没有接收到信号，但检查是否有剩余数据
            if (esp_coze_ring_buffer_available(&g_ring_buffer) > 0) {
                vTaskDelay(pdMS_TO_TICKS(5)); // 短暂等待更多数据
            }
        }
    }

    ESP_LOGI(TAG, "数据解析任务退出");
    g_parser_task_handle = NULL;
    vTaskDelete(NULL);
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
        config->asr_config->hot_words = calloc(2, sizeof(char *));
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
* @brief WebSocket事件处理回调函数
*/
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    esp_coze_chat_handle_t *handle = (esp_coze_chat_handle_t *)handler_args;

    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WebSocket连接成功");
        if (handle) {
            handle->ws_state = ESP_COZE_WS_STATE_CONNECTED;
            // 会话配置
            example_send_custom_chat_update();
        }
        break;

    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WebSocket连接断开");
        if (handle) {
            handle->ws_state = ESP_COZE_WS_STATE_DISCONNECTED;
        }
        break;

    case WEBSOCKET_EVENT_DATA:
        // ESP_LOGI(TAG, "WebSocket接收到数据，长度: %d", data->data_len);
        if (data->data_ptr && data->data_len > 0) {
            // 直接写入环形缓冲区
            esp_err_t ret = esp_coze_ring_buffer_write(&g_ring_buffer, (uint8_t *)data->data_ptr, data->data_len);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "写入环形缓冲区失败: %s", esp_err_to_name(ret));
            }
        }
        break;

    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGE(TAG, "WebSocket连接错误");
        if (handle) {
            handle->ws_state = ESP_COZE_WS_STATE_ERROR;
        }
        break;

    case WEBSOCKET_EVENT_BEFORE_CONNECT:
        ESP_LOGI(TAG, "WebSocket准备连接");
        if (handle) {
            handle->ws_state = ESP_COZE_WS_STATE_CONNECTING;
        }
        break;

    default:
        ESP_LOGD(TAG, "WebSocket其他事件: %d", (int)event_id);
        break;
    }
}

/**
* @brief 使用自定义参数初始化扣子聊天客户端
*/
esp_err_t esp_coze_chat_init_with_config(const esp_coze_chat_config_t *config)
{
    // 参数校验
    if (config == NULL || config->ws_base_url == NULL || config->access_token == NULL ||
            config->bot_id == NULL || config->device_id == NULL) {
        ESP_LOGE(TAG, "配置参数不能为空");
        return ESP_ERR_INVALID_ARG;
    }

    // 如果已经初始化过，先销毁再重新初始化
    if (g_coze_handle != NULL) {
        ESP_LOGI(TAG, "扣子聊天客户端已初始化，重新初始化");
        esp_coze_chat_destroy();
    }

    // 分配句柄内存
    g_coze_handle = calloc(1, sizeof(esp_coze_chat_handle_t));
    if (g_coze_handle == NULL) {
        ESP_LOGE(TAG, "分配句柄内存失败");
        return ESP_ERR_NO_MEM;
    }

    // 构建完整的WebSocket URL，包含查询参数
    size_t url_len = strlen(config->ws_base_url) + strlen("?bot_id=") + strlen(config->bot_id) +
                     strlen("&device_id=") + strlen(config->device_id) + 1;
    g_coze_handle->ws_url = malloc(url_len);
    if (g_coze_handle->ws_url) {
        snprintf(g_coze_handle->ws_url, url_len, "%s?bot_id=%s&device_id=%s",
                 config->ws_base_url, config->bot_id, config->device_id);
    }

    // 设置参数
    g_coze_handle->access_token = strdup(config->access_token);
    g_coze_handle->bot_id = strdup(config->bot_id);
    g_coze_handle->device_id = strdup(config->device_id);
    g_coze_handle->conversation_id = config->conversation_id ? strdup(config->conversation_id) : strdup(ESP_COZE_DEFAULT_CONVERSATION_ID);

    // 构建Authorization请求头
    size_t auth_header_len = strlen("Authorization: Bearer ") + strlen(config->access_token) + 3;
    g_coze_handle->auth_header = malloc(auth_header_len);
    if (g_coze_handle->auth_header) {
        snprintf(g_coze_handle->auth_header, auth_header_len, "Authorization: Bearer %s\r\n", config->access_token);
    }

    // 检查内存分配是否成功
    if (!g_coze_handle->ws_url || !g_coze_handle->access_token ||
            !g_coze_handle->bot_id || !g_coze_handle->device_id ||
            !g_coze_handle->conversation_id || !g_coze_handle->auth_header) {
        ESP_LOGE(TAG, "字符串内存分配失败");
        goto cleanup;
    }

    // 准备WebSocket配置，包含Authorization请求头
    esp_websocket_client_config_t ws_cfg = {
        .uri = g_coze_handle->ws_url,
        .headers = g_coze_handle->auth_header,
        .task_stack = 4096,
        .task_prio = 5,
        .reconnect_timeout_ms = 10000,    // 重连超时10秒
        .network_timeout_ms = 10000,      // 网络超时10秒
    };

    // 初始化WebSocket客户端
    g_coze_handle->ws_client = esp_websocket_client_init(&ws_cfg);
    if (g_coze_handle->ws_client == NULL) {
        ESP_LOGE(TAG, "初始化WebSocket客户端失败");
        goto cleanup;
    }

    // 注册WebSocket事件处理器
    esp_websocket_register_events(g_coze_handle->ws_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, g_coze_handle);

    // 初始化环形缓冲区
    esp_err_t ret = esp_coze_ring_buffer_init(&g_ring_buffer, RING_BUFFER_SIZE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "初始化环形缓冲区失败");
        goto cleanup;
    }

    // 启动数据解析任务 - 使用静态任务，栈在PSRAM
    g_parser_running = true;
    g_parser_task_handle = xTaskCreateStatic(
        data_parser_task,           // 任务函数
        "data_parser",              // 任务名称
        DATA_PARSER_STACK_SIZE,     // 栈大小
        NULL,                       // 任务参数
        5,                          // 优先级
        data_parser_stack,          // 栈数组(PSRAM)
        &data_parser_task_buffer    // 任务控制块(内部RAM)
    );
    
    if (g_parser_task_handle == NULL) {
        ESP_LOGE(TAG, "创建数据解析任务失败");
        g_parser_running = false;
        goto cleanup;
    }

    // 初始化连接状态
    g_coze_handle->ws_state = ESP_COZE_WS_STATE_DISCONNECTED;

    ESP_LOGI(TAG, "扣子聊天客户端初始化成功");
    return ESP_OK;

cleanup:
    // 清理资源
    if (g_coze_handle) {
        if (g_coze_handle->ws_url) free(g_coze_handle->ws_url);
        if (g_coze_handle->access_token) free(g_coze_handle->access_token);
        if (g_coze_handle->bot_id) free(g_coze_handle->bot_id);
        if (g_coze_handle->device_id) free(g_coze_handle->device_id);
        if (g_coze_handle->conversation_id) free(g_coze_handle->conversation_id);
        if (g_coze_handle->auth_header) free(g_coze_handle->auth_header);
        free(g_coze_handle);
        g_coze_handle = NULL;
    }
    esp_coze_ring_buffer_deinit(&g_ring_buffer);
    return ESP_ERR_NO_MEM;
}

/**
* @brief 启动扣子聊天服务（仅连接，需要先初始化）
*/
esp_err_t esp_coze_chat_start()
{
    // 检查是否已经初始化
    if (g_coze_handle == NULL) {
        ESP_LOGE(TAG, "扣子聊天客户端未初始化，请先调用esp_coze_chat_init()或esp_coze_chat_init_with_config()");
        return ESP_ERR_INVALID_STATE;
    }

    // 启动WebSocket连接
    esp_err_t ret = esp_coze_chat_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "连接WebSocket服务器失败");
        return ret;
    }

    ESP_LOGI(TAG, "扣子聊天服务启动成功");
    return ESP_OK;
}

/**
* @brief 销毁扣子聊天客户端
*/
esp_err_t esp_coze_chat_destroy()
{
    if (g_coze_handle == NULL) {
        ESP_LOGW(TAG, "扣子聊天客户端未初始化");
        return ESP_OK;
    }

    // 停止数据解析任务
    g_parser_running = false;
    if (g_parser_task_handle) {
        int retry = 100;
        while (g_parser_task_handle && retry-- > 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    // 销毁WebSocket客户端
    if (g_coze_handle->ws_client) {
        esp_websocket_client_destroy(g_coze_handle->ws_client);
    }

    // 释放字符串内存
    if (g_coze_handle->ws_url) free(g_coze_handle->ws_url);
    if (g_coze_handle->access_token) free(g_coze_handle->access_token);
    if (g_coze_handle->bot_id) free(g_coze_handle->bot_id);
    if (g_coze_handle->device_id) free(g_coze_handle->device_id);
    if (g_coze_handle->conversation_id) free(g_coze_handle->conversation_id);
    if (g_coze_handle->auth_header) free(g_coze_handle->auth_header);

    // 销毁环形缓冲区
    esp_coze_ring_buffer_deinit(&g_ring_buffer);

    // 释放句柄内存
    free(g_coze_handle);
    g_coze_handle = NULL;

    ESP_LOGI(TAG, "扣子聊天客户端销毁成功");
    return ESP_OK;
}

/**
* @brief 连接到扣子WebSocket服务器
*/
esp_err_t esp_coze_chat_connect()
{
    if (g_coze_handle == NULL || g_coze_handle->ws_client == NULL) {
        ESP_LOGE(TAG, "扣子聊天客户端未初始化");
        return ESP_FAIL;
    }

    esp_err_t ret = esp_websocket_client_start(g_coze_handle->ws_client);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "开始连接WebSocket服务器");
        g_coze_handle->ws_state = ESP_COZE_WS_STATE_CONNECTING;
    } else {
        ESP_LOGE(TAG, "启动WebSocket连接失败");
        g_coze_handle->ws_state = ESP_COZE_WS_STATE_ERROR;
    }

    return ret;
}

/**
* @brief 断开WebSocket连接
*/
esp_err_t esp_coze_chat_disconnect()
{
    if (g_coze_handle == NULL || g_coze_handle->ws_client == NULL) {
        ESP_LOGE(TAG, "扣子聊天客户端未初始化");
        return ESP_FAIL;
    }

    esp_err_t ret = esp_websocket_client_stop(g_coze_handle->ws_client);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "断开WebSocket连接");
        g_coze_handle->ws_state = ESP_COZE_WS_STATE_DISCONNECTED;
    } else {
        ESP_LOGE(TAG, "断开WebSocket连接失败");
    }

    return ret;
}

/**
* @brief 通过WebSocket发送文本消息
*/
esp_err_t esp_coze_websocket_send_text(const char *data)
{
    if (g_coze_handle == NULL || g_coze_handle->ws_client == NULL) {
        ESP_LOGE(TAG, "WebSocket客户端未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    if (!data) {
        ESP_LOGE(TAG, "发送数据不能为空");
        return ESP_ERR_INVALID_ARG;
    }

    int len = esp_websocket_client_send_text(g_coze_handle->ws_client, data, strlen(data), portMAX_DELAY);
    if (len < 0) {
        ESP_LOGE(TAG, "发送WebSocket文本消息失败");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "成功发送WebSocket文本消息，长度: %d", len);
    return ESP_OK;
}

/**
* @brief 通过WebSocket发送二进制消息
*/
esp_err_t esp_coze_websocket_send_binary(const uint8_t *data, size_t len)
{
    if (g_coze_handle == NULL || g_coze_handle->ws_client == NULL) {
        ESP_LOGE(TAG, "WebSocket客户端未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    if (!data || len == 0) {
        ESP_LOGE(TAG, "发送数据不能为空");
        return ESP_ERR_INVALID_ARG;
    }

    int sent_len = esp_websocket_client_send_bin(g_coze_handle->ws_client, (char *)data, len, portMAX_DELAY);
    if (sent_len < 0) {
        ESP_LOGE(TAG, "发送WebSocket二进制消息失败");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "成功发送WebSocket二进制消息，长度: %d", sent_len);
    return ESP_OK;
}
