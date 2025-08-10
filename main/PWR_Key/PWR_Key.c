#include "PWR_Key.h"

// 电池状态标志，0：未初始化，1：按键按下准备，2：按键已松开可检测长按
static uint8_t BAT_State = 0; 
// 设备状态标志，1：休眠，2：重启
static uint8_t Device_State = 0; 
// 长按计数器，用于判断长按时长
static uint16_t Long_Press = 0;

/**
 * @brief 电源管理主循环处理函数
 * 
 * 该函数需在主循环中周期性调用，用于检测电源按键状态，
 * 并根据长按时长判断是否进入休眠、重启或关机。
 */
void PWR_Loop(void)
{
    // 只有BAT_State非0时才进行按键检测
    if (BAT_State) { 
        // 检测电源按键是否被按下（低电平有效）
        if (!gpio_get_level(PWR_KEY_Input_PIN)) {   
            // 只有在BAT_State为2（已松开过一次）时才计数长按
            if (BAT_State == 2) {         
                Long_Press++; // 长按计数递增

                // 判断长按时长，依次处理休眠、重启、关机
                if (Long_Press >= Device_Sleep_Time) {
                    // 长按时间达到休眠阈值但未到重启阈值
                    if (Long_Press >= Device_Sleep_Time && Long_Press < Device_Restart_Time)
                        Device_State = 1; // 标记为休眠
                    // 长按时间达到重启阈值但未到关机阈值
                    else if (Long_Press >= Device_Restart_Time && Long_Press < Device_Shutdown_Time)
                        Device_State = 2; // 标记为重启
                    // 长按时间达到关机阈值
                    else if (Long_Press >= Device_Shutdown_Time)
                        Shutdown(); // 执行关机操作
                }
            }
        } else {
            // 按键松开时
            if (BAT_State == 1)   
                BAT_State = 2; // 状态切换为可检测长按
            Long_Press = 0; // 长按计数清零
        }
    }
}

/**
 * @brief 设备休眠函数（待实现）
 * 
 * 该函数用于将设备切换到低功耗休眠状态。
 */
void Fall_Asleep(void)
{
    // TODO: 实现休眠逻辑
}

/**
 * @brief 设备重启函数（待实现）
 * 
 * 该函数用于软重启设备。
 */
void Restart(void)                              
{
    // TODO: 实现重启逻辑
}

/**
 * @brief 设备关机函数
 * 
 * 关闭电源控制引脚，并关闭LCD背光。
 */
void Shutdown(void)
{
    gpio_set_level(PWR_Control_PIN, false); // 关闭电源控制
    LCD_Backlight = 0;                      // 关闭LCD背光
}

/**
 * @brief GPIO引脚配置函数
 * 
 * @param pin  引脚编号
 * @param Mode 引脚模式（输入/输出）
 */
void configure_GPIO(int pin, gpio_mode_t Mode)
{
    gpio_reset_pin(pin);           // 复位引脚，恢复默认状态
    gpio_set_direction(pin, Mode); // 设置引脚方向（输入/输出）
}

/**
 * @brief 电源管理模块初始化
 * 
 * 配置电源按键输入引脚和电源控制输出引脚，并初始化电源状态。
 */
void PWR_Init(void) {
    // 配置电源按键为输入模式
    configure_GPIO(PWR_KEY_Input_PIN, GPIO_MODE_INPUT);    
    // 配置电源控制引脚为输出模式
    configure_GPIO(PWR_Control_PIN, GPIO_MODE_OUTPUT);
    // 默认关闭电源控制
    gpio_set_level(PWR_Control_PIN, false);
    // 延时100ms，等待电路稳定
    vTaskDelay(100);
    // 检查上电时按键是否被按下（低电平有效）
    if (!gpio_get_level(PWR_KEY_Input_PIN)) {   
        BAT_State = 1;               // 标记为按键按下准备
        gpio_set_level(PWR_Control_PIN, true); // 打开电源控制
    }
}
