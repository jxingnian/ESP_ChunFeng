/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-31 23:19:25
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-09-01 00:32:53
 * @FilePath: \ESP_ChunFeng\components\lottie\lottie_manager.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/*
 * LVGL Lottie动画管理器
 * 支持从Flash加载JSON到PSRAM，实现多动画切换
 */

#ifndef LOTTIE_MANAGER_H
#define LOTTIE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * 初始化Lottie管理器
 * @return true 成功，false 失败
 */
bool lottie_manager_init(void);

#ifdef __cplusplus
}
#endif

#endif // LOTTIE_MANAGER_H
