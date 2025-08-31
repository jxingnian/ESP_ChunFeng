/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-30 16:30:00
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-30 16:30:00
 * @FilePath: \esp-chunfeng\main\Touch_Driver\Touch_SPD2010_Official.h
 * @Description: 使用官方esp_lcd_touch_spd2010组件的触摸驱动头文件
 */

#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c_types.h"

// 官方SPD2010触摸组件头文件
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_spd2010.h"

/*********************
 * 触摸屏参数配置
 *********************/

// 触摸屏分辨率（与显示屏分辨率一致）
#define TOUCH_MAX_X                     412
#define TOUCH_MAX_Y                     412

// I2C接口配置（使用外部已初始化的I2C驱动）
#define TOUCH_I2C_PORT                  0                   // I2C端口号（与I2C_Driver.h一致）

// 触摸控制引脚配置
#define TOUCH_RST_PIN                   (-1)                // 复位引脚（-1表示不使用）
#define TOUCH_INT_PIN                   (-1)                // 中断引脚（-1表示不使用，因为LVGL使用轮询方式）

// 触摸屏坐标变换配置
#define TOUCH_SWAP_XY                   0                   // 是否交换X和Y坐标
#define TOUCH_MIRROR_X                  0                   // 是否镜像X坐标
#define TOUCH_MIRROR_Y                  0                   // 是否镜像Y坐标

// 触摸相关常量
#define TOUCH_MAX_POINTS                5                   // 最大支持触摸点数
#define TOUCH_DEBOUNCE_TIME_MS          50                  // 触摸防抖时间

/*********************
 * 全局变量声明
 *********************/

// 触摸句柄
extern esp_lcd_touch_handle_t touch_handle;
extern esp_lcd_panel_io_handle_t touch_io_handle;

/*********************
 * 函数声明
 *********************/

/**
 * @brief 初始化SPD2010触摸控制器（官方组件版本）
 * @return esp_err_t 初始化结果
 *         - ESP_OK: 成功
 *         - ESP_FAIL: 失败
 * @note 使用官方esp_lcd_touch_spd2010组件实现
 */
esp_err_t Touch_Init_Official(void);

/**
 * @brief 反初始化触摸驱动
 * @note 清理所有分配的资源
 */
void Touch_Deinit_Official(void);

/**
 * @brief 读取触摸数据（官方组件版本）
 * @param touch_x X坐标数组，用于存储触摸点的X坐标
 * @param touch_y Y坐标数组，用于存储触摸点的Y坐标
 * @param strength 触摸强度数组，可为NULL（如果不需要强度信息）
 * @param touch_count 触摸点数量指针，返回实际检测到的触摸点数
 * @param max_points 最大触摸点数，数组的大小
 * @return bool 是否有触摸点
 *         - true: 检测到触摸点
 *         - false: 没有触摸点或读取失败
 * @note 这个函数与原来的Touch_Get_xy函数兼容
 */
bool Touch_Get_xy_Official(uint16_t *touch_x, uint16_t *touch_y, uint16_t *strength, 
                          uint8_t *touch_count, uint8_t max_points);

/**
 * @brief 获取触摸句柄
 * @return esp_lcd_touch_handle_t 触摸句柄
 * @note 用于LVGL驱动获取触摸句柄
 */
esp_lcd_touch_handle_t Touch_Get_Handle_Official(void);

/**
 * @brief 检查触摸控制器是否已初始化
 * @return bool 是否已初始化
 *         - true: 已初始化
 *         - false: 未初始化
 */
bool Touch_Is_Initialized_Official(void);

/*********************
 * 兼容性宏定义
 *********************/

// 为了与现有代码兼容，提供宏定义
#define Touch_Init()                    Touch_Init_Official()
#define Touch_Get_xy(x, y, s, c, m)     Touch_Get_xy_Official(x, y, s, c, m)
#define Touch_Deinit()                  Touch_Deinit_Official()
