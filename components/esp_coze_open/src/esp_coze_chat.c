/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "esp_websocket_client.h"
#include "cJSON.h"
#include "esp_coze_chat.h"

static const char *TAG = "ESP_COZE_CHAT";

#define ESP_COZE_DEFAULT_WS_URL "wss://ws.coze.cn/v1/chat"
#define ESP_COZE_DEFAULT_TIMEOUT_MS 10000
#define ESP_COZE_DEFAULT_KEEPALIVE_MS 30000
#define ESP_COZE_MAX_RECONNECT_ATTEMPTS 5
#define ESP_COZE_RECONNECT_DELAY_MS 1000
#define ESP_COZE_MAX_MESSAGE_SIZE 4096
#define ESP_COZE_AUDIO_CHUNK_SIZE 1024

/**
 * @brief WebSocket连接状态
 */
typedef enum {
    ESP_COZE_WS_STATE_DISCONNECTED = 0,
    ESP_COZE_WS_STATE_CONNECTING,
    ESP_COZE_WS_STATE_CONNECTED,
    ESP_COZE_WS_STATE_ERROR,
} esp_coze_ws_state_t;

/**
 * @brief 内部消息类型
 */
typedef enum {
    ESP_COZE_MSG_TYPE_AUDIO = 0,
    ESP_COZE_MSG_TYPE_TEXT,
    ESP_COZE_MSG_TYPE_CONTROL,
    ESP_COZE_MSG_TYPE_HEARTBEAT,
} esp_coze_msg_type_t;

/**
 * @brief 扣子聊天句柄结构
 */
struct esp_coze_chat_handle_s {
    esp_websocket_client_handle_t ws_client;
    esp_coze_chat_config_t config;
    esp_coze_ws_state_t ws_state;
    esp_coze_session_info_t session_info;
    esp_coze_stats_t stats;
    
    SemaphoreHandle_t mutex;
    QueueHandle_t event_queue;
    TaskHandle_t event_task_handle;
    TaskHandle_t heartbeat_task_handle;
    esp_timer_handle_t reconnect_timer;
    
    bool is_speech_active;
    bool is_conversation_active;
    uint8_t reconnect_attempts;
    uint64_t session_start_time;
    uint64_t last_heartbeat_time;
    
    char *ws_url;
    char *full_ws_url;
    char *auth_header;  // 存储认证头，以便后续释放
};

/**
 * @brief 内部事件结构
 */
typedef struct {
    esp_coze_event_data_t event_data;
    char *allocated_message;
    uint8_t *allocated_data;
} esp_coze_internal_event_t;

static esp_log_level_t s_log_level = ESP_LOG_INFO;

// 前向声明
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static void event_task(void *param);
static void heartbeat_task(void *param);
static void reconnect_timer_callback(void *arg);
static esp_err_t send_websocket_message(esp_coze_chat_handle_t handle, const char *message);
static esp_err_t send_audio_message(esp_coze_chat_handle_t handle, const uint8_t *audio_data, size_t data_len);
static esp_err_t send_control_message(esp_coze_chat_handle_t handle, const char *action, const char *params);
static esp_err_t build_websocket_url(esp_coze_chat_handle_t handle);
static void post_event(esp_coze_chat_handle_t handle, esp_coze_event_type_t event_type, void *data, size_t data_len, const char *message, int error_code);
static uint64_t get_timestamp_ms(void);
static esp_err_t validate_config(const esp_coze_chat_config_t *config);
static void free_internal_event(esp_coze_internal_event_t *internal_event);

/**
 * @brief 获取当前时间戳（毫秒）
 */
static uint64_t get_timestamp_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/**
 * @brief 验证配置参数
 */
static esp_err_t validate_config(const esp_coze_chat_config_t *config)
{
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "配置不能为空");
    ESP_RETURN_ON_FALSE(config->access_token != NULL && strlen(config->access_token) > 0, ESP_ERR_INVALID_ARG, TAG, "访问令牌不能为空");
    ESP_RETURN_ON_FALSE(config->bot_id != NULL && strlen(config->bot_id) > 0, ESP_ERR_INVALID_ARG, TAG, "智能体ID不能为空");

    return ESP_OK;
}

