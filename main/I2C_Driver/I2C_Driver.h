#pragma once


#include <stdint.h>
#include <string.h>  // 用于 memcpy 内存拷贝
#include "esp_log.h" // ESP-IDF 日志库，便于调试输出
#include "driver/gpio.h" // ESP-IDF GPIO 驱动头文件
#include "driver/i2c.h"  // ESP-IDF I2C 驱动头文件

#ifdef __cplusplus
extern "C" {
#endif

/********************* I2C 配置相关宏定义 *********************/
// I2C SCL（时钟线）所用的 GPIO 引脚编号
#define I2C_SCL_IO                  10         /*!< 用于 I2C 主机时钟的 GPIO 编号 */
// I2C SDA（数据线）所用的 GPIO 引脚编号
#define I2C_SDA_IO                  11         /*!< 用于 I2C 主机数据的 GPIO 编号 */
// I2C 主机端口号（ESP32-S3 有多个 I2C 外设，此处选用 0 号）
#define I2C_MASTER_NUM              0          /*!< I2C 主机端口号，具体可用数量取决于芯片型号 */
// I2C 主机时钟频率（单位：Hz），此处设置为 400kHz（高速模式）
#define I2C_MASTER_FREQ_HZ          400000     /*!< I2C 主机时钟频率 */
// I2C 主机发送缓冲区大小（0 表示不使用缓冲区）
#define I2C_MASTER_TX_BUF_DISABLE   0          /*!< I2C 主机不需要发送缓冲区 */
// I2C 主机接收缓冲区大小（0 表示不使用缓冲区）
#define I2C_MASTER_RX_BUF_DISABLE   0          /*!< I2C 主机不需要接收缓冲区 */
// I2C 操作超时时间（单位：毫秒）
#define I2C_MASTER_TIMEOUT_MS       1000       /*!< I2C 操作超时时间，单位为毫秒 */

/********************* I2C 驱动函数声明 *********************/

/**
 * @brief I2C 总线初始化函数
 * 
 * 初始化 I2C 外设，配置 SCL/SDA 引脚、时钟频率等参数。
 * 需在主程序启动时调用一次。
 */
void I2C_Init(void);

/**
 * @brief I2C 写寄存器函数
 * 
 * 向指定 I2C 设备的某个寄存器写入数据（支持多字节写入）。
 * 
 * @param Driver_addr  I2C 设备地址（7 位地址，左对齐）
 * @param Reg_addr     寄存器地址（8 位）
 * @param Reg_data     指向待写入数据的指针
 * @param Length       写入数据的字节数
 * @return esp_err_t   ESP-IDF 错误码（ESP_OK 表示成功）
 */
esp_err_t I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length);

/**
 * @brief I2C 读寄存器函数
 * 
 * 从指定 I2C 设备的某个寄存器读取数据（支持多字节读取）。
 * 
 * @param Driver_addr  I2C 设备地址（7 位地址，左对齐）
 * @param Reg_addr     寄存器地址（8 位）
 * @param Reg_data     指向用于存放读取数据的缓冲区指针
 * @param Length       读取数据的字节数
 * @return esp_err_t   ESP-IDF 错误码（ESP_OK 表示成功）
 */
esp_err_t I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length);

#ifdef __cplusplus
}
#endif