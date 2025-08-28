/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-28 15:16:27
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-28 15:32:51
 * @FilePath: \esp-chunfeng\main\LCD_Driver\Display_SPD2010.c
 * @Description: I2C驱动头文件，包含I2C初始化、读写函数声明及相关宏定义
 */

#pragma once

#include <stdint.h>
#include <string.h>  // 用于memcpy函数
#include "esp_log.h" // ESP日志库
#include "driver/gpio.h" // GPIO驱动
#include "driver/i2c.h"  // I2C驱动

/********************* I2C 配置宏定义 *********************/
/**
 * @brief I2C主机时钟线SCL所使用的GPIO编号
 */
#define I2C_SCL_IO                  10         /*!< 用于I2C主机时钟的GPIO编号 */

/**
 * @brief I2C主机数据线SDA所使用的GPIO编号
 */
#define I2C_SDA_IO                  11         /*!< 用于I2C主机数据的GPIO编号 */

/**
 * @brief I2C主机端口号
 * 具体可用端口数量取决于芯片型号
 */
#define I2C_MASTER_NUM              0          /*!< I2C主机端口号 */

/**
 * @brief I2C主机时钟频率，单位Hz
 */
#define I2C_MASTER_FREQ_HZ          400000     /*!< I2C主机时钟频率 */

/**
 * @brief I2C主机发送缓冲区禁用
 * 主机模式下不需要缓冲区
 */
#define I2C_MASTER_TX_BUF_DISABLE   0          /*!< I2C主机不需要发送缓冲区 */

/**
 * @brief I2C主机接收缓冲区禁用
 * 主机模式下不需要缓冲区
 */
#define I2C_MASTER_RX_BUF_DISABLE   0          /*!< I2C主机不需要接收缓冲区 */

/**
 * @brief I2C主机操作超时时间，单位ms
 */
#define I2C_MASTER_TIMEOUT_MS       1000       /*!< I2C主机超时时间 */

/********************* I2C 函数声明 *********************/

/**
 * @brief I2C初始化函数
 *        配置I2C主机相关参数并初始化I2C外设
 */
void I2C_Init(void);

/**
 * @brief I2C写操作函数
 * @param Driver_addr  目标I2C设备地址
 * @param Reg_addr     目标寄存器地址（8位）
 * @param Reg_data     待写入的数据指针
 * @param Length       写入数据长度
 * @return             操作结果，esp_err_t类型
 */
esp_err_t I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length);

/**
 * @brief I2C读操作函数
 * @param Driver_addr  目标I2C设备地址
 * @param Reg_addr     目标寄存器地址（8位）
 * @param Reg_data     读取数据存放指针
 * @param Length       读取数据长度
 * @return             操作结果，esp_err_t类型
 */
esp_err_t I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length);