/**
 * @brief 构建WebSocket URL
 */
static esp_err_t build_websocket_url(esp_coze_chat_handle_t handle)
{
    if (handle->full_ws_url) {
        free(handle->full_ws_url);
        handle->full_ws_url = NULL;
    }

    // 计算URL长度
    size_t url_len = strlen(handle->ws_url) + strlen("?bot_id=") + strlen(handle->config.bot_id) + 1;

    if (handle->config.workflow_id) {
        url_len += strlen("&workflow_id=") + strlen(handle->config.workflow_id);
    }

    if (handle->config.device_id) {
        url_len += strlen("&device_id=") + strlen(handle->config.device_id);
    }

    if (handle->config.conversation_id) {
        url_len += strlen("&conversation_id=") + strlen(handle->config.conversation_id);
    }

    handle->full_ws_url = malloc(url_len);
    ESP_RETURN_ON_FALSE(handle->full_ws_url != NULL, ESP_ERR_NO_MEM, TAG, "分配URL内存失败");

    // 构建URL
    snprintf(handle->full_ws_url, url_len, "%s?bot_id=%s", handle->ws_url, handle->config.bot_id);

    if (handle->config.workflow_id) {
        strcat(handle->full_ws_url, "&workflow_id=");
        strcat(handle->full_ws_url, handle->config.workflow_id);
    }

    if (handle->config.device_id) {
        strcat(handle->full_ws_url, "&device_id=");
        strcat(handle->full_ws_url, handle->config.device_id);
    }

    if (handle->config.conversation_id) {
        strcat(handle->full_ws_url, "&conversation_id=");
        strcat(handle->full_ws_url, handle->config.conversation_id);
    }

    ESP_LOGI(TAG, "构建WebSocket URL: %s", handle->full_ws_url);
    return ESP_OK;
}

/**
 * @brief 发送WebSocket消息
 */
static esp_err_t send_websocket_message(esp_coze_chat_handle_t handle, const char *message)
{
    ESP_RETURN_ON_FALSE(handle != NULL && message != NULL, ESP_ERR_INVALID_ARG, TAG, "参数不能为空");
    ESP_RETURN_ON_FALSE(handle->ws_state == ESP_COZE_WS_STATE_CONNECTED, ESP_FAIL, TAG, "WebSocket未连接");

    int ret = esp_websocket_client_send_text(handle->ws_client, message, strlen(message), portMAX_DELAY);
    if (ret < 0) {
        ESP_LOGE(TAG, "发送WebSocket消息失败");
        return ESP_FAIL;
    }

    handle->stats.total_messages_sent++;
    ESP_LOGD(TAG, "发送消息: %s", message);
    return ESP_OK;
}

/**
 * @brief 发送音频消息
 */
static esp_err_t send_audio_message(esp_coze_chat_handle_t handle, const uint8_t *audio_data, size_t data_len)
{
    ESP_RETURN_ON_FALSE(handle != NULL && audio_data != NULL && data_len > 0, ESP_ERR_INVALID_ARG, TAG, "参数不能为空");
    ESP_RETURN_ON_FALSE(handle->ws_state == ESP_COZE_WS_STATE_CONNECTED, ESP_FAIL, TAG, "WebSocket未连接");

    // 创建音频消息JSON
    cJSON *json = cJSON_CreateObject();
    cJSON *type = cJSON_CreateString("audio");
    cJSON *data = cJSON_CreateString(""); // 这里需要将二进制数据编码为base64

    cJSON_AddItemToObject(json, "type", type);
    cJSON_AddItemToObject(json, "data", data);

    char *json_string = cJSON_Print(json);
    esp_err_t ret = send_websocket_message(handle, json_string);

    free(json_string);
    cJSON_Delete(json);

    if (ret == ESP_OK) {
        handle->stats.total_audio_bytes_sent += data_len;
    }

    return ret;
}

