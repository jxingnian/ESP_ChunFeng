/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2024-12-03 11:05:09
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-10 15:09:19
 * @FilePath: \ESP32-S3-Touch-LCD-1.46-Test\main\Audio_Driver\PCM5101.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

// 引入 ESP-IDF 日志库，用于调试输出
#include "esp_log.h" 
// ESP-IDF 错误检查宏
#include "esp_check.h" 
// ESP-Audio 播放器头文件
#include "audio_player.h" 
// GPIO 驱动头文件
#include "driver/gpio.h" 
// FreeRTOS 信号量头文件
#include "freertos/semphr.h" 

// SD 卡驱动头文件（如果需要的话）
#include "SD_MMC.h"

// I2S 外设编号，0 表示 I2S0
#define CONFIG_BSP_I2S_NUM 0

// I2S 各引脚定义（根据硬件原理图分配）
#define BSP_I2S_SCLK          (GPIO_NUM_48)   // I2S 串行时钟（BCLK）
#define BSP_I2S_MCLK          (GPIO_NUM_NC)   // I2S 主时钟（未连接）
#define BSP_I2S_LCLK          (GPIO_NUM_38)   // I2S 左右声道时钟（LRCK/WS）
#define BSP_I2S_DOUT          (GPIO_NUM_47)   // I2S 数据输出（DOUT）
#define BSP_I2S_DSIN          (GPIO_NUM_NC)   // I2S 数据输入（未连接）

// I2S GPIO 配置结构体宏，便于初始化 I2S 外设
#define BSP_I2S_GPIO_CFG       \
    {                          \
        .mclk = BSP_I2S_MCLK,  /* 主时钟引脚 */ \
        .bclk = BSP_I2S_SCLK,  /* 位时钟引脚 */ \
        .ws = BSP_I2S_LCLK,    /* 字选择引脚 */ \
        .dout = BSP_I2S_DOUT,  /* 数据输出引脚 */ \
        .din = BSP_I2S_DSIN,   /* 数据输入引脚 */ \
        .invert_flags = {      /* 时钟极性设置 */ \
            .mclk_inv = false, /* 主时钟不反相 */ \
            .bclk_inv = false, /* 位时钟不反相 */ \
            .ws_inv = false,   /* 字选择不反相 */ \
        },                     \
    }

// I2S 单声道双工配置宏，_sample_rate 为采样率
#define BSP_I2S_DUPLEX_MONO_CFG(_sample_rate)                                                         \
    {                                                                                                 \
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(_sample_rate),                                          /* 时钟配置 */ \
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO), /* 槽配置：16位单声道 */ \
        .gpio_cfg = BSP_I2S_GPIO_CFG,                                                                 /* GPIO 配置 */ \
    }

// 音量最大值定义（0~100）
#define Volume_MAX  100

#ifdef __cplusplus
extern "C" {
#endif

// 音乐播放控制相关全局变量和函数声明
extern bool Music_Next_Flag;      // 标志：是否切换到下一首
extern uint8_t Volume;            // 当前音量值（0~100）

// 音频初始化函数，需在主程序启动时调用
void Audio_Init(void);

// 播放指定目录下的音乐文件
// @param directory 目录路径
// @param fileName  文件名
void Play_Music(const char* directory, const char* fileName);

// 恢复音乐播放
void Music_resume(void);

// 暂停音乐播放
void Music_pause(void);

// 获取当前音乐总时长（单位：毫秒）
uint32_t Music_Duration(void);

// 获取当前已播放时长（单位：毫秒）
uint32_t Music_Elapsed(void);

// 获取当前音乐能量（如音量指示，单位可自定义）
uint16_t Music_Energy(void);

// 设置音量
// @param Volume 音量值（0~100）
void Volume_adjustment(uint8_t Volume);

#ifdef __cplusplus
}
#endif