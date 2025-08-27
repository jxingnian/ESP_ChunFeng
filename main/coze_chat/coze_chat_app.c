/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-21 17:22:36
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-27 15:41:00
 * @FilePath: \esp-brookesia-chunfeng\main\coze_chat\coze_chat_app.c
 * @Description: Coze聊天应用程序实现文件，负责初始化和管理与Coze服务器的WebSocket连接
 *
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

#include "esp_coze_chat.h"
#include "audio_hal.h"

// 日志标签
static const char *TAG = "COZE_CHAT_APP";

/**
 * @brief 初始化并启动Coze聊天服务
 * 
 * 配置Coze聊天参数，包括服务器地址、认证信息、音频配置等，
 * 然后初始化并启动与Coze服务器的WebSocket连接
 * 
 * @return esp_err_t 
 *         - ESP_OK: 初始化成功
 *         - 其他: 初始化失败的错误码
 */
static esp_err_t init_and_start_coze(void)
{
    esp_err_t ret;

    // 初始化Coze聊天客户端
    ret = esp_coze_chat_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_coze_chat_init 失败: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

/**
 * @brief 初始化Coze聊天应用程序
 * 
 * 应用程序入口函数，负责启动整个Coze聊天功能
 * 
 * @return esp_err_t 
 *         - ESP_OK: 初始化成功
 *         - 其他: 初始化失败的错误码
 */
esp_err_t coze_chat_app_init(void)
{
    // 初始化并启动Coze服务，如果失败则直接返回错误
    ESP_ERROR_CHECK(init_and_start_coze());
    return ESP_OK;
}