/**
 * @brief 发送控制消息
 */
static esp_err_t send_control_message(esp_coze_chat_handle_t handle, const char *action, const char *params)
{
    ESP_RETURN_ON_FALSE(handle != NULL && action != NULL, ESP_ERR_INVALID_ARG, TAG, "参数不能为空");

    cJSON *json = cJSON_CreateObject();
    cJSON *type = cJSON_CreateString("control");
    cJSON *action_item = cJSON_CreateString(action);

    cJSON_AddItemToObject(json, "type", type);
    cJSON_AddItemToObject(json, "action", action_item);

    if (params) {
        cJSON *params_item = cJSON_CreateString(params);
        cJSON_AddItemToObject(json, "params", params_item);
    }

    char *json_string = cJSON_Print(json);
    esp_err_t ret = send_websocket_message(handle, json_string);

    free(json_string);
    cJSON_Delete(json);

    return ret;
}

/**
 * @brief 发送事件到队列
 */
static void post_event(esp_coze_chat_handle_t handle, esp_coze_event_type_t event_type,
                       void *data, size_t data_len, const char *message, int error_code)
{
    if (!handle || !handle->event_queue) {
        return;
    }

    esp_coze_internal_event_t *internal_event = malloc(sizeof(esp_coze_internal_event_t));
    if (!internal_event) {
        ESP_LOGE(TAG, "分配事件内存失败");
        return;
    }

    memset(internal_event, 0, sizeof(esp_coze_internal_event_t));
    internal_event->event_data.event_type = event_type;
    internal_event->event_data.error_code = error_code;

    // 复制数据
    if (data && data_len > 0) {
        internal_event->allocated_data = malloc(data_len);
        if (internal_event->allocated_data) {
            memcpy(internal_event->allocated_data, data, data_len);
            internal_event->event_data.data = internal_event->allocated_data;
            internal_event->event_data.data_len = data_len;
        }
    }

    // 复制消息
    if (message) {
        internal_event->allocated_message = strdup(message);
        internal_event->event_data.message = internal_event->allocated_message;
    }

    if (xQueueSend(handle->event_queue, &internal_event, 0) != pdTRUE) {
        ESP_LOGW(TAG, "事件队列已满，丢弃事件");
        free_internal_event(internal_event);
    }
}

/**
 * @brief 释放内部事件
 */
static void free_internal_event(esp_coze_internal_event_t *internal_event)
{
    if (internal_event) {
        if (internal_event->allocated_data) {
            free(internal_event->allocated_data);
        }
        if (internal_event->allocated_message) {
            free(internal_event->allocated_message);
        }
        free(internal_event);
    }
}

