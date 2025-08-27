/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-26 10:26:27
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-27 17:01:18
 * @FilePath: \esp-brookesia-chunfeng\components\esp_coze_open\include\esp_coze_chat.h
 * @Description: 扣子聊天客户端头文件
 *
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_websocket_client.h"
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
#include "esp_check.h"
#include "esp_timer.h"
#include "cJSON.h"

// 默认配置参数宏定义
#define ESP_COZE_DEFAULT_WS_BASE_URL "wss://ws.coze.cn/v1/chat"                                        // 默认扣子WebSocket服务器地址
#define ESP_COZE_DEFAULT_ACCESS_TOKEN "sat_NmQ4PmUmFHYUjI9JdlusqfFtOvD2qOzjdWZ5nHTU3IsamZEuG2fuNrONhxpscThM"  // 默认访问令牌
#define ESP_COZE_DEFAULT_BOT_ID "7507830126416560143"                                                  // 默认机器人 ID
#define ESP_COZE_DEFAULT_DEVICE_ID "123456789"                                                         // 默认设备ID
#define ESP_COZE_DEFAULT_CONVERSATION_ID "default_conversation"                                        // 默认会话 ID
/**
 * @brief WebSocket连接状态枚举
 */
typedef enum {
    ESP_COZE_WS_STATE_DISCONNECTED = 0,  ///< WebSocket已断开连接
    ESP_COZE_WS_STATE_CONNECTING,        ///< WebSocket正在连接中
    ESP_COZE_WS_STATE_CONNECTED,         ///< WebSocket已成功连接
    ESP_COZE_WS_STATE_ERROR              ///< WebSocket连接出现错误
} esp_coze_ws_state_t;

/**
 * @brief 扣子聊天WebSocket句柄结构体
 */
typedef struct {
    esp_websocket_client_handle_t ws_client;  ///< ESP-IDF WebSocket客户端句柄
    char *ws_url;                             ///< WebSocket服务器完整URL地址
    char *access_token;                       ///< 扣子API访问令牌
    char *bot_id;                            ///< 扣子机器人唯一标识ID
    char *device_id;                         ///< 设备唯一标识ID
    char *conversation_id;                   ///< 会话唯一标识ID
    char *auth_header;                       ///< 预构建的认证头部字符串
    esp_coze_ws_state_t ws_state;           ///< 当前WebSocket连接状态
} esp_coze_chat_handle_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 扣子聊天配置参数结构体
 */
typedef struct {
    char *ws_base_url;      ///< WebSocket基础URL
    char *access_token;     ///< 访问令牌
    char *bot_id;          ///< 机器人 ID
    char *device_id;       ///< 设备ID
    char *conversation_id; ///< 会话 ID (可选，传NULL使用默认值)
} esp_coze_chat_config_t;

/**
 * @brief 使用自定义参数初始化扣子聊天客户端
 *
 * @param config 配置参数，不能为NULL
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_ERR_NO_MEM: 内存不足
 */
esp_err_t esp_coze_chat_init_with_config(const esp_coze_chat_config_t *config);

/**
 * @brief 启动扣子聊天服务（仅连接WebSocket，需要先初始化）
 *
 * @note 必须先调用 esp_coze_chat_init() 或 esp_coze_chat_init_with_config() 进行初始化
 *
 * @return esp_err_t
 *         - ESP_OK: 成功启动
 *         - ESP_ERR_INVALID_STATE: 客户端未初始化
 *         - ESP_FAIL: 连接失败
 */
esp_err_t esp_coze_chat_start();

/**
 * @brief 销毁扣子聊天客户端
 *
 * @return esp_err_t
 *         - ESP_OK: 成功
 */
esp_err_t esp_coze_chat_destroy();

/**
 * @brief 连接到扣子WebSocket服务器
 *
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_FAIL: 连接失败
 */
esp_err_t esp_coze_chat_connect();

/**
 * @brief 断开WebSocket连接
 *
 * @return esp_err_t
 *         - ESP_OK: 成功
 */
esp_err_t esp_coze_chat_disconnect();

#ifdef __cplusplus
}
#endif
