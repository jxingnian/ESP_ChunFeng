/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-17 11:50:17
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-28 15:57:17
 * @FilePath: \esp-chunfeng\main\Audio\audio_hal.c
 * @Description: 基础音频 HAL 实现
 *
 */
#include "audio_hal.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "AUDIO_HAL";
static i2s_chan_handle_t s_tx = NULL; // 扬声器通道句柄
static i2s_chan_handle_t s_rx = NULL; // 麦克风通道句柄
static bool s_inited = false;         // HAL 是否已初始化
static uint8_t s_volume = 95;         // 软件音量（0~100）
static TaskHandle_t s_loop_task = NULL;

// 音频环回任务栈 - 放在PSRAM
#define AUDIO_LOOP_STACK_SIZE (3 * 1024 / sizeof(StackType_t))
static EXT_RAM_BSS_ATTR StackType_t audio_loop_stack[AUDIO_LOOP_STACK_SIZE];
static StaticTask_t audio_loop_task_buffer; // 环回任务句柄
static size_t s_loop_frame = 0;         // 环回每帧采样数
static bool s_loop_running = false;     // 环回是否运行中

/**
 * @brief 创建扬声器（TX）通道
 * @return ESP_OK 成功，否则返回错误码
 */
static esp_err_t create_tx_channel(void)
{
    // 配置 I2S 通道参数
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(AUDIO_SPK_PORT, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    // 创建 TX 通道
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &s_tx, NULL), TAG, "new tx channel failed");
    // 配置 I2S 标准模式参数
    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE_HZ),
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(AUDIO_BITS_PER_SAMPLE, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = AUDIO_SPK_MCLK_GPIO,
            .bclk = AUDIO_SPK_BCLK_GPIO,
            .ws   = AUDIO_SPK_LRCK_GPIO,
            .dout = AUDIO_SPK_DATA_GPIO,
            .din  = GPIO_NUM_NC,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    // 说明：保持立体声配置，但上层提供的是单声道采样，我们在写入前复制到 L/R，避免把两个连续时间点误当成 L/R 导致播放速率减半、音调下降。
    // 初始化 TX 通道为标准模式
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_tx, &std_cfg), TAG, "init tx std mode failed");
    // 使能 TX 通道
    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_tx), TAG, "enable tx failed");
    return ESP_OK;
}

/**
 * @brief 创建麦克风（RX）通道
 * @return ESP_OK 成功，否则返回错误码
 */
static esp_err_t create_rx_channel(void)
{
    // 配置 I2S 通道参数
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(AUDIO_MIC_PORT, I2S_ROLE_MASTER);
    // 创建 RX 通道
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, NULL, &s_rx), TAG, "new rx channel failed");
    // 配置 I2S 标准模式参数
    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE_HZ),
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(AUDIO_MIC_BITS_CAPTURE, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = AUDIO_MIC_MCLK_GPIO,
            .bclk = AUDIO_MIC_BCLK_GPIO,
            .ws   = AUDIO_MIC_LRCK_GPIO,
            .dout = GPIO_NUM_NC,
            .din  = AUDIO_MIC_DATA_GPIO,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT; // 仅取一路（右声道）
    // 初始化 RX 通道为标准模式
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_rx, &std_cfg), TAG, "init rx std mode failed");
    // 使能 RX 通道
    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_rx), TAG, "enable rx failed");
    return ESP_OK;
}

/**
 * @brief 初始化音频 HAL
 * @return ESP_OK 成功，否则返回错误码
 */
esp_err_t audio_hal_init(void)
{
    if (s_inited) return ESP_OK; // 已初始化直接返回
    ESP_RETURN_ON_ERROR(create_tx_channel(), TAG, "tx channel create err");
    ESP_RETURN_ON_ERROR(create_rx_channel(), TAG, "rx channel create err");
    s_inited = true;
    ESP_LOGI(TAG, "Audio HAL init OK sr=%d", AUDIO_SAMPLE_RATE_HZ);
    return ESP_OK;
}

/**
 * @brief 设置音量（0~AUDIO_VOLUME_MAX）
 * @param vol 音量值
 */
void audio_hal_set_volume(uint8_t vol)
{
    if (vol > AUDIO_VOLUME_MAX) vol = AUDIO_VOLUME_MAX;
    s_volume = vol;
}

/**
 * @brief 获取当前音量
 * @return 当前音量值
 */
uint8_t audio_hal_get_volume(void)
{
    return s_volume;
}

/**
 * @brief 向扬声器写入音频数据
 * @param samples 音频采样数据指针
 * @param sample_count 采样数
 * @param timeout_ms 写入超时时间（毫秒）
 * @return ESP_OK 成功，否则返回错误码
 */
