/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-28 15:16:27
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-28 15:28:56
 * @FilePath: \esp-chunfeng\main\LCD_Driver\Display_SPD2010.h
 * @Description: SPD2010 LCD驱动头文件，包含LCD参数、SPI配置、背光控制、全局变量及相关函数声明
 */

#pragma once

// ESP-IDF及标准库头文件
#include "esp_err.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_io_interface.h"
#include "esp_intr_alloc.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "lvgl.h"
#include "driver/ledc.h"

#include "esp_lcd_spd2010.h"
#include "Touch_SPD2010.h"
#include "LVGL_Driver.h"
#include "TCA9554PWR.h"

// LCD显示屏参数定义
#define EXAMPLE_LCD_WIDTH                   (412)                 // LCD屏幕宽度，单位：像素
#define EXAMPLE_LCD_HEIGHT                  (412)                 // LCD屏幕高度，单位：像素
#define EXAMPLE_LCD_COLOR_BITS              (16)                  // 颜色位深，16位表示RGB565格式

// SPI总线配置参数
#define ESP_PANEL_HOST_SPI_ID_DEFAULT       (SPI2_HOST)           // 使用SPI2主机接口
#define ESP_PANEL_LCD_SPI_MODE              (0)                   // SPI模式，0/1/2/3，通常设置为0
#define ESP_PANEL_LCD_SPI_CLK_HZ            (80 * 1000 * 1000)    // SPI时钟频率80MHz，应为80M的整数除数，通常设置为40M
#define ESP_PANEL_LCD_SPI_TRANS_QUEUE_SZ    (10)                  // SPI传输队列大小，通常设置为10
#define ESP_PANEL_LCD_SPI_CMD_BITS          (32)                  // SPI命令位数，通常设置为32位
#define ESP_PANEL_LCD_SPI_PARAM_BITS        (8)                   // SPI参数位数，通常设置为8位

// LCD SPI接口引脚定义
#define ESP_PANEL_LCD_SPI_IO_TE             (18)                  // 撕裂效应信号引脚，用于同步显示
#define ESP_PANEL_LCD_SPI_IO_SCK            (40)                  // SPI时钟信号引脚
#define ESP_PANEL_LCD_SPI_IO_DATA0          (46)                  // SPI数据线0引脚（QSPI模式下的D0）
#define ESP_PANEL_LCD_SPI_IO_DATA1          (45)                  // SPI数据线1引脚（QSPI模式下的D1）
#define ESP_PANEL_LCD_SPI_IO_DATA2          (42)                  // SPI数据线2引脚（QSPI模式下的D2）
#define ESP_PANEL_LCD_SPI_IO_DATA3          (41)                  // SPI数据线3引脚（QSPI模式下的D3）
#define ESP_PANEL_LCD_SPI_IO_CS             (21)                  // SPI片选信号引脚
#define EXAMPLE_LCD_PIN_NUM_RST             (-1)                  // LCD复位引脚，-1表示通过EXIO2扩展IO控制
#define EXAMPLE_LCD_PIN_NUM_BK_LIGHT        (5)                   // LCD背光控制引脚

// 背光控制电平定义
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL       (1)                   // 背光开启时的电平状态（高电平有效）
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  // 背光关闭时的电平状态

// SPI传输配置
#define ESP_PANEL_HOST_SPI_MAX_TRANSFER_SIZE   (2048)             // SPI单次传输的最大字节数

// LEDC PWM背光控制配置
#define LEDC_HS_TIMER          LEDC_TIMER_0                       // 使用LEDC定时器0
#define LEDC_LS_MODE           LEDC_LOW_SPEED_MODE                // LEDC低速模式
#define LEDC_HS_CH0_GPIO       EXAMPLE_LCD_PIN_NUM_BK_LIGHT       // PWM输出引脚，连接到背光控制引脚
#define LEDC_HS_CH0_CHANNEL    LEDC_CHANNEL_0                     // 使用LEDC通道0
#define LEDC_TEST_DUTY         (4000)                             // PWM占空比测试值
#define LEDC_ResolutionRatio   LEDC_TIMER_13_BIT                  // PWM分辨率为13位（0-8191）
#define LEDC_MAX_Duty          ((1 << LEDC_ResolutionRatio) - 1)  // PWM最大占空比值（8191）
#define Backlight_MAX   100                                       // 背光亮度最大值（百分比）

// 全局变量声明
extern esp_lcd_panel_handle_t panel_handle;                      // LCD面板句柄，用于LCD操作
extern uint8_t LCD_Backlight;                                    // 当前背光亮度值（0-100）

// 函数声明

/**
 * @brief SPD2010显示驱动初始化
 * @note 初始化SPD2010显示控制器的基本配置
 */
void SPD2010_Init();

/**
 * @brief LCD显示屏初始化函数
 * @note 必须在主函数中调用此函数来初始化屏幕！！！！！
 *       包含SPI接口初始化、LCD控制器初始化、背光初始化等
 */
void LCD_Init(void);

/**
 * @brief 在LCD指定区域显示颜色数据
 * @param Xstart 起始X坐标
 * @param Ystart 起始Y坐标
 * @param Xend   结束X坐标
 * @param Yend   结束Y坐标
 * @param color  颜色数据指针，指向RGB565格式的颜色数组
 * @note 用于在指定矩形区域内填充颜色数据
 */
void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t *color);

/**
 * @brief 初始化LCD背光PWM控制
 * @note 此函数已在LCD_Init函数中调用，通常不需要单独调用
 *       配置LEDC PWM用于背光亮度控制
 */
void Backlight_Init(void);

/**
 * @brief 设置LCD背光亮度
 * @param Light 背光亮度值，范围0-100（0为最暗，100为最亮）
 * @note 通过PWM控制背光亮度，实现平滑的亮度调节
 */
void Set_Backlight(uint8_t Light);