/**
 * @brief WebSocket事件处理器
 */
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_coze_chat_handle_t handle = (esp_coze_chat_handle_t)handler_args;
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WebSocket连接成功");
        xSemaphoreTake(handle->mutex, portMAX_DELAY);
        handle->ws_state = ESP_COZE_WS_STATE_CONNECTED;
        handle->reconnect_attempts = 0;
        handle->stats.connection_count++;
        handle->session_start_time = get_timestamp_ms();
        xSemaphoreGive(handle->mutex);
        post_event(handle, ESP_COZE_EVENT_CONNECTED, NULL, 0, NULL, 0);
        break;

    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WebSocket连接断开");
        xSemaphoreTake(handle->mutex, portMAX_DELAY);
        handle->ws_state = ESP_COZE_WS_STATE_DISCONNECTED;
        handle->is_speech_active = false;
        handle->is_conversation_active = false;
        xSemaphoreGive(handle->mutex);
        post_event(handle, ESP_COZE_EVENT_DISCONNECTED, NULL, 0, NULL, 0);

        // 自动重连
        if (handle->config.auto_reconnect && handle->reconnect_attempts < handle->config.max_reconnect_attempts) {
            ESP_LOGI(TAG, "尝试重连 (%d/%d)", handle->reconnect_attempts + 1, handle->config.max_reconnect_attempts);
            esp_timer_start_once(handle->reconnect_timer, ESP_COZE_RECONNECT_DELAY_MS * 1000);
        }
        break;

    case WEBSOCKET_EVENT_DATA:
        if (data->op_code == 0x01) { // 文本数据
            ESP_LOGD(TAG, "收到WebSocket数据: %.*s", data->data_len, (char *)data->data_ptr);
            handle->stats.total_messages_received++;

            // 解析JSON消息
            cJSON *json = cJSON_ParseWithLength((char *)data->data_ptr, data->data_len);
            if (json) {
                cJSON *type = cJSON_GetObjectItem(json, "type");
                                    // cJSON *event = cJSON_GetObjectItem(json, "event"); // 暂时未使用

                if (type && cJSON_IsString(type)) {
                    const char *type_str = cJSON_GetStringValue(type);

                    if (strcmp(type_str, "conversation.start") == 0) {
                        handle->is_conversation_active = true;
                        post_event(handle, ESP_COZE_EVENT_CONVERSATION_START, NULL, 0, NULL, 0);
                    } else if (strcmp(type_str, "conversation.end") == 0) {
                        handle->is_conversation_active = false;
                        post_event(handle, ESP_COZE_EVENT_CONVERSATION_END, NULL, 0, NULL, 0);
                    } else if (strcmp(type_str, "audio.start") == 0) {
                        post_event(handle, ESP_COZE_EVENT_AUDIO_START, NULL, 0, NULL, 0);
                    } else if (strcmp(type_str, "audio.end") == 0) {
                        post_event(handle, ESP_COZE_EVENT_AUDIO_END, NULL, 0, NULL, 0);
                    } else if (strcmp(type_str, "speech.start") == 0) {
                        handle->is_speech_active = true;
                        post_event(handle, ESP_COZE_EVENT_SPEECH_START, NULL, 0, NULL, 0);
                    } else if (strcmp(type_str, "speech.end") == 0) {
                        handle->is_speech_active = false;
                        post_event(handle, ESP_COZE_EVENT_SPEECH_END, NULL, 0, NULL, 0);
                    } else if (strcmp(type_str, "message") == 0) {
                        cJSON *content = cJSON_GetObjectItem(json, "content");
                        if (content && cJSON_IsString(content)) {
                            post_event(handle, ESP_COZE_EVENT_MESSAGE_RECEIVED, NULL, 0,
                                       cJSON_GetStringValue(content), 0);
                        }
                    } else if (strcmp(type_str, "asr.result") == 0) {
                        cJSON *result = cJSON_GetObjectItem(json, "result");
                        if (result && cJSON_IsString(result)) {
                            post_event(handle, ESP_COZE_EVENT_ASR_RESULT, NULL, 0,
                                       cJSON_GetStringValue(result), 0);
                        }
                    } else if (strcmp(type_str, "tts.audio") == 0) {
                        // 处理TTS音频数据
                        post_event(handle, ESP_COZE_EVENT_TTS_AUDIO, data->data_ptr, data->data_len, NULL, 0);
                        handle->stats.total_audio_bytes_received += data->data_len;
                    }
                }

                cJSON_Delete(json);
            }
        }
        break;

    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGE(TAG, "WebSocket错误: %s", (char *)data->data_ptr);
        xSemaphoreTake(handle->mutex, portMAX_DELAY);
        handle->ws_state = ESP_COZE_WS_STATE_ERROR;
        xSemaphoreGive(handle->mutex);
        post_event(handle, ESP_COZE_EVENT_ERROR, NULL, 0, (char *)data->data_ptr, -1);
        break;

    default:
        break;
    }
}

/**
 * @brief 事件处理任务
 */
static void event_task(void *param)
{
    esp_coze_chat_handle_t handle = (esp_coze_chat_handle_t)param;
    esp_coze_internal_event_t *internal_event;

    while (1) {
        if (xQueueReceive(handle->event_queue, &internal_event, portMAX_DELAY) == pdTRUE) {
            if (handle->config.event_callback) {
                handle->config.event_callback(handle, &internal_event->event_data, handle->config.user_data);
            }
            free_internal_event(internal_event);
        }
    }
}

