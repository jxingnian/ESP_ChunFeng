/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-27 21:00:00
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-27 21:00:00
 * @FilePath: \esp-brookesia-chunfeng\components\esp_coze_open\include\esp_coze_audio_flash.h
 * @Description: 音频Flash存储管理
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

#define AUDIO_FLASH_BASE_ADDR 0x300000  // 3MB位置开始
#define AUDIO_FLASH_SIZE      0x100000  // 1MB音频缓冲区
#define AUDIO_FLASH_SECTOR_SIZE 4096    // Flash扇区大小

/**
 * @brief 音频Flash存储结构体
 */
typedef struct {
    uint32_t base_addr;      ///< Flash基础地址
    uint32_t size;           ///< 总大小
    volatile uint32_t write_offset; ///< 写入偏移
    volatile uint32_t read_offset;  ///< 读取偏移
    SemaphoreHandle_t mutex; ///< 互斥锁
} esp_coze_audio_flash_t;

/**
 * @brief 音频播放回调函数类型
 * 
 * @param pcm_data PCM音频数据
 * @param len 数据长度
 * @param user_data 用户数据
 */
typedef void (*esp_coze_audio_play_callback_t)(const uint8_t *pcm_data, size_t len, void *user_data);

/**
 * @brief 初始化音频Flash存储
 * 
 * @param af 音频Flash结构体指针
 * @param base_addr Flash基础地址
 * @param size 存储大小
 * @return esp_err_t 
 */
esp_err_t esp_coze_audio_flash_init(esp_coze_audio_flash_t *af, uint32_t base_addr, size_t size);

/**
 * @brief 写入PCM音频数据到Flash
 * 
 * @param af 音频Flash结构体指针
 * @param pcm_data PCM数据
 * @param len 数据长度
 * @return esp_err_t 
 */
esp_err_t esp_coze_audio_flash_write(esp_coze_audio_flash_t *af, const uint8_t *pcm_data, size_t len);

/**
 * @brief 从Flash读取PCM音频数据
 * 
 * @param af 音频Flash结构体指针
 * @param pcm_data 输出缓冲区
 * @param len 请求长度
 * @param actual_len 实际读取长度
 * @return esp_err_t 
 */
esp_err_t esp_coze_audio_flash_read(esp_coze_audio_flash_t *af, uint8_t *pcm_data, size_t len, size_t *actual_len);

/**
 * @brief 获取Flash中可读的音频数据量
 * 
 * @param af 音频Flash结构体指针
 * @return uint32_t 可读数据量
 */
uint32_t esp_coze_audio_flash_available(esp_coze_audio_flash_t *af);

/**
 * @brief 清空Flash音频缓冲区
 * 
 * @param af 音频Flash结构体指针
 * @return esp_err_t 
 */
esp_err_t esp_coze_audio_flash_clear(esp_coze_audio_flash_t *af);

/**
 * @brief 注册音频播放回调函数
 * 
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return esp_err_t 
 */
esp_err_t esp_coze_audio_register_play_callback(esp_coze_audio_play_callback_t callback, void *user_data);

/**
 * @brief 启动音频播放任务
 * 
 * @return esp_err_t 
 */
esp_err_t esp_coze_audio_player_start(void);

/**
 * @brief 停止音频播放任务
 * 
 * @return esp_err_t 
 */
esp_err_t esp_coze_audio_player_stop(void);

/**
 * @brief 销毁音频Flash存储
 * 
 * @param af 音频Flash结构体指针
 */
void esp_coze_audio_flash_deinit(esp_coze_audio_flash_t *af);

#ifdef __cplusplus
}
#endif
