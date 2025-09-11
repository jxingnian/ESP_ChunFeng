/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-07-29 14:55:50
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-07-29 15:46:00
 * @FilePath: \esp_chunfeng\components\wifi_manage\wifi_manager.h
 * @Description:
 *
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved.
 */

#pragma once

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "http_server.h"

// WiFi相关宏定义
#define ESP_WIFI_CHANNEL   1     // WiFi信道
#define EXAMPLE_MAX_STA_CONN       4     // 最大连接数
#define ESP_AP_SSID      CONFIG_ESP_AP_SSID        // AP名称
#define ESP_AP_PASS      CONFIG_ESP_AP_PASSWORD    // AP密码
#define MAX_RETRY_COUNT 5
#define DEFAULT_SCAN_LIST_SIZE 10  // 默认扫描列表大小
#define MAX_WIFI_CONFIGS 3        // 最大保存的WiFi配置数量

#ifdef __cplusplus
extern "C" {
#endif

// WiFi配置结构体，包含优先级信息
typedef struct {
    wifi_config_t config;
    uint32_t priority;          // 优先级，数值越小优先级越高
    time_t last_success_time;   // 上次成功连接时间
    bool is_valid;              // 配置是否有效
} wifi_config_entry_t;

// 多WiFi配置管理结构体
typedef struct {
    wifi_config_entry_t configs[MAX_WIFI_CONFIGS];
    uint8_t count;              // 当前配置数量
    uint8_t current_index;      // 当前尝试连接的配置索引
    uint32_t checksum;          // 校验和
} multi_wifi_config_t;

// WiFi IP获取回调函数类型定义
typedef void (*wifi_got_ip_callback_t)(esp_netif_ip_info_t *ip_info);

// WiFi初始化函数
esp_err_t wifi_init_softap(void);
// wifi连接次数重置
esp_err_t wifi_reset_connection_retry(void);
// WiFi扫描函数
esp_err_t wifi_scan_networks(wifi_ap_record_t **ap_records, uint16_t *ap_count);
// 保存WiFi配置到NVS（旧接口，兼容性保留）
esp_err_t wifi_save_config_to_nvs(const wifi_config_t *sta_config);
// 注册WiFi IP获取回调函数
esp_err_t wifi_register_got_ip_callback(wifi_got_ip_callback_t callback);

// 多WiFi配置管理函数
esp_err_t wifi_load_multi_configs(multi_wifi_config_t *multi_config);
esp_err_t wifi_save_multi_configs(const multi_wifi_config_t *multi_config);
esp_err_t wifi_add_config(const wifi_config_t *config);
esp_err_t wifi_remove_config(const char *ssid);
esp_err_t wifi_get_saved_configs(wifi_config_entry_t **configs, uint8_t *count);
esp_err_t wifi_connect_next_config(void);

#ifdef __cplusplus
}
#endif
