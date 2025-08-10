/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-10 15:52:44
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-10 16:15:33
 * @FilePath: \esp-brookesia-chunfeng\main\EXIO\TCA9554PWR.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

#include <stdio.h>
#include "I2C_Driver.h"

// TCA9554PWR 扩展IO引脚宏定义，分别对应8个IO口（注意：实际TCA9554只有8个IO，编号1~8）
#define TCA9554_EXIO1 0x01    // EXIO1 引脚编号
#define TCA9554_EXIO2 0x02    // EXIO2 引脚编号
#define TCA9554_EXIO3 0x03    // EXIO3 引脚编号
#define TCA9554_EXIO4 0x04    // EXIO4 引脚编号
#define TCA9554_EXIO5 0x05    // EXIO5 引脚编号
#define TCA9554_EXIO6 0x06    // EXIO6 引脚编号
#define TCA9554_EXIO7 0x07    // EXIO7 引脚编号
#define TCA9554_EXIO8 0x08    // EXIO8 引脚编号

/******************************************************
 * TCA9554PWR 相关宏定义
 ******************************************************/

#define TCA9554_ADDRESS      0x20    // TCA9554PWR I2C 设备地址（7位地址，默认A0~A2接地）

// TCA9554PWR 寄存器地址定义
#define TCA9554_INPUT_REG    0x00    // 输入寄存器，读取各IO当前输入电平
#define TCA9554_OUTPUT_REG   0x01    // 输出寄存器，设置各IO输出高低电平
#define TCA9554_Polarity_REG 0x02    // 极性反转寄存器，仅对输入模式有效，反转输入极性
#define TCA9554_CONFIG_REG   0x03    // 配置寄存器，设置IO方向（0=输出，1=输入）

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 * TCA9554PWR 操作函数声明
 ******************************************************/

// 读取TCA9554PWR指定寄存器的值
// @param REG: 寄存器地址（如TCA9554_INPUT_REG等）
// @return 读取到的8位寄存器值
uint8_t Read_REG(uint8_t REG);

// 向TCA9554PWR指定寄存器写入数据
// @param REG: 寄存器地址
// @param Data: 要写入的数据
void Write_REG(uint8_t REG, uint8_t Data);

/******************************************************
 * EXIO模式设置相关函数
 ******************************************************/

// 设置单个EXIO引脚的工作模式（输入/输出）
// @param Pin: 引脚编号（1~8）
// @param State: 0=输出模式，1=输入模式
void Mode_EXIO(uint8_t Pin, uint8_t State);

// 批量设置7个EXIO引脚的工作模式
// @param PinState: 每一位代表一个引脚的模式，0=输出，1=输入（bit0~bit6对应EXIO1~EXIO7，bit7未用）
void Mode_EXIOS(uint8_t PinState);

/******************************************************
 * EXIO状态读取相关函数
 ******************************************************/

// 读取指定EXIO引脚的电平状态
// @param Pin: 引脚编号（1~8）
// @return 该引脚当前电平（0/1）
uint8_t Read_EXIO(uint8_t Pin);

// 读取所有EXIO引脚的电平状态
// 默认读取输入寄存器（即当前IO输入电平），如需读取输出寄存器状态可传TCA9554_OUTPUT_REG
// @return 8位数据，每一位代表一个引脚的电平状态
uint8_t Read_EXIOS(void);

/******************************************************
 * EXIO输出状态设置相关函数
 ******************************************************/

// 设置指定EXIO引脚的输出电平（不影响其它引脚）
// @param Pin: 引脚编号（1~8）
// @param State: 输出电平（0=低，1=高）
void Set_EXIO(uint8_t Pin, bool State);

// 批量设置7个EXIO引脚的输出电平
// @param PinState: 7位数据（bit0~bit6），每一位代表一个引脚的输出状态（bit7未用）
void Set_EXIOS(uint8_t PinState);

/******************************************************
 * EXIO电平翻转相关函数
 ******************************************************/

// 翻转指定EXIO引脚的输出电平
// @param Pin: 引脚编号（1~8）
void Set_Toggle(uint8_t Pin);

/******************************************************
 * TCA9554PWR 设备初始化
 ******************************************************/

// 初始化TCA9554PWR，设置7个引脚的工作模式
// @param PinState: 7位数据（bit0~bit6），每一位代表一个引脚的模式（0=输出，1=输入），bit7未用
// 默认全部为输出模式
void TCA9554PWR_Init(uint8_t PinState);

// EXIO初始化（如I2C初始化等，返回esp_err_t错误码）
esp_err_t EXIO_Init(void);

#ifdef __cplusplus
}
#endif
