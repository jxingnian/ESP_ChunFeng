/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-26 10:26:27
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-27 15:46:58
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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化扣子聊天客户端（使用固定参数的最小实现）
 *
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_NO_MEM: 内存不足
 */
esp_err_t esp_coze_chat_init();

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
