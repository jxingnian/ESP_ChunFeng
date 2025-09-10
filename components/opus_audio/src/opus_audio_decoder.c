/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-01-27 10:00:00
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-01-27 10:00:00
 * @FilePath: \esp-chunfeng\components\opus_audio\src\opus_audio_decoder.c
 * @Description: Opus音频解码器实现
 *
 */
#include "opus_audio_decoder.h"
#include "opus.h"
#include "esp_log.h"
#include "esp_check.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "OPUS_AUDIO_DECODER";

/**
 * @brief Opus解码器结构体
 */
struct opus_audio_decoder_t {
    OpusDecoder *opus_decoder;              ///< Opus解码器句柄
    opus_audio_decoder_config_t config;     ///< 解码器配置
    bool initialized;                       ///< 是否已初始化
};

/**
 * @brief 验证采样率是否支持
 */
static bool is_valid_sample_rate(int sample_rate)
{
    return (sample_rate == 8000 || sample_rate == 12000 || 
            sample_rate == 16000 || sample_rate == 24000 || sample_rate == 48000);
}

/**
 * @brief 验证声道数是否支持
 */
static bool is_valid_channels(int channels)
{
    return (channels == 1 || channels == 2);
}

opus_audio_decoder_t *opus_audio_decoder_create(const opus_audio_decoder_config_t *config)
{
    if (!config) {
        ESP_LOGE(TAG, "配置参数不能为空");
        return NULL;
    }

    // 验证配置参数
    if (!is_valid_sample_rate(config->sample_rate)) {
        ESP_LOGE(TAG, "不支持的采样率: %d", config->sample_rate);
        return NULL;
    }

    if (!is_valid_channels(config->channels)) {
        ESP_LOGE(TAG, "不支持的声道数: %d", config->channels);
        return NULL;
    }

    if (config->max_frame_size <= 0) {
        ESP_LOGE(TAG, "无效的最大帧大小: %d", config->max_frame_size);
        return NULL;
    }

    // 分配解码器结构体内存
    opus_audio_decoder_t *decoder = calloc(1, sizeof(opus_audio_decoder_t));
    if (!decoder) {
        ESP_LOGE(TAG, "分配解码器内存失败");
        return NULL;
    }

    // 复制配置
    memcpy(&decoder->config, config, sizeof(opus_audio_decoder_config_t));

    // 创建Opus解码器
    int error = 0;
    decoder->opus_decoder = opus_decoder_create(config->sample_rate, config->channels, &error);
    if (error != OPUS_OK || !decoder->opus_decoder) {
        ESP_LOGE(TAG, "创建Opus解码器失败，错误码: %d", error);
        free(decoder);
        return NULL;
    }

    decoder->initialized = true;
    ESP_LOGI(TAG, "Opus解码器创建成功，采样率: %d, 声道数: %d", 
             config->sample_rate, config->channels);

    return decoder;
}

void opus_audio_decoder_destroy(opus_audio_decoder_t *decoder)
{
    if (!decoder) {
        return;
    }

    if (decoder->opus_decoder) {
        opus_decoder_destroy(decoder->opus_decoder);
        decoder->opus_decoder = NULL;
    }

    decoder->initialized = false;
    free(decoder);
    
    ESP_LOGI(TAG, "Opus解码器已销毁");
}

esp_err_t opus_audio_decoder_decode(opus_audio_decoder_t *decoder,
                                   const uint8_t *opus_data,
                                   size_t opus_len,
                                   int16_t *pcm_output,
                                   size_t pcm_max_samples,
                                   size_t *pcm_decoded_samples)
{
    if (!decoder || !decoder->initialized) {
        ESP_LOGE(TAG, "解码器未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    if (!opus_data || opus_len == 0) {
        ESP_LOGE(TAG, "Opus数据无效");
        return ESP_ERR_INVALID_ARG;
    }

    if (!pcm_output || pcm_max_samples == 0) {
        ESP_LOGE(TAG, "PCM输出缓冲区无效");
        return ESP_ERR_INVALID_ARG;
    }

    if (!pcm_decoded_samples) {
        ESP_LOGE(TAG, "输出参数无效");
        return ESP_ERR_INVALID_ARG;
    }

    // 调用Opus解码
    int frame_size = pcm_max_samples / decoder->config.channels;
    int decoded_samples = opus_decode(decoder->opus_decoder,
                                    opus_data,
                                    opus_len,
                                    pcm_output,
                                    frame_size,
                                    0);  // 不使用FEC

    if (decoded_samples < 0) {
        ESP_LOGE(TAG, "Opus解码失败，错误码: %d", decoded_samples);
        *pcm_decoded_samples = 0;
        return ESP_FAIL;
    }

    *pcm_decoded_samples = decoded_samples * decoder->config.channels;
    
    ESP_LOGD(TAG, "解码成功，输入: %d字节，输出: %d样本", 
             (int)opus_len, (int)*pcm_decoded_samples);

    return ESP_OK;
}

esp_err_t opus_audio_decoder_get_config(const opus_audio_decoder_t *decoder,
                                       opus_audio_decoder_config_t *config)
{
    if (!decoder || !config) {
        ESP_LOGE(TAG, "参数无效");
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(config, &decoder->config, sizeof(opus_audio_decoder_config_t));
    return ESP_OK;
}

esp_err_t opus_audio_decoder_reset(opus_audio_decoder_t *decoder)
{
    if (!decoder || !decoder->initialized) {
        ESP_LOGE(TAG, "解码器未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    // 重置Opus解码器状态
    int ret = opus_decoder_ctl(decoder->opus_decoder, OPUS_RESET_STATE);
    if (ret != OPUS_OK) {
        ESP_LOGE(TAG, "重置Opus解码器状态失败，错误码: %d", ret);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Opus解码器状态已重置");
    return ESP_OK;
}
