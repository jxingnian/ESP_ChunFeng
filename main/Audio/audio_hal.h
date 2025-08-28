/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-17 11:50:07
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-27 20:23:18
 * @FilePath: \esp-brookesia-chunfeng\main\Audio\audio_hal.h
 * @Description: 音频硬件抽象层头文件，提供I2S麦克风采集和扬声器播放功能
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

// 扬声器I2S引脚配置（按硬件修改）
#define AUDIO_SPK_PORT              0                   ///< 扬声器使用的I2S端口号
#define AUDIO_SPK_BCLK_GPIO         (GPIO_NUM_48)       ///< 扬声器位时钟引脚
#define AUDIO_SPK_LRCK_GPIO         (GPIO_NUM_38)       ///< 扬声器左右声道时钟引脚
#define AUDIO_SPK_DATA_GPIO         (GPIO_NUM_47)       ///< 扬声器数据输出引脚
#define AUDIO_SPK_MCLK_GPIO         (GPIO_NUM_NC)       ///< 扬声器主时钟引脚（未连接）

// 麦克风I2S引脚配置（按硬件修改）
#define AUDIO_MIC_PORT              1                   ///< 麦克风使用的I2S端口号
#define AUDIO_MIC_BCLK_GPIO         (GPIO_NUM_15)       ///< 麦克风位时钟引脚
#define AUDIO_MIC_LRCK_GPIO         (GPIO_NUM_2)        ///< 麦克风左右声道时钟引脚
#define AUDIO_MIC_DATA_GPIO         (GPIO_NUM_39)       ///< 麦克风数据输入引脚
#define AUDIO_MIC_MCLK_GPIO         (GPIO_NUM_NC)       ///< 麦克风主时钟引脚（未连接）

// 音频参数配置
#define AUDIO_SAMPLE_RATE_HZ        16000               ///< 音频采样率（16kHz）
#define AUDIO_BITS_PER_SAMPLE       I2S_DATA_BIT_WIDTH_16BIT  ///< 音频位深度（16位）
#define AUDIO_MIC_BITS_CAPTURE      I2S_DATA_BIT_WIDTH_32BIT  ///< 麦克风采集位深度（32位）

#define AUDIO_VOLUME_MAX            100                 ///< 最大音量值

/**
 * @brief 初始化音频HAL
 *
 * 初始化I2S扬声器和麦克风接口，配置相关GPIO和时钟参数
 *
 * @return esp_err_t
 *         - ESP_OK: 初始化成功
 *         - 其他: 初始化失败的错误码
 */
esp_err_t audio_hal_init(void);

/**
 * @brief 向扬声器写入音频数据
 *
 * @param samples 16位PCM音频样本数据指针
 * @param sample_count 样本数量
 * @param timeout_ms 超时时间（毫秒）
 * @return esp_err_t
 *         - ESP_OK: 写入成功
 *         - ESP_ERR_TIMEOUT: 写入超时
 *         - 其他: 写入失败的错误码
 */
esp_err_t audio_hal_write(const int16_t *samples, size_t sample_count, uint32_t timeout_ms);

/**
 * @brief 从麦克风读取音频数据
 *
 * @param out_samples 输出缓冲区，存储16位PCM音频样本
 * @param sample_count 请求读取的样本数量
 * @param out_got 实际读取到的样本数量
 * @param timeout_ms 超时时间（毫秒）
 * @return esp_err_t
 *         - ESP_OK: 读取成功
 *         - ESP_ERR_TIMEOUT: 读取超时
 *         - 其他: 读取失败的错误码
 */
esp_err_t audio_hal_read(int16_t *out_samples, size_t sample_count, size_t *out_got, uint32_t timeout_ms);

/**
 * @brief 设置扬声器音量
 *
 * @param vol 音量值（0-100）
 */
void audio_hal_set_volume(uint8_t vol);

/**
 * @brief 获取当前扬声器音量
 *
 * @return uint8_t 当前音量值（0-100）
 */
uint8_t audio_hal_get_volume(void);

/**
 * @brief 启动音频回环测试
 *
 * 启动麦克风到扬声器的音频回环，用于测试音频链路
 *
 * @param frame_samples 每帧样本数量
 * @return esp_err_t
 *         - ESP_OK: 启动成功
 *         - 其他: 启动失败的错误码
 */
esp_err_t audio_hal_loopback_start(size_t frame_samples);

/**
 * @brief 停止音频回环测试
 *
 * @return esp_err_t
 *         - ESP_OK: 停止成功
 *         - 其他: 停止失败的错误码
 */
esp_err_t audio_hal_loopback_stop(void);

/**
 * @brief 检查音频回环是否正在运行
 *
 * @return true 回环正在运行
 * @return false 回环未运行
 */
bool audio_hal_loopback_running(void);

#ifdef __cplusplus
}
#endif
