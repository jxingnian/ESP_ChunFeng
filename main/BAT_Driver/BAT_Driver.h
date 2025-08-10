#pragma once

// 引入 ESP-IDF 日志库，用于调试输出
#include "esp_log.h"
// 引入 ADC 单次采样模式头文件
#include "esp_adc/adc_oneshot.h"
// 引入 ADC 校准相关头文件
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#ifdef __cplusplus
extern "C" {
#endif  

/*---------------------------------------------------------------
        ADC 通用宏定义
---------------------------------------------------------------*/
// ADC1 通道选择（此处选择通道7，对应 GPIO8）
#define EXAMPLE_ADC1_CHAN       ADC_CHANNEL_7           // ADC1 通道7，硬件引脚为 GPIO8
// ADC 衰减设置（12dB 衰减，输入电压范围更大，适合测量高于 1.1V 的电压）
#define EXAMPLE_ADC_ATTEN       ADC_ATTEN_DB_12         // 12dB 衰减，最大输入电压约 3.3V

// 电压测量修正系数（用于校准 ADC 测量误差，实际电压 = 测量值 × Measurement_offset）
#define Measurement_offset 0.990476  

// 电池模拟电压全局变量，存储最新一次测量的电压值（单位：伏特）
extern float BAT_analogVolts;

// 电池检测初始化函数声明，需在主程序初始化时调用
void BAT_Init(void);

// 获取当前电池电压值的函数声明，返回值为实际电压（单位：伏特）
float BAT_Get_Volts(void);

#ifdef __cplusplus
}
#endif
