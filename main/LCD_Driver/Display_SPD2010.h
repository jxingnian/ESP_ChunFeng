#pragma once

// ESP-IDF 错误码相关头文件
#include "esp_err.h"
// ESP-IDF 日志输出头文件
#include "esp_log.h"
// 标准输入输出库
#include <stdio.h>
// 标准库，包含内存分配等
#include <stdlib.h>
// 字符串处理库
#include <string.h>
// FreeRTOS 主头文件
#include "freertos/FreeRTOS.h"
// FreeRTOS 任务管理头文件
#include "freertos/task.h"
// ESP32 GPIO 驱动头文件
#include "driver/gpio.h"
// ESP32 SPI 主机驱动头文件
#include "driver/spi_master.h"
// ESP32 定时器相关头文件
#include "esp_timer.h"
// ESP32 LCD 面板 IO 驱动头文件
#include "esp_lcd_panel_io.h"
// ESP32 LCD 面板 IO 接口定义
#include "esp_lcd_panel_io_interface.h"
// ESP32 中断分配相关头文件
#include "esp_intr_alloc.h"
// ESP32 LCD 面板操作相关头文件
#include "esp_lcd_panel_ops.h"
// ESP32 LCD 面板厂商相关头文件
#include "esp_lcd_panel_vendor.h"
// LVGL 图形库主头文件
#include "lvgl.h"
// ESP32 LEDC（PWM）驱动头文件
#include "driver/ledc.h"

// SPD2010 LCD 驱动专用头文件
#include "esp_lcd_spd2010.h"
// SPD2010 触摸驱动头文件
#include "Touch_SPD2010.h"
// LVGL 驱动适配头文件
#include "LVGL_Driver.h"
// TCA9554PWR 扩展IO驱动头文件
#include "TCA9554PWR.h"

// ===================== LCD 屏幕参数宏定义 =====================

// LCD 屏幕宽度（像素）
#define EXAMPLE_LCD_WIDTH                   (412)
// LCD 屏幕高度（像素）
#define EXAMPLE_LCD_HEIGHT                  (412)
// LCD 屏幕色深（每像素位数，16位=RGB565）
#define EXAMPLE_LCD_COLOR_BITS              (16)

// ===================== SPI 总线相关宏定义 =====================

// 默认 SPI 主机编号（ESP32-S3 通常为 SPI2_HOST）
#define ESP_PANEL_HOST_SPI_ID_DEFAULT       (SPI2_HOST)
// SPI 工作模式（0/1/2/3，通常为0）
#define ESP_PANEL_LCD_SPI_MODE              (0)                   // 0/1/2/3，通常设置为0
// SPI 时钟频率（单位Hz，80MHz，通常设置为40MHz）
#define ESP_PANEL_LCD_SPI_CLK_HZ            (80 * 1000 * 1000)    // 应为80M的整数分频，常用40M
// SPI 传输队列长度（通常为10）
#define ESP_PANEL_LCD_SPI_TRANS_QUEUE_SZ    (10)                  // 通常设置为10
// SPI 命令位宽（通常为32位）
#define ESP_PANEL_LCD_SPI_CMD_BITS          (32)                  // 通常设置为32
// SPI 参数位宽（通常为8位）
#define ESP_PANEL_LCD_SPI_PARAM_BITS        (8)                   // 通常设置为8

// ===================== SPI 引脚宏定义 =====================

// TE 信号引脚（Tearing Effect，屏幕同步用）
#define ESP_PANEL_LCD_SPI_IO_TE             (18)
// SPI SCK 时钟引脚
#define ESP_PANEL_LCD_SPI_IO_SCK            (40)
// SPI 数据线 D0
#define ESP_PANEL_LCD_SPI_IO_DATA0          (46)
// SPI 数据线 D1
#define ESP_PANEL_LCD_SPI_IO_DATA1          (45)
// SPI 数据线 D2
#define ESP_PANEL_LCD_SPI_IO_DATA2          (42)
// SPI 数据线 D3
#define ESP_PANEL_LCD_SPI_IO_DATA3          (41)
// SPI 片选 CS 引脚
#define ESP_PANEL_LCD_SPI_IO_CS             (21)
// LCD 复位引脚（如用扩展IO，设为-1，实际由EXIO2控制）
#define EXAMPLE_LCD_PIN_NUM_RST             (-1)    // 由 EXIO2 控制
// LCD 背光控制引脚（PWM输出）
#define EXAMPLE_LCD_PIN_NUM_BK_LIGHT        (5)

// ===================== 背光控制相关宏定义 =====================

// 背光点亮时的电平（1=高电平点亮，0=低电平点亮）
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL       (1)
// 背光关闭时的电平（取反）
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL      !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL

// SPI 最大单次传输字节数
#define ESP_PANEL_HOST_SPI_MAX_TRANSFER_SIZE   (2048)

// ===================== LEDC（PWM）背光调光相关宏定义 =====================

// LEDC 定时器编号（用于背光PWM）
#define LEDC_HS_TIMER          LEDC_TIMER_0
// LEDC 工作模式（低速模式）
#define LEDC_LS_MODE           LEDC_LOW_SPEED_MODE
// LEDC 通道0对应的GPIO（即背光引脚）
#define LEDC_HS_CH0_GPIO       EXAMPLE_LCD_PIN_NUM_BK_LIGHT
// LEDC 通道编号
#define LEDC_HS_CH0_CHANNEL    LEDC_CHANNEL_0
// LEDC 测试用占空比
#define LEDC_TEST_DUTY         (4000)
// LEDC 分辨率（13位，最大8191）
#define LEDC_ResolutionRatio   LEDC_TIMER_13_BIT
// LEDC 最大占空比值
#define LEDC_MAX_Duty          ((1 << LEDC_ResolutionRatio) - 1)
// 背光亮度最大值（百分比，0~100）
#define Backlight_MAX          100      

#ifdef __cplusplus
extern "C" {
#endif

// ===================== 全局变量声明 =====================

// LCD 面板句柄（全局唯一）
extern esp_lcd_panel_handle_t panel_handle;
// 当前背光亮度值（0~100）
extern uint8_t LCD_Backlight;

// ===================== LCD/背光相关函数声明 =====================

/**
 * @brief SPD2010 LCD 屏幕初始化（底层驱动初始化）
 * 
 * 初始化 SPI、LCD 面板、背光PWM等，通常由 LCD_Init 间接调用。
 */
void SPD2010_Init(void);

/**
 * @brief LCD 屏幕初始化函数
 * 
 * 必须在主函数中调用一次，完成屏幕、背光、LVGL等初始化。
 */
void LCD_Init(void);

/**
 * @brief LCD 屏幕区域填充函数
 * 
 * 向指定区域（窗口）写入像素数据。
 * @param Xstart 区域起始X坐标
 * @param Ystart 区域起始Y坐标
 * @param Xend   区域结束X坐标
 * @param Yend   区域结束Y坐标
 * @param color  指向像素数据的指针（RGB565格式）
 */
void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t* color);

/**
 * @brief 背光PWM初始化
 * 
 * 已在 LCD_Init 内部调用，用户无需单独调用。
 */
void Backlight_Init(void);

/**
 * @brief 设置LCD背光亮度
 * 
 * 调用此函数可调节背光亮度，Light取值范围0~100（百分比）。
 * @param Light 背光亮度（0=最暗，100=最亮）
 */
void Set_Backlight(uint8_t Light);

#ifdef __cplusplus
}
#endif
