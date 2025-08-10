/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2024-12-03 11:05:10
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-10 13:44:57
 * @FilePath: \ESP32-S3-Touch-LCD-1.46-Test\main\QMI8658\QMI8658.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

#include "I2C_Driver.h"

#ifdef __cplusplus
extern "C" {
#endif

// 设备I2C地址（低电平/高电平）
#define QMI8658_L_SLAVE_ADDRESS                 (0x6B)   // 低电平I2C地址
#define QMI8658_H_SLAVE_ADDRESS                 (0x6A)   // 高电平I2C地址

// QMI8658寄存器地址定义
#define QMI8658_WHO_AM_I      0x00    // 设备ID寄存器
#define QMI8658_REVISION_ID   0x01    // 版本号寄存器
#define QMI8658_CTRL1         0x02    // SPI接口及传感器使能控制
#define QMI8658_CTRL2         0x03    // 加速度计设置
#define QMI8658_CTRL3         0x04    // 陀螺仪设置
#define QMI8658_CTRL4         0x05    // 保留（未使用）
#define QMI8658_CTRL5         0x06    // 低通滤波器设置
#define QMI8658_CTRL6         0x07    // 姿态引擎设置（未使用）
#define QMI8658_CTRL7         0x08    // 传感器使能
#define QMI8658_CTRL8         0x09    // 运动检测控制（当前库未用）
#define QMI8658_CTRL9         0x0A    // 主机命令（当前库未用）

// 校准寄存器
#define QMI8658_CAL1_L        0x0B    // 校准1寄存器低字节
#define QMI8658_CAL1_H        0x0C    // 校准1寄存器高字节
#define QMI8658_CAL2_L        0x0D    // 校准2寄存器低字节
#define QMI8658_CAL2_H        0x0E    // 校准2寄存器高字节
#define QMI8658_CAL3_L        0x0F    // 校准3寄存器低字节
#define QMI8658_CAL3_H        0x10    // 校准3寄存器高字节
#define QMI8658_CAL4_L        0x11    // 校准4寄存器低字节
#define QMI8658_CAL4_H        0x12    // 校准4寄存器高字节

// 温度数据寄存器
#define QMI8658_TEMP_L        0x33    // 温度数据低字节
#define QMI8658_TEMP_H        0x34    // 温度数据高字节

#define QMI8658_STATUSINT     0x2D    // 状态及中断寄存器

// 加速度计原始数据寄存器
#define QMI8658_AX_L          0x35    // X轴加速度低字节
#define QMI8658_AX_H          0x36    // X轴加速度高字节
#define QMI8658_AY_L          0x37    // Y轴加速度低字节
#define QMI8658_AY_H          0x38    // Y轴加速度高字节
#define QMI8658_AZ_L          0x39    // Z轴加速度低字节
#define QMI8658_AZ_H          0x3A    // Z轴加速度高字节

// 陀螺仪原始数据寄存器
#define QMI8658_GX_L          0x3B    // X轴角速度低字节
#define QMI8658_GX_H          0x3C    // X轴角速度高字节
#define QMI8658_GY_L          0x3D    // Y轴角速度低字节
#define QMI8658_GY_H          0x3E    // Y轴角速度高字节
#define QMI8658_GZ_L          0x3F    // Z轴角速度低字节
#define QMI8658_GZ_H          0x40    // Z轴角速度高字节

// 配置寄存器掩码及偏移量
#define QMI8658_AODR_MASK         0x0F    // 加速度计输出数据速率掩码（CTRL2）
#define QMI8658_GODR_MASK         0x0F    // 陀螺仪输出数据速率掩码（CTRL3）
#define QMI8658_ASCALE_MASK       0x70    // 加速度计量程掩码
#define QMI8658_GSCALE_MASK       0x70    // 陀螺仪量程掩码
#define QMI8658_ALPF_MASK         0x06    // 加速度计低通滤波器设置掩码
#define QMI8658_GLPF_MASK         0x60    // 陀螺仪低通滤波器设置掩码
#define QMI8658_ASCALE_OFFSET     4       // 加速度计量程位偏移
#define QMI8658_GSCALE_OFFSET     4       // 陀螺仪量程位偏移
#define QMI8658_ALPF_OFFSET       1       // 加速度计低通滤波器位偏移
#define QMI8658_GLPF_OFFSET       5       // 陀螺仪低通滤波器位偏移

#define QMI8658_COMM_TIMEOUT      50      // 通信超时时间，单位ms

// 传感器数据刷新延迟（微秒）
// 仅在locking模式下单次读取时生效，running模式无效
#define QMI8658_REFRESH_DELAY     2000

// 控制时钟门控（数据锁定功能需要）
#define QMI8658_CTRL_CMD_AHB_CLOCK_GATING 0x12

// 加速度计输出数据速率枚举
typedef enum {
    acc_odr_norm_8000 = 0x0,   // 正常模式 8000Hz
    acc_odr_norm_4000,         // 正常模式 4000Hz
    acc_odr_norm_2000,         // 正常模式 2000Hz
    acc_odr_norm_1000,         // 正常模式 1000Hz
    acc_odr_norm_500,          // 正常模式 500Hz
    acc_odr_norm_250,          // 正常模式 250Hz
    acc_odr_norm_120,          // 正常模式 120Hz
    acc_odr_norm_60,           // 正常模式 60Hz
    acc_odr_norm_30,           // 正常模式 30Hz
    acc_odr_lp_128 = 0xC,      // 低功耗模式 128Hz
    acc_odr_lp_21,             // 低功耗模式 21Hz
    acc_odr_lp_11,             // 低功耗模式 11Hz
    acc_odr_lp_3,              // 低功耗模式 3Hz
} acc_odr_t;

// 陀螺仪输出数据速率枚举
typedef enum {
    gyro_odr_norm_8000 = 0x0,  // 正常模式 8000Hz
    gyro_odr_norm_4000,        // 正常模式 4000Hz
    gyro_odr_norm_2000,        // 正常模式 2000Hz
    gyro_odr_norm_1000,        // 正常模式 1000Hz
    gyro_odr_norm_500,         // 正常模式 500Hz
    gyro_odr_norm_250,         // 正常模式 250Hz
    gyro_odr_norm_120,         // 正常模式 120Hz
    gyro_odr_norm_60,          // 正常模式 60Hz
    gyro_odr_norm_30           // 正常模式 30Hz
} gyro_odr_t;

// 加速度计量程枚举
typedef enum {
    ACC_RANGE_2G = 0x0,        // ±2g
    ACC_RANGE_4G,              // ±4g
    ACC_RANGE_8G,              // ±8g
    ACC_RANGE_16G              // ±16g
} acc_scale_t;

// 陀螺仪量程枚举
typedef enum {
    GYR_RANGE_16DPS = 0x0,     // ±16dps
    GYR_RANGE_32DPS,           // ±32dps
    GYR_RANGE_64DPS,           // ±64dps
    GYR_RANGE_128DPS,          // ±128dps
    GYR_RANGE_256DPS,          // ±256dps
    GYR_RANGE_512DPS,          // ±512dps
    GYR_RANGE_1024DPS          // ±1024dps
} gyro_scale_t;

// 低通滤波器模式枚举
typedef enum {
    LPF_MODE_0 = 0x0,     // 截止频率为ODR的2.66%
    LPF_MODE_1 = 0x2,     // 截止频率为ODR的3.63%
    LPF_MODE_2 = 0x4,     // 截止频率为ODR的5.39%
    LPF_MODE_3 = 0x6      // 截止频率为ODR的13.37%
} lpf_t;

// 传感器状态枚举
typedef enum {
    sensor_default,        // 默认状态
    sensor_power_down,     // 省电/掉电状态
    sensor_running,        // 正常运行状态
    sensor_locking         // 锁定模式
} sensor_state_t;

// 三轴IMU数据结构体
typedef struct __IMUdata {
    float x;   // X轴数据
    float y;   // Y轴数据
    float z;   // Z轴数据
} IMUdata;

// 全局变量声明：加速度计与陀螺仪三轴数据
extern IMUdata Accel;   // 加速度计数据
extern IMUdata Gyro;    // 陀螺仪数据

// QMI8658初始化函数，完成芯片配置
void QMI8658_Init(void);

// QMI8658主循环处理函数
void QMI8658_Loop(void);

// 向QMI8658指定寄存器写入数据
void QMI8658_transmit(uint8_t addr, uint8_t data);

// 从QMI8658指定寄存器读取数据
uint8_t QMI8658_receive(uint8_t addr);

// 向CTRL9寄存器写入命令
void QMI8658_CTRL9_Write(uint8_t command);

// 更新传感器数据（读取加速度计和陀螺仪原始数据并转换为物理量）
void QMI8658_sensor_update();

// 根据需要自动更新传感器数据（如有新数据则更新）
void QMI8658_update_if_needed();

// 设置加速度计输出数据速率
void setAccODR(acc_odr_t odr);

// 设置陀螺仪输出数据速率
void setGyroODR(gyro_odr_t odr);

// 设置加速度计量程
void setAccScale(acc_scale_t scale);

// 设置陀螺仪量程
void setGyroScale(gyro_scale_t scale);

// 设置加速度计低通滤波器
void setAccLPF(lpf_t lpf);

// 设置陀螺仪低通滤波器
void setGyroLPF(lpf_t lpf);

// 设置传感器工作状态
void setState(sensor_state_t state);

// 获取原始加速度计和陀螺仪数据（6轴，顺序：AX, AY, AZ, GX, GY, GZ）
void getRawReadings(int16_t* buf);

// 获取加速度计X轴物理量（单位g）
float getAccX();
// 获取加速度计Y轴物理量（单位g）
float getAccY();
// 获取加速度计Z轴物理量（单位g）
float getAccZ();

// 获取陀螺仪X轴物理量（单位dps）
float getGyroX();
// 获取陀螺仪Y轴物理量（单位dps）
float getGyroY();
// 获取陀螺仪Z轴物理量（单位dps）
float getGyroZ();

// 读取并更新全局加速度计数据
void getAccelerometer(void);

// 读取并更新全局陀螺仪数据
void getGyroscope(void);

#ifdef __cplusplus
}
#endif
