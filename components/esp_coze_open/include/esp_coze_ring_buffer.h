/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-27 21:00:00
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-28 11:35:51
 * @FilePath: \esp-chunfeng\components\esp_coze_open\include\esp_coze_ring_buffer.h
 * @Description: 环形缓冲区实现
 *
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RING_BUFFER_SIZE 131072  // 128KB环形缓冲区

/**
 * @brief 环形缓冲区结构体
 */
typedef struct {
    uint8_t *buffer;           ///< 缓冲区数据
    size_t size;               ///< 缓冲区总大小
    volatile size_t write_pos; ///< 写入位置
    volatile size_t read_pos;  ///< 读取位置
    SemaphoreHandle_t mutex;   ///< 互斥锁
    SemaphoreHandle_t data_sem; ///< 数据信号量
} esp_coze_ring_buffer_t;

/**
 * @brief 初始化环形缓冲区
 *
 * @param rb 环形缓冲区指针
 * @param size 缓冲区大小
 * @return esp_err_t
 */
esp_err_t esp_coze_ring_buffer_init(esp_coze_ring_buffer_t *rb, size_t size);

/**
 * @brief 向环形缓冲区写入数据
 *
 * @param rb 环形缓冲区指针
 * @param data 数据指针
 * @param len 数据长度
 * @return esp_err_t
 */
esp_err_t esp_coze_ring_buffer_write(esp_coze_ring_buffer_t *rb, const uint8_t *data, size_t len);

/**
 * @brief 从环形缓冲区读取字节数据
 *
 * @param rb 环形缓冲区指针
 * @param out 输出缓冲区
 * @param max_len 希望读取的最大字节数
 * @param out_len 实际读取的字节数（可为NULL）
 * @param timeout_ms 当无可读数据时等待的超时（毫秒），0表示不等待
 * @return esp_err_t
 *         - ESP_OK: 读取成功（可能小于max_len）
 *         - ESP_ERR_TIMEOUT: 超时无数据
 *         - ESP_ERR_INVALID_ARG: 参数错误
 */
esp_err_t esp_coze_ring_buffer_read(esp_coze_ring_buffer_t *rb, uint8_t *out, size_t max_len, size_t *out_len, uint32_t timeout_ms);

/**
 * @brief 从环形缓冲区读取一个完整的JSON对象
 *
 * @param rb 环形缓冲区指针
 * @param data 输出数据缓冲区
 * @param max_len 最大读取长度
 * @param actual_len 实际读取长度
 * @return esp_err_t
 */
esp_err_t esp_coze_ring_buffer_read_json_object(esp_coze_ring_buffer_t *rb, uint8_t *data, size_t max_len, size_t *actual_len);

/**
 * @brief 获取环形缓冲区可用数据量
 *
 * @param rb 环形缓冲区指针
 * @return size_t 可用数据量
 */
size_t esp_coze_ring_buffer_available(esp_coze_ring_buffer_t *rb);

/**
 * @brief 销毁环形缓冲区
 *
 * @param rb 环形缓冲区指针
 */
void esp_coze_ring_buffer_deinit(esp_coze_ring_buffer_t *rb);

#ifdef __cplusplus
}
#endif
