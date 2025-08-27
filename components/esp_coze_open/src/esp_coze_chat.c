/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-26 10:26:52
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-27 15:52:21
 * @FilePath: \esp-brookesia-chunfeng\components\esp_coze_open\src\esp_coze_chat.c
 * @Description: 扣子聊天客户端实现
 * 
 */
#include "esp_coze_chat.h"

static const char *TAG = "ESP_COZE_CHAT";

// 固定配置参数
#define FIXED_WS_URL "wss://ws.coze.cn/v1/chat"
#define FIXED_ACCESS_TOKEN "pat_l6HkcY2hjo56Jfbk8GlyMh2DYux1YIrCFyJBpGAhYIxxq5xPCakJosGexmopmyxw"
#define FIXED_BOT_ID "7507830126416560143"
#define FIXED_CONVERSATION_ID "default_conversation"

// 简化的句柄结构体
typedef struct {
    esp_websocket_client_handle_t ws_client;
    char *ws_url;
    char *access_token;
    char *bot_id;
    char *conversation_id;
    char *auth_header;
} esp_coze_chat_handle_t;

static esp_coze_chat_handle_t *g_coze_handle = NULL;

esp_err_t esp_coze_chat_init()
{
    // 如果已经初始化过，直接返回成功
    if (g_coze_handle != NULL) {
        ESP_LOGI(TAG, "扣子聊天客户端已经初始化");
        return ESP_OK;
    }

    // 分配句柄内存
    g_coze_handle = calloc(1, sizeof(esp_coze_chat_handle_t));
    if (g_coze_handle == NULL) {
        ESP_LOGE(TAG, "分配句柄内存失败");
        return ESP_ERR_NO_MEM;
    }

    // 设置固定参数
    g_coze_handle->ws_url = strdup(FIXED_WS_URL);
    g_coze_handle->access_token = strdup(FIXED_ACCESS_TOKEN);
    g_coze_handle->bot_id = strdup(FIXED_BOT_ID);
    g_coze_handle->conversation_id = strdup(FIXED_CONVERSATION_ID);

    // 构建Authorization请求头
    size_t auth_header_len = strlen("Authorization: Bearer ") + strlen(FIXED_ACCESS_TOKEN) + 3;
    g_coze_handle->auth_header = malloc(auth_header_len);
    if (g_coze_handle->auth_header) {
        snprintf(g_coze_handle->auth_header, auth_header_len, "Authorization: Bearer %s\r\n", FIXED_ACCESS_TOKEN);
    }

    // 检查内存分配是否成功
    if (!g_coze_handle->ws_url || !g_coze_handle->access_token || 
        !g_coze_handle->bot_id || !g_coze_handle->conversation_id || !g_coze_handle->auth_header) {
        ESP_LOGE(TAG, "字符串内存分配失败");
        goto cleanup;
    }

    // 准备WebSocket配置，包含Authorization请求头
    esp_websocket_client_config_t ws_cfg = {
        .uri = g_coze_handle->ws_url,
        .headers = g_coze_handle->auth_header,
        .task_stack = 4096,
        .task_prio = 5,
    };

    // 初始化WebSocket客户端
    g_coze_handle->ws_client = esp_websocket_client_init(&ws_cfg);
    if (g_coze_handle->ws_client == NULL) {
        ESP_LOGE(TAG, "初始化WebSocket客户端失败");
        goto cleanup;
    }

    ESP_LOGI(TAG, "扣子聊天客户端初始化成功");
    return ESP_OK;

cleanup:
    // 清理资源
    if (g_coze_handle) {
        if (g_coze_handle->ws_url) free(g_coze_handle->ws_url);
        if (g_coze_handle->access_token) free(g_coze_handle->access_token);
        if (g_coze_handle->bot_id) free(g_coze_handle->bot_id);
        if (g_coze_handle->conversation_id) free(g_coze_handle->conversation_id);
        if (g_coze_handle->auth_header) free(g_coze_handle->auth_header);
        free(g_coze_handle);
        g_coze_handle = NULL;
    }
    return ESP_ERR_NO_MEM;
}

/**
 * @brief 销毁扣子聊天客户端
 * 
 * @return esp_err_t 
 */
esp_err_t esp_coze_chat_destroy()
{
    if (g_coze_handle == NULL) {
        ESP_LOGW(TAG, "扣子聊天客户端未初始化");
        return ESP_OK;
    }

    // 销毁WebSocket客户端
    if (g_coze_handle->ws_client) {
        esp_websocket_client_destroy(g_coze_handle->ws_client);
    }

    // 释放字符串内存
    if (g_coze_handle->ws_url) free(g_coze_handle->ws_url);
    if (g_coze_handle->access_token) free(g_coze_handle->access_token);
    if (g_coze_handle->bot_id) free(g_coze_handle->bot_id);
    if (g_coze_handle->conversation_id) free(g_coze_handle->conversation_id);
    if (g_coze_handle->auth_header) free(g_coze_handle->auth_header);

    // 释放句柄内存
    free(g_coze_handle);
    g_coze_handle = NULL;

    ESP_LOGI(TAG, "扣子聊天客户端销毁成功");
    return ESP_OK;
}
