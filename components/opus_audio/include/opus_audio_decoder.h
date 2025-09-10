/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-01-27 10:00:00
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-01-27 10:00:00
 * @FilePath: \esp-chunfeng\components\opus_audio\include\opus_audio_decoder.h
 * @Description: Opus音频解码器头文件
 *
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opus解码器配置结构体
 */
typedef struct {
    int sample_rate;        ///< 采样率 (8000, 12000, 16000, 24000, 48000)
    int channels;           ///< 声道数 (1或2)
    int max_frame_size;     ///< 最大帧大小（样本数）
} opus_audio_decoder_config_t;

/**
 * @brief Opus解码器句柄
 */
typedef struct opus_audio_decoder_t opus_audio_decoder_t;

/**
 * @brief 创建Opus解码器
 *
 * @param config 解码器配置
 * @return opus_audio_decoder_t* 解码器句柄，失败返回NULL
 */
opus_audio_decoder_t *opus_audio_decoder_create(const opus_audio_decoder_config_t *config);

/**
 * @brief 销毁Opus解码器
 *
 * @param decoder 解码器句柄
 */
void opus_audio_decoder_destroy(opus_audio_decoder_t *decoder);

/**
 * @brief 解码Opus音频数据
 *
 * @param decoder 解码器句柄
 * @param opus_data Opus编码的音频数据
 * @param opus_len Opus数据长度
 * @param pcm_output PCM输出缓冲区
 * @param pcm_max_samples PCM缓冲区最大样本数
 * @param pcm_decoded_samples 实际解码的样本数（输出参数）
 * @return esp_err_t
 *         - ESP_OK: 解码成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_ERR_INVALID_STATE: 解码器状态无效
 *         - ESP_FAIL: 解码失败
 */
esp_err_t opus_audio_decoder_decode(opus_audio_decoder_t *decoder,
                                   const uint8_t *opus_data,
                                   size_t opus_len,
                                   int16_t *pcm_output,
                                   size_t pcm_max_samples,
                                   size_t *pcm_decoded_samples);

/**
 * @brief 获取解码器配置
 *
 * @param decoder 解码器句柄
 * @param config 输出配置
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t opus_audio_decoder_get_config(const opus_audio_decoder_t *decoder,
                                       opus_audio_decoder_config_t *config);

/**
 * @brief 重置解码器状态
 *
 * @param decoder 解码器句柄
 * @return esp_err_t
 *         - ESP_OK: 成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t opus_audio_decoder_reset(opus_audio_decoder_t *decoder);

#ifdef __cplusplus
}
#endif
