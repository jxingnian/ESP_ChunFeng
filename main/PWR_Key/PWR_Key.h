/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-10 16:13:47
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-10 20:23:34
 * @FilePath: \esp-brookesia-chunfeng\main\PWR_Key\PWR_Key.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once  // 防止头文件被多次包含

#include "Display_SPD2010.h"  // 引入LCD显示屏驱动头文件

#ifdef __cplusplus
extern "C" {
#endif

// ===================== 电源按键与控制引脚宏定义 =====================
// 电源按键输入引脚编号（GPIO编号，需根据实际硬件连接修改）
#define PWR_KEY_Input_PIN   6
// 电源控制输出引脚编号（用于控制电源芯片或MOS管等）
#define PWR_Control_PIN     7

// ===================== 设备电源管理时间参数（单位：秒） =====================
// 设备进入休眠的等待时间（秒），即无操作后多长时间自动休眠
#define Device_Sleep_Time    10
// 设备重启等待时间（秒），如长按按键触发重启的判定时间
#define Device_Restart_Time  3
// 设备关机等待时间（秒），如长按按键触发关机的判定时间
#define Device_Shutdown_Time 5

// ===================== 电源管理相关函数声明 =====================

/**
 * @brief 使设备进入休眠模式
 * 
 * 该函数用于将设备切换到低功耗休眠状态，通常在无操作或按键事件触发时调用。
 */
void Fall_Asleep(void);

/**
 * @brief 关闭设备电源（关机）
 * 
 * 该函数用于安全地关闭设备电源，执行关机流程。
 */
void Shutdown(void);

/**
 * @brief 重启设备
 * 
 * 该函数用于软重启设备，通常在长按电源键等场景下调用。
 */
void Restart(void);

/**
 * @brief 电源管理模块初始化
 * 
 * 初始化电源按键输入、控制引脚等相关硬件资源，需在主程序启动时调用。
 */
void PWR_Init(void);

/**
 * @brief 电源管理主循环处理函数
 * 
 * 需在主循环中周期性调用，用于检测按键状态、处理休眠/重启/关机等事件。
 */
void PWR_Loop(void);

#ifdef __cplusplus
}
#endif