/**
 * @brief 心跳任务
 */
static void heartbeat_task(void *param)
{
    esp_coze_chat_handle_t handle = (esp_coze_chat_handle_t)param;
    TickType_t delay = pdMS_TO_TICKS(handle->config.keepalive_idle_timeout_ms);

    while (1) {
        vTaskDelay(delay);

        if (handle->ws_state == ESP_COZE_WS_STATE_CONNECTED) {
            uint64_t current_time = get_timestamp_ms();
            if (current_time - handle->last_heartbeat_time > handle->config.keepalive_idle_timeout_ms) {
                ESP_LOGD(TAG, "发送心跳");
                send_control_message(handle, "ping", NULL);
                handle->last_heartbeat_time = current_time;
            }
        }
    }
}

/**
 * @brief 重连定时器回调
 */
static void reconnect_timer_callback(void *arg)
{
    esp_coze_chat_handle_t handle = (esp_coze_chat_handle_t)arg;

    xSemaphoreTake(handle->mutex, portMAX_DELAY);
    handle->reconnect_attempts++;
    handle->stats.reconnection_count++;
    xSemaphoreGive(handle->mutex);

    ESP_LOGI(TAG, "执行重连...");
    esp_coze_chat_start(handle);
}

// 公共API实现

esp_err_t esp_coze_chat_init(const esp_coze_chat_config_t *config, esp_coze_chat_handle_t *handle)
{
    ESP_RETURN_ON_FALSE(config != NULL && handle != NULL, ESP_ERR_INVALID_ARG, TAG, "参数不能为空");
    ESP_RETURN_ON_ERROR(validate_config(config), TAG, "配置验证失败");

    esp_coze_chat_handle_t h = calloc(1, sizeof(struct esp_coze_chat_handle_s));
    ESP_RETURN_ON_FALSE(h != NULL, ESP_ERR_NO_MEM, TAG, "分配句柄内存失败");

    // 复制配置
    memcpy(&h->config, config, sizeof(esp_coze_chat_config_t));

    esp_err_t ret = ESP_OK;
    
    // 复制字符串字段
    if (config->ws_base_url) {
        h->ws_url = strdup(config->ws_base_url);
    } else {
        h->ws_url = strdup(ESP_COZE_DEFAULT_WS_URL);
    }

    if (config->access_token) {
        h->config.access_token = strdup(config->access_token);
    }

    if (config->bot_id) {
        h->config.bot_id = strdup(config->bot_id);
    }

    if (config->workflow_id) {
        h->config.workflow_id = strdup(config->workflow_id);
    }

    if (config->device_id) {
        h->config.device_id = strdup(config->device_id);
    }

    if (config->conversation_id) {
        h->config.conversation_id = strdup(config->conversation_id);
    }

    // 设置默认值
    if (h->config.connect_timeout_ms == 0) {
        h->config.connect_timeout_ms = ESP_COZE_DEFAULT_TIMEOUT_MS;
    }

    if (h->config.keepalive_idle_timeout_ms == 0) {
        h->config.keepalive_idle_timeout_ms = ESP_COZE_DEFAULT_KEEPALIVE_MS;
    }

    if (h->config.max_reconnect_attempts == 0) {
        h->config.max_reconnect_attempts = ESP_COZE_MAX_RECONNECT_ATTEMPTS;
    }

    // 设置默认音频配置
    if (h->config.audio_config.format == 0) {
        h->config.audio_config.format = ESP_COZE_AUDIO_FORMAT_PCM;
        h->config.audio_config.sample_rate = ESP_COZE_SAMPLE_RATE_16000;
        h->config.audio_config.channels = 1;
        h->config.audio_config.bits_per_sample = 16;
    }

    // 创建同步对象
    h->mutex = xSemaphoreCreateMutex();
    ESP_GOTO_ON_FALSE(h->mutex != NULL, ESP_ERR_NO_MEM, cleanup, TAG, "创建互斥锁失败");

    h->event_queue = xQueueCreate(10, sizeof(esp_coze_internal_event_t *));
    ESP_GOTO_ON_FALSE(h->event_queue != NULL, ESP_ERR_NO_MEM, cleanup, TAG, "创建事件队列失败");

    // 创建重连定时器
    esp_timer_create_args_t timer_args = {
        .callback = reconnect_timer_callback,
        .arg = h,
        .name = "coze_reconnect"
    };
    ret = esp_timer_create(&timer_args, &h->reconnect_timer);
    ESP_GOTO_ON_ERROR(ret, cleanup, TAG, "创建重连定时器失败");

    // 构建WebSocket URL
    ret = build_websocket_url(h);
    ESP_GOTO_ON_ERROR(ret, cleanup, TAG, "构建WebSocket URL失败");
    
    // 初始化WebSocket客户端配置
    esp_websocket_client_config_t ws_cfg = {
        .uri = h->full_ws_url,
        .headers = NULL,
        .task_stack = 4096,
        .task_prio = 5,
    };
    
    // 准备认证头
    if (h->config.access_token) {
        size_t header_len = strlen("Authorization: Bearer ") + strlen(h->config.access_token) + 3;
        h->auth_header = malloc(header_len);
        if (h->auth_header) {
            snprintf(h->auth_header, header_len, "Authorization: Bearer %s\r\n", h->config.access_token);
            ws_cfg.headers = h->auth_header;
        }
    }

    h->ws_client = esp_websocket_client_init(&ws_cfg);
    ESP_GOTO_ON_FALSE(h->ws_client != NULL, ESP_FAIL, cleanup, TAG, "初始化WebSocket客户端失败");

    esp_websocket_register_events(h->ws_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, h);

    // 创建事件处理任务
    BaseType_t task_ret = xTaskCreate(event_task, "coze_event", 3072, h, 5, &h->event_task_handle);
    ESP_GOTO_ON_FALSE(task_ret == pdTRUE, ESP_FAIL, cleanup, TAG, "创建事件任务失败");

    // 创建心跳任务
    task_ret = xTaskCreate(heartbeat_task, "coze_heartbeat", 2048, h, 4, &h->heartbeat_task_handle);
    ESP_GOTO_ON_FALSE(task_ret == pdTRUE, ESP_FAIL, cleanup, TAG, "创建心跳任务失败");

    h->ws_state = ESP_COZE_WS_STATE_DISCONNECTED;
    h->last_heartbeat_time = get_timestamp_ms();

        *handle = h;
    ESP_LOGI(TAG, "扣子聊天客户端初始化成功");
    
    return ESP_OK;
    
cleanup:
    esp_coze_chat_destroy(h);
    return ESP_FAIL;
}

