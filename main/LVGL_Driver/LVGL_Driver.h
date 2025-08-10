/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-10 15:48:03
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-10 15:48:49
 * @FilePath: \esp-brookesia-chunfeng\main\LVGL_Driver\LVGL_Driver.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"    // FreeRTOS 主头文件
#include "freertos/task.h"        // FreeRTOS 任务管理头文件
#include "esp_timer.h"            // ESP32 定时器相关头文件
#include "esp_err.h"              // ESP-IDF 错误码相关头文件
#include "esp_log.h"              // ESP-IDF 日志输出头文件
#include "lvgl.h"                 // LVGL 图形库主头文件
#include "demos/lv_demos.h"       // LVGL 官方演示示例头文件

#include "Display_SPD2010.h"      // SPD2010 显示屏驱动头文件

#ifdef __cplusplus
extern "C" {
#endif

// ===================== LVGL 缓冲区长度宏定义 =====================
// LVGL 显存缓冲区长度（单位：像素数），此处为屏幕总像素的1/20，适用于DMA传输优化
#define LVGL_BUF_LEN  (EXAMPLE_LCD_WIDTH * EXAMPLE_LCD_HEIGHT / 20)

// LVGL 定时节拍周期（单位：毫秒），用于驱动LVGL内部定时器
#define EXAMPLE_LVGL_TICK_PERIOD_MS    2

// ===================== LVGL 相关全局变量声明 =====================

// LVGL 显示缓冲区对象（draw buffer），用于存放待刷新的像素数据
extern lv_disp_draw_buf_t disp_buf;          // 内部图形缓冲区（draw buffer）对象

// LVGL 显示驱动对象，包含回调函数等驱动参数
extern lv_disp_drv_t disp_drv;               // 显示驱动结构体

// LVGL 显示设备对象指针，代表一个物理屏幕
extern lv_disp_t *disp;                      // 显示设备指针

// ===================== LVGL 相关回调函数声明 =====================

/**
 * @brief LVGL 刷新回调函数
 * 
 * 用于将LVGL渲染好的区域像素数据刷新到物理屏幕。
 * 该函数会在LVGL需要刷新屏幕时被自动调用。
 * 
 * @param drv       LVGL 显示驱动指针
 * @param area      需要刷新的屏幕区域
 * @param color_map 区域内的像素颜色数据
 */
void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);

/**
 * @brief LVGL 屏幕旋转/参数更新回调
 * 
 * 当LVGL驱动参数（如旋转方向）发生变化时调用，用于同步屏幕和触摸的旋转状态。
 * 
 * @param drv LVGL 显示驱动指针
 */
void example_lvgl_port_update_callback(lv_disp_drv_t *drv);

/**
 * @brief LVGL 定时节拍增加函数
 * 
 * 用于定时器回调中周期性地增加LVGL内部tick计数，驱动动画、超时等功能。
 * 
 * @param arg 用户参数（一般不用）
 */
void example_increase_lvgl_tick(void *arg);

// ===================== LVGL 初始化函数声明 =====================

/**
 * @brief LVGL 初始化函数
 * 
 * 初始化LVGL图形库、显示驱动、缓冲区等。主程序需首先调用本函数。
 * 必须在主函数中调用一次，才能正常使用LVGL相关功能。
 */
void LVGL_Init(void); // 屏幕初始化函数，主函数必须调用！！！！

#ifdef __cplusplus
}
#endif