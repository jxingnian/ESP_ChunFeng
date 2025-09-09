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
#include "esp_heap_caps.h"
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

    // 优先使用PSRAM分配缓冲区
    rb->buffer = (uint8_t *)heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    if (!rb->buffer) {
        ESP_LOGW(TAG, "PSRAM分配失败，尝试内部RAM");
        rb->buffer = malloc(size);
    }

    if (!rb->buffer) {
        ESP_LOGE(TAG, "分配缓冲区内存失败，需要%d字节", (int)size);
        return ESP_ERR_NO_MEM;
    }

    // 显示分配的内存类型
    ESP_LOGI(TAG, "WebSocket缓冲区分配成功: %d KB, 位置: %s",
             (int)(size / 1024),
             (esp_ptr_external_ram(rb->buffer)) ? "PSRAM" : "内部RAM");

    rb->size = size;
    rb->write_pos = 0;
    rb->read_pos = 0;
    rb->total_overwrites = 0;
    rb->total_overwritten_bytes = 0;

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

    // 真正的环形缓冲区：写入数据，必要时覆盖旧数据
    size_t overwritten_bytes = 0;
    
    for (size_t i = 0; i < len; i++) {
        // 写入数据
        rb->buffer[rb->write_pos] = data[i];
        
        // 移动写指针
        size_t old_write_pos = rb->write_pos;
        rb->write_pos = (rb->write_pos + 1) % rb->size;
        
        // 检查是否覆盖了未读数据
        if (rb->write_pos == rb->read_pos) {
            // 写指针追上了读指针，强制移动读指针（覆盖最老的数据）
            rb->read_pos = (rb->read_pos + 1) % rb->size;
            overwritten_bytes++;
            rb->total_overwritten_bytes++;
        }
    }
    
    // 如果有数据被覆盖，记录警告（但不阻止写入）
    if (overwritten_bytes > 0) {
        rb->total_overwrites++;
        ESP_LOGW(TAG, "环形缓冲区覆盖了 %d 字节旧数据，总覆盖次数: %d，总覆盖字节: %d", 
                 (int)overwritten_bytes, 
                 (int)rb->total_overwrites,
                 (int)rb->total_overwritten_bytes);
    }

    xSemaphoreGive(rb->mutex);

    // 通知有数据可读
    xSemaphoreGive(rb->data_sem);

    ESP_LOGD(TAG, "写入数据: %d bytes, 写入位置: %d (环形缓冲区永远成功)", (int)len, (int)rb->write_pos);
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

esp_err_t esp_coze_ring_buffer_get_overwrite_stats(esp_coze_ring_buffer_t *rb, 
                                                   uint32_t *total_overwrites, 
                                                   uint32_t *total_overwritten_bytes)
{
    if (!rb || !total_overwrites || !total_overwritten_bytes) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(rb->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "获取互斥锁超时");
        return ESP_ERR_TIMEOUT;
    }

    *total_overwrites = rb->total_overwrites;
    *total_overwritten_bytes = rb->total_overwritten_bytes;

    xSemaphoreGive(rb->mutex);
    return ESP_OK;
}

esp_err_t esp_coze_ring_buffer_reset_overwrite_stats(esp_coze_ring_buffer_t *rb)
{
    if (!rb) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(rb->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "获取互斥锁超时");
        return ESP_ERR_TIMEOUT;
    }

    rb->total_overwrites = 0;
    rb->total_overwritten_bytes = 0;
    ESP_LOGI(TAG, "环形缓冲区统计信息已重置");

    xSemaphoreGive(rb->mutex);
    return ESP_OK;
}
