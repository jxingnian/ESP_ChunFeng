#pragma once  // 防止头文件被多次包含

#include "Display_SPD2010.h"   // 引入显示屏驱动头文件
#include "I2C_Driver.h"        // 引入I2C驱动头文件
#include "TCA9554PWR.h"        // 引入TCA9554电源控制相关头文件

#ifdef __cplusplus
extern "C" {
#endif

// ===================== SPD2010 触摸芯片相关宏定义 =====================

// SPD2010 I2C地址（7位地址，实际使用时左移1位作为8位地址）
#define SPD2010_ADDR                    (0x53)

// 触摸中断引脚编号（GPIO编号，需根据实际硬件连接修改）
#define EXAMPLE_PIN_NUM_TOUCH_INT       (4)

// 触摸复位引脚编号（如未使用则设为-1）
#define EXAMPLE_PIN_NUM_TOUCH_RST       (-1)

// 最大支持的触摸点数（根据芯片规格调整）
#define CONFIG_ESP_LCD_TOUCH_MAX_POINTS     (5)     

// SPD2010状态寄存器地址（如未定义可设为NULL）
#define SPD2010_REG_Status         (NULL)

// ===================== 触摸点报告结构体 =====================
/**
 * @brief 单个触摸点数据结构
 * 
 * id      : 触摸点ID（用于多点区分）
 * x, y    : 触摸点坐标
 * weight  : 触摸点压力/强度（如有支持）
 */
typedef struct {
  uint8_t id;
  uint16_t x;
  uint16_t y;
  uint8_t weight;
} tp_report_t;

// ===================== SPD2010 触摸数据结构体 =====================
/**
 * @brief SPD2010触摸数据结构体
 * 
 * rpt[]      : 最多支持10个触摸点的详细数据
 * touch_num  : 当前检测到的触摸点数量
 * pack_code  : 数据包标识码
 * down/up    : 按下/抬起事件标志
 * gesture    : 手势类型
 * down_x/y   : 按下时的坐标
 * up_x/y     : 抬起时的坐标
 */
typedef struct{
  tp_report_t rpt[10];
  uint8_t touch_num;     // 当前触摸点数量
  uint8_t pack_code;     // 数据包标识
  uint8_t down;          // 按下事件标志
  uint8_t up;            // 抬起事件标志
  uint8_t gesture;       // 手势类型
  uint16_t down_x;       // 按下时X坐标
  uint16_t down_y;       // 按下时Y坐标
  uint16_t up_x;         // 抬起时X坐标
  uint16_t up_y;         // 抬起时Y坐标
} SPD2010_Touch;

// ===================== 状态寄存器高字节结构体 =====================
/**
 * @brief SPD2010状态寄存器高字节结构体
 * 
 * 各字段含义需参考SPD2010芯片手册
 */
typedef struct {
  uint8_t none0;        // 保留/未用
  uint8_t none1;        // 保留/未用
  uint8_t none2;        // 保留/未用
  uint8_t cpu_run;      // CPU运行状态
  uint8_t tint_low;     // 低电平中断标志
  uint8_t tic_in_cpu;   // CPU中断标志
  uint8_t tic_in_bios;  // BIOS中断标志
  uint8_t tic_busy;     // 忙状态标志
} tp_status_high_t;

// ===================== 状态寄存器低字节结构体 =====================
/**
 * @brief SPD2010状态寄存器低字节结构体
 * 
 * 各字段含义需参考SPD2010芯片手册
 */
typedef struct {
  uint8_t pt_exist;     // 是否存在触摸点
  uint8_t gesture;      // 手势标志
  uint8_t key;          // 按键标志
  uint8_t aux;          // 辅助标志
  uint8_t keep;         // 保持标志
  uint8_t raw_or_pt;    // 原始数据/点数据标志
  uint8_t none6;        // 保留/未用
  uint8_t none7;        // 保留/未用
} tp_status_low_t;

// ===================== SPD2010状态整体结构体 =====================
/**
 * @brief SPD2010状态结构体
 * 
 * status_low   : 低字节状态
 * status_high  : 高字节状态
 * read_len     : 需读取的数据长度
 */
typedef struct {
  tp_status_low_t status_low;
  tp_status_high_t status_high;
  uint16_t read_len;
} tp_status_t;

// ===================== HDP状态结构体 =====================
/**
 * @brief SPD2010 HDP（高数据包）状态结构体
 * 
 * status           : 当前HDP状态
 * next_packet_len  : 下一个数据包长度
 */
typedef struct {
  uint8_t status;
  uint16_t next_packet_len;
} tp_hdp_status_t;

// ===================== 全局变量声明 =====================

// SPD2010触摸数据全局变量
extern SPD2010_Touch touch_data;

// ===================== I2C通信相关函数声明 =====================

/**
 * @brief 通过I2C读取触摸芯片寄存器
 * 
 * @param Driver_addr  设备I2C地址
 * @param Reg_addr     寄存器地址
 * @param Reg_data     读取数据缓冲区指针
 * @param Length       读取数据长度
 * @return esp_err_t   ESP-IDF错误码
 */
esp_err_t I2C_Read_Touch(uint8_t Driver_addr, uint16_t Reg_addr, uint8_t *Reg_data, uint32_t Length);

/**
 * @brief 通过I2C写入触摸芯片寄存器
 * 
 * @param Driver_addr  设备I2C地址
 * @param Reg_addr     寄存器地址
 * @param Reg_data     待写入数据缓冲区指针
 * @param Length       写入数据长度
 * @return esp_err_t   ESP-IDF错误码
 */
esp_err_t I2C_Write_Touch(uint8_t Driver_addr, uint16_t Reg_addr, uint8_t *Reg_data, uint32_t Length);

// ===================== 触摸驱动主要接口函数声明 =====================

/**
 * @brief 触摸芯片初始化
 * 
 * @return uint8_t 初始化结果（1-成功，0-失败）
 */
uint8_t Touch_Init();

/**
 * @brief 触摸主循环处理函数（需在主循环中周期性调用）
 */
void Touch_Loop(void);

/**
 * @brief 触摸芯片硬件复位
 * 
 * @return uint8_t 复位结果（1-成功，0-失败）
 */
uint8_t SPD2010_Touch_Reset(void);

/**
 * @brief 读取SPD2010配置（如固件版本等）
 * 
 * @return uint16_t 读取结果
 */
uint16_t SPD2010_Read_cfg(void); 

/**
 * @brief 读取触摸点数据并更新全局touch_data
 */
void Touch_Read_Data(void);

/**
 * @brief 获取触摸点坐标及强度
 * 
 * @param x            X坐标数组指针
 * @param y            Y坐标数组指针
 * @param strength     压力/强度数组指针
 * @param point_num    实际检测到的点数指针
 * @param max_point_num最大支持点数
 * @return bool        是否有触摸点
 */
bool Touch_Get_xy(uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num);

// ===================== SPD2010专用底层命令函数声明 =====================

/**
 * @brief 设置触摸点模式命令
 */
esp_err_t write_tp_point_mode_cmd();

/**
 * @brief 启动触摸检测命令
 */
esp_err_t write_tp_start_cmd();

/**
 * @brief 启动触摸CPU命令
 */
esp_err_t write_tp_cpu_start_cmd();

/**
 * @brief 清除中断命令
 */
esp_err_t write_tp_clear_int_cmd();

/**
 * @brief 读取状态及数据长度
 * 
 * @param tp_status 状态结构体指针
 */
esp_err_t read_tp_status_length(tp_status_t *tp_status);

/**
 * @brief 读取HDP数据包
 * 
 * @param tp_status 状态结构体指针
 * @param touch     触摸数据结构体指针
 */
esp_err_t read_tp_hdp(tp_status_t *tp_status, SPD2010_Touch *touch);

/**
 * @brief 读取HDP状态
 * 
 * @param tp_hdp_status HDP状态结构体指针
 */
esp_err_t read_tp_hdp_status(tp_hdp_status_t *tp_hdp_status);

/**
 * @brief 读取剩余HDP数据
 * 
 * @param tp_hdp_status HDP状态结构体指针
 */
esp_err_t Read_HDP_REMAIN_DATA(tp_hdp_status_t *tp_hdp_status);

/**
 * @brief 读取固件版本
 */
esp_err_t read_fw_version();

/**
 * @brief 读取触摸点详细数据
 * 
 * @param touch 触摸数据结构体指针
 */
esp_err_t tp_read_data(SPD2010_Touch *touch);

#ifdef __cplusplus
}
#endif