esp_err_t audio_hal_write(const int16_t *samples, size_t sample_count, uint32_t timeout_ms)
{
    if (!s_inited || !s_tx) return ESP_ERR_INVALID_STATE;
    if (!samples || sample_count == 0) return ESP_ERR_INVALID_ARG;
    // 输入视为单声道，复制到左右 -> 立体声帧数 = sample_count
    size_t stereo_samples = sample_count * 2; // L+R
    size_t bytes = stereo_samples * sizeof(int16_t);
    int16_t *tmp = (int16_t *)malloc(bytes);
    if (!tmp) return ESP_ERR_NO_MEM;
    float factor = s_volume / 100.0f;
    for (size_t i = 0; i < sample_count; ++i) {
        int16_t v = samples[i];
        if (factor < 0.999f) v = (int16_t)(v * factor);
        tmp[2 * i] = v;    // Left
        tmp[2 * i + 1] = v; // Right
    }
    size_t written = 0;
    esp_err_t ret = i2s_channel_write(s_tx, (const char *)tmp, bytes, &written, timeout_ms);
    free(tmp);
    return ret;
}

/**
 * @brief 从麦克风读取音频数据
 * @param out_samples 输出采样缓冲区
 * @param sample_count 期望采样数
 * @param out_got 实际读取采样数指针（可为NULL）
 * @param timeout_ms 读取超时时间（毫秒）
 * @return ESP_OK 成功，否则返回错误码
 */
esp_err_t audio_hal_read(int16_t *out_samples, size_t sample_count, size_t *out_got, uint32_t timeout_ms)
{
    if (!s_inited || !s_rx) return ESP_ERR_INVALID_STATE;
    if (!out_samples || sample_count == 0) return ESP_ERR_INVALID_ARG;
    size_t bytes32 = sample_count * sizeof(int32_t);
    int32_t *temp = (int32_t *)malloc(bytes32); // 临时缓冲区
    if (!temp) return ESP_ERR_NO_MEM;
    size_t bytes_read = 0;
    esp_err_t ret = i2s_channel_read(s_rx, temp, bytes32, &bytes_read, timeout_ms);
    size_t got = bytes_read / sizeof(int32_t);
    // 数据转换：32位右移14位转为16位
    for (size_t i = 0; i < got; ++i)
        out_samples[i] = (int16_t)(temp[i] >> 14);
    free(temp);
    if (out_got) *out_got = got;
    return ret;
}

/**
 * @brief 音频环回任务（将麦克风数据直接写入扬声器）
 * @param arg 未使用
 */
static void loop_task(void *arg)
{
    const size_t frame = s_loop_frame;
    int16_t *buf = (int16_t *)malloc(frame * sizeof(int16_t));
    if (!buf) {
        s_loop_running = false;
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Loopback running frame=%u", (unsigned)frame);
    while (s_loop_running) {
        size_t got = 0;
        // 读取麦克风数据并写入扬声器
        if (audio_hal_read(buf, frame, &got, 100) == ESP_OK && got) {
            audio_hal_write(buf, got, 100);
        }
    }
    free(buf);
    ESP_LOGI(TAG, "Loopback end");
    vTaskDelete(NULL);
}

/**
 * @brief 启动音频环回（麦克风->扬声器）
 * @param frame_samples 每帧采样数
 * @return ESP_OK 成功，否则返回错误码
 */
esp_err_t audio_hal_loopback_start(size_t frame_samples)
{
    if (!s_inited) return ESP_ERR_INVALID_STATE;
    if (s_loop_running) return ESP_ERR_INVALID_STATE;
    if (frame_samples == 0) return ESP_ERR_INVALID_ARG;
    s_loop_frame = frame_samples;
    s_loop_running = true;
    // 创建环回任务 - 使用静态任务，栈在PSRAM
    s_loop_task = xTaskCreateStatic(
        loop_task,                  // 任务函数
        "audio_loop",               // 任务名称
        AUDIO_LOOP_STACK_SIZE,      // 栈大小
        NULL,                       // 任务参数
        5,                          // 优先级
        audio_loop_stack,           // 栈数组(PSRAM)
        &audio_loop_task_buffer     // 任务控制块(内部RAM)
    );
    
    if (s_loop_task == NULL) {
        s_loop_running = false;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

/**
 * @brief 停止音频环回
 * @return ESP_OK 成功，否则返回错误码
 */
esp_err_t audio_hal_loopback_stop(void)
{
    if (!s_loop_running) return ESP_ERR_INVALID_STATE;
    s_loop_running = false;
    return ESP_OK;
}

/**
 * @brief 查询环回是否运行中
 * @return true: 运行中, false: 未运行
 */
bool audio_hal_loopback_running(void)
{
    return s_loop_running;
}
