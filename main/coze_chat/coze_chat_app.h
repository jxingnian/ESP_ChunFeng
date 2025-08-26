/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Initialize the Coze chat application
 *
 * @return
 *       - ESP_OK  On success
 *       - Other   Appropriate esp_err_t error code on failure
 */
esp_err_t coze_chat_app_init(void);

/**
 * @brief  Send text message to Coze
 *
 * @param message Text message to send
 * @return
 *       - ESP_OK  On success
 *       - Other   Appropriate esp_err_t error code on failure
 */
esp_err_t coze_chat_app_send_text(const char *message);

/**
 * @brief  Send audio data to Coze
 *
 * @param audio_data Audio data buffer
 * @param data_len Length of audio data
 * @return
 *       - ESP_OK  On success
 *       - Other   Appropriate esp_err_t error code on failure
 */
esp_err_t coze_chat_app_send_audio(const uint8_t *audio_data, size_t data_len);

/**
 * @brief  Start speech input
 *
 * @return
 *       - ESP_OK  On success
 *       - Other   Appropriate esp_err_t error code on failure
 */
esp_err_t coze_chat_app_start_speech(void);

/**
 * @brief  Stop speech input
 *
 * @return
 *       - ESP_OK  On success
 *       - Other   Appropriate esp_err_t error code on failure
 */
esp_err_t coze_chat_app_stop_speech(void);

/**
 * @brief  Get connection status
 *
 * @return
 *       - true   Connected
 *       - false  Disconnected
 */
bool coze_chat_app_is_connected(void);

/**
 * @brief  Interrupt current conversation
 *
 * @return
 *       - ESP_OK  On success
 *       - Other   Appropriate esp_err_t error code on failure
 */
esp_err_t coze_chat_app_interrupt(void);

/**
 * @brief  Deinitialize the Coze chat application
 *
 * @return
 *       - ESP_OK  On success
 *       - Other   Appropriate esp_err_t error code on failure
 */
esp_err_t coze_chat_app_deinit(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