esp_err_t esp_coze_chat_start(esp_coze_chat_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "句柄不能为空");

    xSemaphoreTake(handle->mutex, portMAX_DELAY);

    if (handle->ws_state == ESP_COZE_WS_STATE_CONNECTED) {
        xSemaphoreGive(handle->mutex);
        return ESP_OK;
    }

    handle->ws_state = ESP_COZE_WS_STATE_CONNECTING;
    xSemaphoreGive(handle->mutex);

    esp_err_t ret = esp_websocket_client_start(handle->ws_client);
    if (ret != ESP_OK) {
        xSemaphoreTake(handle->mutex, portMAX_DELAY);
        handle->ws_state = ESP_COZE_WS_STATE_ERROR;
        xSemaphoreGive(handle->mutex);
        ESP_LOGE(TAG, "启动WebSocket客户端失败: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "正在连接到扣子服务器...");
    return ESP_OK;
}

esp_err_t esp_coze_chat_stop(esp_coze_chat_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "句柄不能为空");

    xSemaphoreTake(handle->mutex, portMAX_DELAY);

    if (handle->ws_state == ESP_COZE_WS_STATE_DISCONNECTED) {
        xSemaphoreGive(handle->mutex);
        return ESP_OK;
    }

    // 停止重连定时器
    esp_timer_stop(handle->reconnect_timer);

    handle->ws_state = ESP_COZE_WS_STATE_DISCONNECTED;
    handle->is_speech_active = false;
    handle->is_conversation_active = false;

    xSemaphoreGive(handle->mutex);

    esp_err_t ret = esp_websocket_client_stop(handle->ws_client);
    ESP_LOGI(TAG, "扣子聊天客户端已停止");
    return ret;
}

