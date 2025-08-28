/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-28 15:16:27
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-28 15:32:51
 * @FilePath: \esp-chunfeng\main\LCD_Driver\Display_SPD2010.c
 * @Description: SPD2010 电容触摸屏驱动头文件，包含数据结构定义、宏定义及相关函数声明
 */

#pragma once

#include "Display_SPD2010.h"   // 显示驱动头文件
#include "I2C_Driver.h"        // I2C 通信驱动头文件
#include "TCA9554PWR.h"        // IO 扩展芯片驱动头文件

// SPD2010 触摸芯片 I2C 地址
#define SPD2010_ADDR                    (0x53)

// 触摸中断引脚编号（GPIO4）
#define EXAMPLE_PIN_NUM_TOUCH_INT       (4)

// 触摸复位引脚编号（未使用，设为-1）
#define EXAMPLE_PIN_NUM_TOUCH_RST       (-1)

// 支持的最大触摸点数
#define CONFIG_ESP_LCD_TOUCH_MAX_POINTS     (5)

/****************HYN_REG_MUT_DEBUG_INFO_MODE address start***********/
// SPD2010 状态寄存器地址（未定义，设为NULL）
#define SPD2010_REG_Status         (NULL)

// 单个触摸点数据结构
typedef struct {
    uint8_t id;        // 触摸点ID
    uint16_t x;        // X坐标
    uint16_t y;        // Y坐标
    uint8_t weight;    // 压力/权重
} tp_report_t;

// SPD2010 触摸数据结构体，包含多点信息及手势等
typedef struct {
    tp_report_t rpt[10];   // 最多支持10个触摸点的数据
    uint8_t touch_num;     // 当前触摸点数量
    uint8_t pack_code;     // 数据包编号
    uint8_t down;          // 按下标志
    uint8_t up;            // 抬起标志
    uint8_t gesture;       // 手势类型
    uint16_t down_x;       // 按下时X坐标
    uint16_t down_y;       // 按下时Y坐标
    uint16_t up_x;         // 抬起时X坐标
    uint16_t up_y;         // 抬起时Y坐标
} SPD2010_Touch;

// 触摸状态高字节结构体
typedef struct {
    uint8_t none0;         // 保留字段
    uint8_t none1;         // 保留字段
    uint8_t none2;         // 保留字段
    uint8_t cpu_run;       // CPU运行状态
    uint8_t tint_low;      // 低电平中断标志
    uint8_t tic_in_cpu;    // CPU内部计数
    uint8_t tic_in_bios;   // BIOS内部计数
    uint8_t tic_busy;      // 忙状态标志
} tp_status_high_t;

// 触摸状态低字节结构体
typedef struct {
    uint8_t pt_exist;      // 是否有触摸点存在
    uint8_t gesture;       // 手势信息
    uint8_t key;           // 按键信息
    uint8_t aux;           // 辅助信息
    uint8_t keep;          // 保持标志
    uint8_t raw_or_pt;     // 原始数据或点数据标志
    uint8_t none6;         // 保留字段
    uint8_t none7;         // 保留字段
} tp_status_low_t;

// 触摸状态整体结构体
typedef struct {
    tp_status_low_t status_low;    // 低字节状态
    tp_status_high_t status_high;  // 高字节状态
    uint16_t read_len;             // 读取数据长度
} tp_status_t;

// HDP状态结构体
typedef struct {
    uint8_t status;                // HDP状态
    uint16_t next_packet_len;      // 下一个数据包长度
} tp_hdp_status_t;

// 全局触摸数据变量声明
extern SPD2010_Touch touch_data;

// I2C 读取触摸寄存器函数
// Driver_addr: 设备地址，Reg_addr: 寄存器地址，Reg_data: 数据缓冲区，Length: 读取长度
esp_err_t I2C_Read_Touch(uint8_t Driver_addr, uint16_t Reg_addr, uint8_t *Reg_data, uint32_t Length);

// I2C 写入触摸寄存器函数
// Driver_addr: 设备地址，Reg_addr: 寄存器地址，Reg_data: 数据缓冲区，Length: 写入长度
esp_err_t I2C_Write_Touch(uint8_t Driver_addr, uint16_t Reg_addr, uint8_t *Reg_data, uint32_t Length);

// 触摸初始化函数，返回0成功
uint8_t Touch_Init();

// 触摸循环处理函数，需周期性调用
void Touch_Loop(void);

// 触摸复位函数，返回0成功
uint8_t SPD2010_Touch_Reset(void);

// 读取SPD2010配置函数，返回配置值
uint16_t SPD2010_Read_cfg(void);

// 读取触摸数据函数
void Touch_Read_Data(void);

// 获取触摸点坐标、压力等信息
// x: X坐标数组，y: Y坐标数组，strength: 压力数组，point_num: 实际点数，max_point_num: 最大点数
bool Touch_Get_xy(uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SPD2010 专用函数声明
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// 设置触摸点模式命令
esp_err_t write_tp_point_mode_cmd();

// 启动触摸命令
esp_err_t write_tp_start_cmd();

// 启动CPU命令
esp_err_t write_tp_cpu_start_cmd();

// 清除中断命令
esp_err_t write_tp_clear_int_cmd();

// 读取触摸状态及数据长度
esp_err_t read_tp_status_length(tp_status_t *tp_status);

// 读取HDP数据包
esp_err_t read_tp_hdp(tp_status_t *tp_status, SPD2010_Touch *touch);

// 读取HDP状态
esp_err_t read_tp_hdp_status(tp_hdp_status_t *tp_hdp_status);

// 读取剩余HDP数据
esp_err_t Read_HDP_REMAIN_DATA(tp_hdp_status_t *tp_hdp_status);

// 读取固件版本
esp_err_t read_fw_version();

// 读取触摸数据到结构体
esp_err_t tp_read_data(SPD2010_Touch *touch);
