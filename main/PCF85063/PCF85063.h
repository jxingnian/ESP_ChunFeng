#pragma once  // 防止头文件被多次包含

#include "I2C_Driver.h"  // 引入I2C驱动头文件，便于与PCF85063通信

#ifdef __cplusplus
extern "C" {
#endif

// ===================== PCF85063 I2C地址定义 =====================
#define PCF85063_ADDRESS   (0x51)    // PCF85063 RTC芯片的7位I2C地址

// ===================== 年份偏移量宏定义 =====================
#define YEAR_OFFSET        (1970)    // RTC寄存器年份基准偏移，实际年份 = YEAR_OFFSET + RTC寄存器值

// ===================== PCF85063寄存器地址定义 =====================
// ---- 控制与状态寄存器 ----
#define RTC_CTRL_1_ADDR    (0x00)    // 控制寄存器1
#define RTC_CTRL_2_ADDR    (0x01)    // 控制寄存器2
#define RTC_OFFSET_ADDR    (0x02)    // 偏移校准寄存器
#define RTC_RAM_by_ADDR    (0x03)    // 1字节RAM寄存器

// ---- 时间与日期寄存器 ----
#define RTC_SECOND_ADDR    (0x04)    // 秒寄存器
#define RTC_MINUTE_ADDR    (0x05)    // 分钟寄存器
#define RTC_HOUR_ADDR      (0x06)    // 小时寄存器
#define RTC_DAY_ADDR       (0x07)    // 日期寄存器（1-31）
#define RTC_WDAY_ADDR      (0x08)    // 星期寄存器（0-6，0为周日）
#define RTC_MONTH_ADDR     (0x09)    // 月份寄存器（1-12）
#define RTC_YEAR_ADDR      (0x0A)    // 年份寄存器（0-99，实际年份需加YEAR_OFFSET）

// ---- 闹钟相关寄存器 ----
#define RTC_SECOND_ALARM   (0x0B)    // 闹钟秒寄存器
#define RTC_MINUTE_ALARM   (0x0C)    // 闹钟分钟寄存器
#define RTC_HOUR_ALARM     (0x0D)    // 闹钟小时寄存器
#define RTC_DAY_ALARM      (0x0E)    // 闹钟日期寄存器
#define RTC_WDAY_ALARM     (0x0F)    // 闹钟星期寄存器

// ---- 定时器相关寄存器 ----
#define RTC_TIMER_VAL      (0x10)    // 定时器计数值寄存器
#define RTC_TIMER_MODE     (0x11)    // 定时器模式寄存器

// ===================== 控制寄存器1位定义 =====================
#define RTC_CTRL_1_EXT_TEST (0x80)   // 外部测试模式使能
#define RTC_CTRL_1_STOP     (0x20)   // RTC时钟停止位，1-停止，0-运行
#define RTC_CTRL_1_SR       (0x10)   // 软件复位，1-复位
#define RTC_CTRL_1_CIE      (0x04)   // 校准中断使能，1-使能
#define RTC_CTRL_1_12_24    (0x02)   // 12/24小时制选择，0-24H，1-12H
#define RTC_CTRL_1_CAP_SEL  (0x01)   // 晶振负载电容选择，0-7pF，1-12.5pF

// ===================== 控制寄存器2位定义 =====================
#define RTC_CTRL_2_AIE      (0x80)   // 闹钟中断使能，1-使能
#define RTC_CTRL_2_AF       (0x40)   // 闹钟标志位，1-闹钟事件发生
#define RTC_CTRL_2_MI       (0x20)   // 分钟中断使能，1-使能
#define RTC_CTRL_2_HMI      (0x10)   // 半分钟中断使能
#define RTC_CTRL_2_TF       (0x08)   // 定时器标志位

// ===================== 偏移校准相关宏定义 =====================
#define RTC_OFFSET_MODE     (0x80)   // 偏移模式选择位

// ===================== 定时器模式寄存器位定义 =====================
#define RTC_TIMER_MODE_TE   (0x04)   // 定时器使能，1-使能
#define RTC_TIMER_MODE_TIE  (0x02)   // 定时器中断使能，1-使能
#define RTC_TIMER_MODE_TI_TP (0x01)  // 定时器中断模式，1-脉冲，0-标志

// ===================== 其他常用宏定义 =====================
#define RTC_ALARM           (0x80)   // 闹钟使能位（AEN_x寄存器）
#define RTC_CTRL_1_DEFAULT  (0x00)   // 控制寄存器1默认值
#define RTC_CTRL_2_DEFAULT  (0x00)   // 控制寄存器2默认值
#define RTC_TIMER_FLAG      (0x08)   // 定时器标志位

// ===================== 时间日期结构体定义 =====================
typedef struct {
    uint16_t year;   // 年份（实际年，如2024）
    uint8_t  month;  // 月份（1-12）
    uint8_t  day;    // 日期（1-31）
    uint8_t  dotw;   // 星期（0-6，0为周日）
    uint8_t  hour;   // 小时（0-23）
    uint8_t  minute; // 分钟（0-59）
    uint8_t  second; // 秒（0-59）
} datetime_t;

// ===================== 全局变量声明 =====================
extern datetime_t datetime;  // 全局时间变量，存储当前RTC时间

// ===================== 函数声明 =====================

/**
 * @brief 初始化PCF85063 RTC芯片
 */
void PCF85063_Init(void);

/**
 * @brief RTC主循环处理函数，需周期性调用
 */
void PCF85063_Loop(void);

/**
 * @brief 软复位PCF85063芯片
 */
void PCF85063_Reset(void);

/**
 * @brief 设置RTC时间（时分秒）
 * @param time 目标时间结构体
 */
void PCF85063_Set_Time(datetime_t time);

/**
 * @brief 设置RTC日期（年月日、星期）
 * @param date 目标日期结构体
 */
void PCF85063_Set_Date(datetime_t date);

/**
 * @brief 同时设置RTC的全部时间和日期
 * @param time 目标时间日期结构体
 */
void PCF85063_Set_All(datetime_t time);

/**
 * @brief 读取RTC当前时间
 * @param time 指向存储读取结果的结构体指针
 */
void PCF85063_Read_Time(datetime_t *time);

/**
 * @brief 使能RTC闹钟功能
 */
void PCF85063_Enable_Alarm(void);

/**
 * @brief 获取闹钟标志位（判断闹钟是否触发）
 * @return 1-闹钟触发，0-未触发
 */
uint8_t PCF85063_Get_Alarm_Flag(void);

/**
 * @brief 设置闹钟时间
 * @param time 目标闹钟时间结构体
 */
void PCF85063_Set_Alarm(datetime_t time);

/**
 * @brief 读取闹钟设置
 * @param time 指向存储读取结果的结构体指针
 */
void PCF85063_Read_Alarm(datetime_t *time);

/**
 * @brief 将时间结构体转换为字符串（格式：YYYY-MM-DD HH:MM:SS）
 * @param datetime_str 输出字符串缓冲区
 * @param time 输入的时间结构体
 */
void datetime_to_str(char *datetime_str, datetime_t time);

// ===================== 星期格式说明 =====================
// dotw字段取值：
// 0 - 周日 (Sunday)
// 1 - 周一 (Monday)
// 2 - 周二 (Tuesday)
// 3 - 周三 (Wednesday)
// 4 - 周四 (Thursday)
// 5 - 周五 (Friday)
// 6 - 周六 (Saturday)

#ifdef __cplusplus
}
#endif