esp_err_t esp_coze_chat_destroy(esp_coze_chat_handle_t handle)
{
    if (handle == NULL) {
        return ESP_OK;
    }

    // 停止连接
    esp_coze_chat_stop(handle);

    // 删除任务
    if (handle->event_task_handle) {
        vTaskDelete(handle->event_task_handle);
    }

    if (handle->heartbeat_task_handle) {
        vTaskDelete(handle->heartbeat_task_handle);
    }

    // 删除定时器
    if (handle->reconnect_timer) {
        esp_timer_delete(handle->reconnect_timer);
    }

    // 销毁WebSocket客户端
    if (handle->ws_client) {
        esp_websocket_client_destroy(handle->ws_client);
    }

    // 清理事件队列
    if (handle->event_queue) {
        esp_coze_internal_event_t *internal_event;
        while (xQueueReceive(handle->event_queue, &internal_event, 0) == pdTRUE) {
            free_internal_event(internal_event);
        }
        vQueueDelete(handle->event_queue);
    }

    // 删除同步对象
    if (handle->mutex) {
        vSemaphoreDelete(handle->mutex);
    }

        // 释放字符串
    if (handle->ws_url) {
        free(handle->ws_url);
    }
    
    if (handle->full_ws_url) {
        free(handle->full_ws_url);
    }
    
    if (handle->auth_header) {
        free(handle->auth_header);
    }

    if (handle->config.access_token) {
        free(handle->config.access_token);
    }

    if (handle->config.bot_id) {
        free(handle->config.bot_id);
    }

    if (handle->config.workflow_id) {
        free(handle->config.workflow_id);
    }

    if (handle->config.device_id) {
        free(handle->config.device_id);
    }

    if (handle->config.conversation_id) {
        free(handle->config.conversation_id);
    }

    free(handle);
    ESP_LOGI(TAG, "扣子聊天客户端已销毁");
    return ESP_OK;
}

esp_err_t esp_coze_chat_send_audio(esp_coze_chat_handle_t handle, const uint8_t *audio_data, size_t data_len)
{
    ESP_RETURN_ON_FALSE(handle != NULL && audio_data != NULL && data_len > 0, ESP_ERR_INVALID_ARG, TAG, "参数不能为空");

    return send_audio_message(handle, audio_data, data_len);
}

esp_err_t esp_coze_chat_send_text(esp_coze_chat_handle_t handle, const char *message)
{
    ESP_RETURN_ON_FALSE(handle != NULL && message != NULL, ESP_ERR_INVALID_ARG, TAG, "参数不能为空");

    cJSON *json = cJSON_CreateObject();
    cJSON *type = cJSON_CreateString("text");
    cJSON *content = cJSON_CreateString(message);

    cJSON_AddItemToObject(json, "type", type);
    cJSON_AddItemToObject(json, "content", content);

    char *json_string = cJSON_Print(json);
    esp_err_t ret = send_websocket_message(handle, json_string);

    free(json_string);
    cJSON_Delete(json);

    return ret;
}

esp_err_t esp_coze_chat_start_speech_input(esp_coze_chat_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "句柄不能为空");

    return send_control_message(handle, "start_speech_input", NULL);
}

esp_err_t esp_coze_chat_stop_speech_input(esp_coze_chat_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "句柄不能为空");

    return send_control_message(handle, "stop_speech_input", NULL);
}

esp_err_t esp_coze_chat_interrupt_conversation(esp_coze_chat_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "句柄不能为空");

    return send_control_message(handle, "interrupt", NULL);
}

