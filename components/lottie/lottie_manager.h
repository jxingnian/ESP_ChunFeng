/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-31 23:19:25
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-09-01 01:10:00
 * @FilePath: \ESP_ChunFeng\components\lottie\lottie_manager.h
 * @Description: 简单的Lottie动画管理器
 */

#ifndef LOTTIE_MANAGER_H
#define LOTTIE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>

// 动画类型宏定义
#define LOTTIE_ANIM_WIFI_LOADING    0
#define LOTTIE_ANIM_MIC            1
#define LOTTIE_ANIM_SPEAK          2
#define LOTTIE_ANIM_RABBIT         3

// 可以继续添加更多动画类型...

/**
 * @brief 初始化Lottie管理器
 * @return true 成功，false 失败
 */
bool lottie_manager_init(void);

/**
 * @brief 播放指定路径的动画
 * @param file_path 动画文件路径
 * @param width 动画宽度
 * @param height 动画高度
 * @return true 成功，false 失败
 */
bool lottie_manager_play(const char *file_path, uint16_t width, uint16_t height);

/**
 * @brief 停止当前动画
 */
void lottie_manager_stop(void);

/**
 * @brief 隐藏当前动画
 */
void lottie_manager_hide(void);

/**
 * @brief 显示当前动画
 */
void lottie_manager_show(void);

/**
 * @brief 设置动画位置
 * @param x X坐标
 * @param y Y坐标
 */
void lottie_manager_set_pos(int16_t x, int16_t y);

/**
 * @brief 居中显示动画
 */
void lottie_manager_center(void);

/**
 * @brief 播放指定类型的动画（简单API）
 * @param anim_type 动画类型宏（如LOTTIE_ANIM_MIC）
 * @return true 成功，false 失败
 */
bool lottie_manager_play_anim(int anim_type);

/**
 * @brief 停止指定类型的动画（简单API）
 * @param anim_type 动画类型宏，-1表示停止当前所有动画
 */
void lottie_manager_stop_anim(int anim_type);

#ifdef __cplusplus
}
#endif

#endif // LOTTIE_MANAGER_H