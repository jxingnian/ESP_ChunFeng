/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-17 11:50:07
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-25 15:12:34
 * @FilePath: \esp-brookesia-chunfeng\main\Audio\audio_hal.h
 * @Description:
 *
 */
/**
 * 基础音频 HAL: I2S 麦克风采集 + 扬声器播放 + 回环测试 (ESP-IDF 5.5)
 * 实现最小功能：初始化、读取麦克风 16bit PCM、写入扬声器、启动/停止回环。
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"

#ifdef __cplusplus
extern "C" {
#endif

// 引脚配置（按硬件修改）
#define AUDIO_SPK_PORT              0
#define AUDIO_SPK_BCLK_GPIO         (GPIO_NUM_48)
#define AUDIO_SPK_LRCK_GPIO         (GPIO_NUM_38)
#define AUDIO_SPK_DATA_GPIO         (GPIO_NUM_47)
#define AUDIO_SPK_MCLK_GPIO         (GPIO_NUM_NC)

#define AUDIO_MIC_PORT              1
#define AUDIO_MIC_BCLK_GPIO         (GPIO_NUM_15)
#define AUDIO_MIC_LRCK_GPIO         (GPIO_NUM_2)
#define AUDIO_MIC_DATA_GPIO         (GPIO_NUM_39)
#define AUDIO_MIC_MCLK_GPIO         (GPIO_NUM_NC)

#define AUDIO_SAMPLE_RATE_HZ        16000
#define AUDIO_BITS_PER_SAMPLE       I2S_DATA_BIT_WIDTH_16BIT
#define AUDIO_MIC_BITS_CAPTURE      I2S_DATA_BIT_WIDTH_32BIT

#define AUDIO_VOLUME_MAX            100

esp_err_t audio_hal_init(void);
esp_err_t audio_hal_write(const int16_t *samples, size_t sample_count, uint32_t timeout_ms);
esp_err_t audio_hal_read(int16_t *out_samples, size_t sample_count, size_t *out_got, uint32_t timeout_ms);
void audio_hal_set_volume(uint8_t vol);
uint8_t audio_hal_get_volume(void);
esp_err_t audio_hal_loopback_start(size_t frame_samples);
esp_err_t audio_hal_loopback_stop(void);
bool audio_hal_loopback_running(void);

#ifdef __cplusplus
}
#endif