esp_err_t esp_coze_chat_update_config(esp_coze_chat_handle_t handle, const esp_coze_chat_config_t *config)
{
    ESP_RETURN_ON_FALSE(handle != NULL && config != NULL, ESP_ERR_INVALID_ARG, TAG, "参数不能为空");
    ESP_RETURN_ON_ERROR(validate_config(config), TAG, "配置验证失败");

    xSemaphoreTake(handle->mutex, portMAX_DELAY);

    // 更新配置（这里只更新部分字段，避免影响正在运行的连接）
    if (config->bot_id && strcmp(config->bot_id, handle->config.bot_id) != 0) {
        free(handle->config.bot_id);
        handle->config.bot_id = strdup(config->bot_id);
    }

    if (config->workflow_id) {
        if (handle->config.workflow_id) {
            free(handle->config.workflow_id);
        }
        handle->config.workflow_id = strdup(config->workflow_id);
    }

    if (config->device_id) {
        if (handle->config.device_id) {
            free(handle->config.device_id);
        }
        handle->config.device_id = strdup(config->device_id);
    }

    if (config->conversation_id) {
        if (handle->config.conversation_id) {
            free(handle->config.conversation_id);
        }
        handle->config.conversation_id = strdup(config->conversation_id);
    }

    // 更新音频配置
    memcpy(&handle->config.audio_config, &config->audio_config, sizeof(esp_coze_audio_config_t));

    // 更新回调函数
    handle->config.event_callback = config->event_callback;
    handle->config.user_data = config->user_data;

    xSemaphoreGive(handle->mutex);

    ESP_LOGI(TAG, "配置已更新");
    return ESP_OK;
}

esp_err_t esp_coze_chat_get_session_info(esp_coze_chat_handle_t handle, esp_coze_session_info_t *session_info)
{
    ESP_RETURN_ON_FALSE(handle != NULL && session_info != NULL, ESP_ERR_INVALID_ARG, TAG, "参数不能为空");

    xSemaphoreTake(handle->mutex, portMAX_DELAY);
    memcpy(session_info, &handle->session_info, sizeof(esp_coze_session_info_t));
    session_info->is_active = handle->is_conversation_active;
    xSemaphoreGive(handle->mutex);

    return ESP_OK;
}

bool esp_coze_chat_is_connected(esp_coze_chat_handle_t handle)
{
    if (handle == NULL) {
        return false;
    }

    xSemaphoreTake(handle->mutex, portMAX_DELAY);
    bool connected = (handle->ws_state == ESP_COZE_WS_STATE_CONNECTED);
    xSemaphoreGive(handle->mutex);

    return connected;
}

esp_err_t esp_coze_chat_get_stats(esp_coze_chat_handle_t handle, esp_coze_stats_t *stats)
{
    ESP_RETURN_ON_FALSE(handle != NULL && stats != NULL, ESP_ERR_INVALID_ARG, TAG, "参数不能为空");

    xSemaphoreTake(handle->mutex, portMAX_DELAY);
    memcpy(stats, &handle->stats, sizeof(esp_coze_stats_t));

    // 计算会话持续时间
    if (handle->session_start_time > 0) {
        stats->session_duration_ms = get_timestamp_ms() - handle->session_start_time;
    }

    xSemaphoreGive(handle->mutex);

    return ESP_OK;
}

esp_err_t esp_coze_chat_reset_stats(esp_coze_chat_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "句柄不能为空");

    xSemaphoreTake(handle->mutex, portMAX_DELAY);
    memset(&handle->stats, 0, sizeof(esp_coze_stats_t));
    handle->session_start_time = get_timestamp_ms();
    xSemaphoreGive(handle->mutex);

    ESP_LOGI(TAG, "统计信息已重置");
    return ESP_OK;
}

esp_err_t esp_coze_chat_set_log_level(esp_log_level_t level)
{
    s_log_level = level;
    esp_log_level_set(TAG, level);
    return ESP_OK;
}

const char *esp_coze_chat_get_version(void)
{
    return ESP_COZE_CHAT_VERSION;
}
