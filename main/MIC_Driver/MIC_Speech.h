/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2024-12-03 11:05:10
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-10 12:34:37
 * @FilePath: \ESP32-S3-Touch-LCD-1.46-Test\main\MIC_Driver\MIC_Speech.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

// 引入 ESP32 语音前端（AFE）接口头文件
#include "esp_afe_sr_iface.h"
// 引入 ESP32 SDK 配置处理头文件
#include "esp_process_sdkconfig.h"
// 引入模型路径定义头文件
#include "model_path.h"
// 引入显示屏驱动头文件
#include "Display_SPD2010.h"
// 引入 LVGL 音乐播放器头文件
#include "LVGL_Music.h"

#ifdef __cplusplus
extern "C" {
#endif

// I2S 配置宏定义，便于初始化 I2S 外设
// 参数：sample_rate 采样率，channel_fmt 通道格式，bits_per_chan 每通道位宽
#define I2S_CONFIG_DEFAULT(sample_rate, channel_fmt, bits_per_chan) { \
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate), /* 时钟配置，使用标准宏 */ \
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(bits_per_chan, channel_fmt), /* 槽配置，飞利浦标准格式 */ \
        .gpio_cfg = { \
            .mclk = GPIO_NUM_NC, /* 主时钟引脚未连接 */ \
            .bclk = GPIO_NUM_15, /* 位时钟引脚为 GPIO15 */ \
            .ws   = GPIO_NUM_2,  /* 字选择引脚为 GPIO2 */ \
            .dout = GPIO_NUM_NC, /* 数据输出引脚未连接 */ \
            .din  = GPIO_NUM_39, /* 数据输入引脚为 GPIO39 */ \
            .invert_flags = { \
                .mclk_inv = false, /* 主时钟不反相 */ \
                .bclk_inv = false, /* 位时钟不反相 */ \
                .ws_inv   = false, /* 字选择不反相 */ \
            }, \
        }, \
    }

// 语音命令枚举类型
typedef enum
{
    COMMAND_TIMEOUT = -2,         // 命令超时
    COMMAND_NOT_DETECTED = -1,    // 未检测到命令

    COMMAND_ID1 = 0,              // 命令1
    COMMAND_ID2 = 1,              // 命令2
    COMMAND_ID3 = 2,              // 命令3
    COMMAND_ID4 = 3,              // 命令4
    COMMAND_ID5 = 4,              // 命令5
    COMMAND_ID6 = 5,              // 命令6
} command_word_t;

// 语音识别应用结构体
typedef struct {
    const esp_afe_sr_iface_t *afe_handle; // 语音前端处理句柄
    esp_afe_sr_data_t *afe_data;          // 语音前端数据指针
    srmodel_list_t *models;               // 语音识别模型列表
    bool detected;                        // 是否检测到命令
    command_word_t command;               // 检测到的命令类型
} AppSpeech;

// 语音识别初始化函数声明
void MIC_Speech_init();

#ifdef __cplusplus
}
#endif
