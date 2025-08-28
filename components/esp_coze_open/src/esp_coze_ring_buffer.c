/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-27 21:00:00
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-27 21:00:00
 * @FilePath: \esp-brookesia-chunfeng\components\esp_coze_open\src\esp_coze_ring_buffer.c
 * @Description: 环形缓冲区实现
 * 
 */
#include "esp_coze_ring_buffer.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "RING_BUFFER";

/**
 * @brief 初始化环形缓冲区
 */
esp_err_t esp_coze_ring_buffer_init(esp_coze_ring_buffer_t *rb, size_t size)
{
    if (!rb || size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    rb->buffer = malloc(size);
    if (!rb->buffer) {
        ESP_LOGE(TAG, "分配缓冲区内存失败");
        return ESP_ERR_NO_MEM;
    }

    rb->size = size;
    rb->write_pos = 0;
    rb->read_pos = 0;

    rb->mutex = xSemaphoreCreateMutex();
    if (!rb->mutex) {
        free(rb->buffer);
        ESP_LOGE(TAG, "创建互斥锁失败");
        return ESP_ERR_NO_MEM;
    }

    rb->data_sem = xSemaphoreCreateBinary();
    if (!rb->data_sem) {
        vSemaphoreDelete(rb->mutex);
        free(rb->buffer);
        ESP_LOGE(TAG, "创建信号量失败");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "环形缓冲区初始化成功，大小: %d bytes", (int)size);
    return ESP_OK;
}


/**
 * @brief 从环形缓冲区读取字节数据
 */
esp_err_t esp_coze_ring_buffer_read(esp_coze_ring_buffer_t *rb, uint8_t *out, size_t max_len, size_t *out_len, uint32_t timeout_ms)
{
    if (!rb || !out || max_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (out_len) *out_len = 0;

    // 如果无数据且允许等待，则等待一次信号
    if (rb->read_pos == rb->write_pos && timeout_ms > 0) {
        if (xSemaphoreTake(rb->data_sem, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
            return ESP_ERR_TIMEOUT;
        }
    }

    if (xSemaphoreTake(rb->mutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    size_t available;
    if (rb->write_pos >= rb->read_pos) {
        available = rb->write_pos - rb->read_pos;
    } else {
        available = rb->size - rb->read_pos + rb->write_pos;
    }

    size_t to_read = (available < max_len) ? available : max_len;
    for (size_t i = 0; i < to_read; ++i) {
        out[i] = rb->buffer[rb->read_pos];
        rb->read_pos = (rb->read_pos + 1) % rb->size;
    }

    xSemaphoreGive(rb->mutex);

    if (out_len) *out_len = to_read;
    return (to_read > 0) ? ESP_OK : ESP_ERR_TIMEOUT;
}

/**
 * @brief 向环形缓冲区写入数据
 */
esp_err_t esp_coze_ring_buffer_write(esp_coze_ring_buffer_t *rb, const uint8_t *data, size_t len)
{
    if (!rb || !data || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(rb->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "获取互斥锁超时");
        return ESP_ERR_TIMEOUT;
    }

    // 检查剩余空间
    size_t available_space;
    if (rb->write_pos >= rb->read_pos) {
        available_space = rb->size - rb->write_pos + rb->read_pos - 1;
    } else {
        available_space = rb->read_pos - rb->write_pos - 1;
    }

    if (len > available_space) {
        ESP_LOGW(TAG, "缓冲区空间不足，需要: %d, 可用: %d", (int)len, (int)available_space);
        xSemaphoreGive(rb->mutex);
        return ESP_ERR_NO_MEM;
    }

    // 写入数据
    for (size_t i = 0; i < len; i++) {
        rb->buffer[rb->write_pos] = data[i];
        rb->write_pos = (rb->write_pos + 1) % rb->size;
    }

    xSemaphoreGive(rb->mutex);
    
    // 通知有数据可读
    xSemaphoreGive(rb->data_sem);

    ESP_LOGD(TAG, "写入数据: %d bytes, 写入位置: %d", (int)len, (int)rb->write_pos);
    return ESP_OK;
}



/**
 * @brief 从环形缓冲区读取一个完整的JSON对象
 */
esp_err_t esp_coze_ring_buffer_read_json_object(esp_coze_ring_buffer_t *rb, uint8_t *data, size_t max_len, size_t *actual_len)
{
    if (!rb || !data || !actual_len || max_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    *actual_len = 0;

    if (xSemaphoreTake(rb->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "获取互斥锁超时");
        return ESP_ERR_TIMEOUT;
    }

    size_t temp_read_pos = rb->read_pos;
    size_t json_len = 0;
    int brace_count = 0;
    bool found_start = false;
    bool in_string = false;
    bool escape_next = false;

    // 查找完整的JSON对象
    while (temp_read_pos != rb->write_pos && json_len < max_len - 1) {
        uint8_t ch = rb->buffer[temp_read_pos];
        temp_read_pos = (temp_read_pos + 1) % rb->size;
        json_len++;

        if (escape_next) {
            escape_next = false;
            continue;
        }

        if (ch == '\\') {
            escape_next = true;
            continue;
        }

        if (ch == '"' && !escape_next) {
            in_string = !in_string;
            continue;
        }

        if (!in_string) {
            if (ch == '{') {
                brace_count++;
                found_start = true;
            } else if (ch == '}' && found_start) {
                brace_count--;
                if (brace_count == 0) {
                    // 找到完整的JSON对象
                    break;
                }
            }
        }
    }

    if (!found_start || brace_count != 0) {
        // 没有找到完整的JSON对象
        size_t available = (rb->write_pos >= rb->read_pos) ? 
                          (rb->write_pos - rb->read_pos) : 
                          (rb->size - rb->read_pos + rb->write_pos);
        xSemaphoreGive(rb->mutex);
        ESP_LOGD(TAG, "未找到完整JSON对象，found_start=%d, brace_count=%d, available=%d", 
                 found_start, brace_count, (int)available);
        return ESP_ERR_NOT_FOUND;
    }

    // 读取完整的JSON对象并更新读取指针
    for (size_t i = 0; i < json_len; i++) {
        data[i] = rb->buffer[rb->read_pos];
        rb->read_pos = (rb->read_pos + 1) % rb->size;
    }

    data[json_len] = '\0';
    *actual_len = json_len;

    xSemaphoreGive(rb->mutex);

    ESP_LOGD(TAG, "读取JSON对象: %d bytes", (int)json_len);
    return ESP_OK;
}

/**
 * @brief 获取环形缓冲区可用数据量
 */
size_t esp_coze_ring_buffer_available(esp_coze_ring_buffer_t *rb)
{
    if (!rb) {
        return 0;
    }

    if (xSemaphoreTake(rb->mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return 0;
    }

    size_t available;
    if (rb->write_pos >= rb->read_pos) {
        available = rb->write_pos - rb->read_pos;
    } else {
        available = rb->size - rb->read_pos + rb->write_pos;
    }

    xSemaphoreGive(rb->mutex);
    return available;
}

/**
 * @brief 销毁环形缓冲区
 */
void esp_coze_ring_buffer_deinit(esp_coze_ring_buffer_t *rb)
{
    if (!rb) {
        return;
    }

    if (rb->mutex) {
        vSemaphoreDelete(rb->mutex);
    }

    if (rb->data_sem) {
        vSemaphoreDelete(rb->data_sem);
    }

    if (rb->buffer) {
        free(rb->buffer);
    }

    memset(rb, 0, sizeof(esp_coze_ring_buffer_t));
    ESP_LOGI(TAG, "环形缓冲区销毁完成");
}
