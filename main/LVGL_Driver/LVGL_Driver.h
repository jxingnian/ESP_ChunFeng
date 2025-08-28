/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-28 15:16:27
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-28 15:32:51
 * @FilePath: \esp-chunfeng\main\LCD_Driver\Display_SPD2010.c
 * @Description: LVGL驱动头文件，包含LVGL初始化及相关回调函数声明
 */

#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"      // FreeRTOS基础头文件
#include "freertos/task.h"          // FreeRTOS任务管理
#include "esp_timer.h"              // ESP定时器相关
#include "esp_err.h"                // ESP错误码定义
#include "esp_log.h"                // ESP日志输出
#include "lvgl.h"                   // LVGL主头文件
#include "demos/lv_demos.h"         // LVGL官方演示Demo

#include "Display_SPD2010.h"        // 屏幕驱动相关头文件

// LVGL缓冲区长度，单位为像素点数。缓冲区大小=屏幕宽*高/20，具体分配可根据实际需求调整
#define LVGL_BUF_LEN  (EXAMPLE_LCD_WIDTH * EXAMPLE_LCD_HEIGHT / 20)

// LVGL定时器周期，单位为毫秒。LVGL内部tick增加的时间间隔
#define EXAMPLE_LVGL_TICK_PERIOD_MS    2

// LVGL显示缓冲区对象，内部包含图形缓冲区（draw buffer）
extern lv_disp_draw_buf_t disp_buf;

// LVGL显示驱动对象，包含回调函数等驱动参数
extern lv_disp_drv_t disp_drv;

// LVGL显示设备指针，指向注册的显示设备
extern lv_disp_t *disp;

/**
 * @brief LVGL刷屏回调函数
 *
 * @param drv       LVGL显示驱动指针
 * @param area      刷新区域
 * @param color_map 区域对应的颜色数据
 *
 * 该函数由LVGL在需要刷新屏幕时自动调用，需将color_map中的像素数据输出到屏幕指定区域。
 */
void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);

/**
 * @brief LVGL显示驱动参数更新回调
 *
 * @param drv LVGL显示驱动指针
 *
 * 当LVGL旋转屏幕或驱动参数发生变化时调用，用于同步屏幕和触摸的旋转等设置。
 */
void example_lvgl_port_update_callback(lv_disp_drv_t *drv);

/**
 * @brief LVGL定时器回调函数
 *
 * @param arg 用户参数（未使用）
 *
 * 定时调用，用于增加LVGL内部tick计数，驱动动画、定时器等功能。
 */
void example_increase_lvgl_tick(void *arg);

/**
 * @brief LVGL初始化函数
 *
 * 必须在主函数中调用，用于初始化LVGL库、显示驱动、缓冲区等。
 */
void LVGL_Init(void);