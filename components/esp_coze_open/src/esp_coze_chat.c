/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-27 21:30:00
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-27 21:30:00
 * @FilePath: \esp-brookesia-chunfeng\components\esp_coze_open\src\esp_coze_chat.c
 * @Description: 扣子聊天客户端实现 - 环形缓冲区版本
 * 
*/
#include "esp_coze_chat.h"
#include "esp_coze_ring_buffer.h"
#include "esp_coze_audio_flash.h"
#include "cJSON.h"

static const char *TAG = "ESP_COZE_CHAT";

static esp_coze_chat_handle_t *g_coze_handle = NULL;
static esp_coze_ring_buffer_t g_ring_buffer = {0};
static TaskHandle_t g_parser_task_handle = NULL;
static bool g_parser_running = false;

// 外部声明音频处理函数
extern esp_err_t esp_coze_audio_process_base64(const char *base64_data);
extern esp_coze_audio_flash_t* esp_coze_audio_get_flash_instance(void);

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
                cJSON *json = cJSON_ParseWithLength((char*)json_buffer, json_len);
                if (json) {
                    cJSON *event_type_item = cJSON_GetObjectItem(json, "event_type");
                    if (event_type_item && cJSON_IsString(event_type_item)) {
                        const char *event_type = cJSON_GetStringValue(event_type_item);
                        
                        if (strcmp(event_type, "conversation.audio.delta") == 0) {
                            // 处理音频数据 - 优先处理，立即写入Flash
                            cJSON *data_item = cJSON_GetObjectItem(json, "data");
                            if (data_item) {
                                cJSON *content_item = cJSON_GetObjectItem(data_item, "content");
                                if (content_item && cJSON_IsString(content_item)) {
                                    const char *audio_base64 = cJSON_GetStringValue(content_item);
                                    ESP_LOGI(TAG, "处理音频数据，base64长度: %d", (int)strlen(audio_base64));
                                    
                                    // 立即处理音频数据并写入Flash
                                    esp_err_t ret = esp_coze_audio_process_base64(audio_base64);
                                    if (ret != ESP_OK) {
                                        ESP_LOGW(TAG, "音频数据处理失败: %s", esp_err_to_name(ret));
                                    }
                                }
                            }
                        } else {
                            // 其他事件直接打印
                            ESP_LOGI(TAG, "收到事件: %s", event_type);
                        }
                    }
                    cJSON_Delete(json);
                } else {
                    ESP_LOGW(TAG, "JSON解析失败: %.*s", (int)json_len > 100 ? 100 : (int)json_len, (char*)json_buffer);
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
            esp_err_t ret = esp_coze_ring_buffer_write(&g_ring_buffer, (uint8_t*)data->data_ptr, data->data_len);
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

    // 初始化音频Flash存储
    esp_coze_audio_flash_t *audio_flash = esp_coze_audio_get_flash_instance();
    ret = esp_coze_audio_flash_init(audio_flash, AUDIO_FLASH_BASE_ADDR, AUDIO_FLASH_SIZE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "初始化音频Flash存储失败");
        goto cleanup;
    }

    // 启动数据解析任务
    g_parser_running = true;
    BaseType_t task_ret = xTaskCreate(data_parser_task, "data_parser", 8192, NULL, 5, &g_parser_task_handle);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "创建数据解析任务失败");
        g_parser_running = false;
        goto cleanup;
    }

    // 启动音频播放任务
    ret = esp_coze_audio_player_start();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "启动音频播放任务失败: %s", esp_err_to_name(ret));
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

    // 停止音频播放任务
    esp_coze_audio_player_stop();

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

    // 销毁音频Flash存储
    esp_coze_audio_flash_t *audio_flash = esp_coze_audio_get_flash_instance();
    esp_coze_audio_flash_deinit(audio_flash);

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

    int sent_len = esp_websocket_client_send_bin(g_coze_handle->ws_client, (char*)data, len, portMAX_DELAY);
    if (sent_len < 0) {
        ESP_LOGE(TAG, "发送WebSocket二进制消息失败");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "成功发送WebSocket二进制消息，长度: %d", sent_len);
    return ESP_OK;
}
