/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-28 15:16:27
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-30 09:55:38
 * @FilePath: \esp-chunfeng\main\LVGL_Driver\LVGL_Driver copy.h
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
#include "Touch_SPD2010.h"         // 触摸驱动相关头文件

// LVGL缓冲区长度，单位为像素点数。缓冲区大小=屏幕宽*高/20，具体分配可根据实际需求调整
#define LVGL_BUF_LEN  (EXAMPLE_LCD_WIDTH * EXAMPLE_LCD_HEIGHT / 20)

// LVGL定时器周期，单位为毫秒。LVGL内部tick增加的时间间隔
#define EXAMPLE_LVGL_TICK_PERIOD_MS    2

// LVGL 9.x 不再需要这些全局结构体变量，显示对象直接创建
// LVGL显示设备指针，指向创建的显示设备
extern lv_disp_t *disp;

/**
 * @brief LVGL刷屏回调函数
 *
 * @param disp      LVGL显示对象指针
 * @param area      刷新区域
 * @param px_map    区域对应的像素数据（字节数组）
 *
 * 该函数由LVGL在需要刷新屏幕时自动调用，需将px_map中的像素数据输出到屏幕指定区域。
 */
void example_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

/**
 * @brief LVGL显示驱动参数更新回调
 *
 * @param disp LVGL显示对象指针
 *
 * 当LVGL旋转屏幕或驱动参数发生变化时调用，用于同步屏幕和触摸的旋转等设置。
 * 注意：LVGL 9.x中此回调可能不再自动调用，需要手动处理屏幕旋转。
 */
void example_lvgl_port_update_callback(lv_display_t *disp);

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
 * 更新为LVGL 9.2.2版本的API。
 */
void LVGL_Init(void);

/**
 * @brief LVGL触摸读取回调函数
 *
 * @param indev 输入设备指针
 * @param data  触摸数据结构体指针
 *
 * 该函数由LVGL在需要读取触摸状态时自动调用。
 */
void example_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data);

/**
 * @brief LVGL区域对齐回调函数
 *
 * @param disp LVGL显示对象指针
 * @param area 刷新区域指针
 *
 * 用于将刷新区域对齐到4字节边界。
 * 注意：LVGL 9.x中rounder回调已被移除，此函数保留用于兼容性。
 */
void Lvgl_port_rounder_callback(lv_display_t *disp, lv_area_t *area);