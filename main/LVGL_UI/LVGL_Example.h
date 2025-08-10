/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2024-12-03 11:05:10
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-10 13:43:24
 * @FilePath: \ESP32-S3-Touch-LCD-1.46-Test\main\LVGL_UI\LVGL_Example.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once  // 防止头文件被多次包含

// 引入LVGL核心库
#include "lvgl.h"
// 引入LVGL官方演示示例
#include "demos/lv_demos.h"

// 引入自定义的LVGL驱动头文件
#include "LVGL_Driver.h"
// 引入PCF85063实时时钟芯片驱动头文件
#include "PCF85063.h"
// 引入QMI8658传感器驱动头文件（如加速度计/陀螺仪）
#include "QMI8658.h"
// 引入SD卡MMC驱动头文件
#include "SD_MMC.h"
// 引入电池管理驱动头文件
#include "BAT_Driver.h"
// 引入无线通信相关头文件
#include "Wireless.h"
#ifdef __cplusplus
extern "C" {
#endif
// LVGL定时器周期定义，单位为毫秒
#define EXAMPLE1_LVGL_TICK_PERIOD_MS  1000  // 1秒定时周期

/**
 * @brief 背光调节事件回调函数
 * 
 * 当背光调节相关事件发生时被LVGL调用
 * @param e LVGL事件指针
 */
void Backlight_adjustment_event_cb(lv_event_t * e);

/**
 * @brief LVGL示例1的初始化函数
 * 
 * 用于初始化和展示LVGL的第一个示例界面
 */
void Lvgl_Example1(void);

/**
 * @brief 设置背光亮度
 * 
 * @param Backlight 背光亮度值（0~255）
 */
void LVGL_Backlight_adjustment(uint8_t Backlight);

#ifdef __cplusplus
}
#endif
