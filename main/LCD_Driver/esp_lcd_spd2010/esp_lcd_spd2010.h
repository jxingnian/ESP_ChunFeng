/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-28 15:16:27
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-28 15:32:51
 * @FilePath: \esp-chunfeng\main\LCD_Driver\Display_SPD2010.c
 * @Description: SPD2010 LCD驱动头文件，定义了初始化命令结构体、厂商配置结构体、面板创建函数及相关宏
 */
/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>
#include "esp_lcd_panel_vendor.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LCD面板初始化命令结构体
 *
 * 用于描述LCD初始化时需要发送的命令及其参数、数据长度和延时。
 */
typedef struct {
    int cmd;                ///< LCD具体命令字
    const void *data;       ///< 指向命令参数数据的缓冲区指针
    size_t data_bytes;      ///< data缓冲区的字节数
    unsigned int delay_ms;  ///< 该命令发送后需要延时的毫秒数
} spd2010_lcd_init_cmd_t;

/**
 * @brief LCD面板厂商配置结构体
 *
 * 用于选择接口类型（SPI/QSPI）及自定义初始化命令数组。
 * 需传递给esp_lcd_panel_dev_config_t结构体的vendor_config字段。
 */
typedef struct {
    const spd2010_lcd_init_cmd_t *init_cmds;    /*!< 指向初始化命令数组的指针。
                                                 *   该数组应声明为static const并放在函数外部。
                                                 *   参考源文件中的vendor_specific_init_default。
                                                 */
    uint16_t init_cmds_size;    ///< 上述命令数组的命令数量
    struct {
        unsigned int use_qspi_interface: 1;     ///< 置1表示使用QSPI接口，默认SPI接口
    } flags;
} spd2010_vendor_config_t;

/**
 * @brief 创建SPD2010型号LCD面板
 *
 * @param[in]  io LCD面板IO句柄
 * @param[in]  panel_dev_config 面板设备通用配置（通过vendor_config选择QSPI接口或自定义初始化命令）
 * @param[out] ret_panel 返回的LCD面板句柄
 * @return
 *      - ESP_OK: 成功
 *      - 其他: 失败
 */
esp_err_t esp_lcd_new_panel_spd2010(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel);

/**
 * @brief LCD面板总线SPI配置宏
 *
 * 用于快速初始化SPI总线配置结构体，适用于SPD2010面板。
 * @param sclk SCLK引脚编号
 * @param mosi MOSI引脚编号
 * @param max_trans_sz 最大传输字节数
 */
#define SPD2010_PANEL_BUS_SPI_CONFIG(sclk, mosi, max_trans_sz)  \
    {                                                           \
        .sclk_io_num = sclk,                                    /* SPI时钟引脚编号 */ \
                       .mosi_io_num = mosi,                                    /* SPI主输出从输入引脚编号 */ \
                                      .miso_io_num = -1,                                      /* SPI主输入从输出引脚，未使用设为-1 */ \
                                                     .quadhd_io_num = -1,                                    /* QSPI HD引脚，SPI模式未用设为-1 */ \
                                                                      .quadwp_io_num = -1,                                    /* QSPI WP引脚，SPI模式未用设为-1 */ \
                                                                                       .max_transfer_sz = max_trans_sz,                        /* 最大传输字节数 */ \
    }

/**
 * @brief LCD面板总线QSPI配置宏
 *
 * 用于快速初始化QSPI总线配置结构体，适用于SPD2010面板。
 * @param sclk SCLK引脚编号
 * @param d0   QSPI数据线0引脚编号
 * @param d1   QSPI数据线1引脚编号
 * @param d2   QSPI数据线2引脚编号
 * @param d3   QSPI数据线3引脚编号
 * @param max_trans_sz 最大传输字节数
 */
#define SPD2010_PANEL_BUS_QSPI_CONFIG(sclk, d0, d1, d2, d3, max_trans_sz) \
    {                                                           \
        .sclk_io_num = sclk,                                    /* QSPI时钟引脚编号 */ \
                       .data0_io_num = d0,                                     /* QSPI数据线0引脚编号 */ \
                                       .data1_io_num = d1,                                     /* QSPI数据线1引脚编号 */ \
                                                       .data2_io_num = d2,                                     /* QSPI数据线2引脚编号 */ \
                                                                       .data3_io_num = d3,                                     /* QSPI数据线3引脚编号 */ \
                                                                                       .max_transfer_sz = max_trans_sz,                        /* 最大传输字节数 */ \
    }

/**
 * @brief LCD面板IO SPI配置宏
 *
 * 用于快速初始化SPI IO配置结构体，适用于SPD2010面板。
 * @param cs      片选引脚编号
 * @param dc      数据/命令选择引脚编号
 * @param cb      颜色传输完成回调函数
 * @param cb_ctx  回调函数上下文
 */
#define SPD2010_PANEL_IO_SPI_CONFIG(cs, dc, cb, cb_ctx)         \
    {                                                           \
        .cs_gpio_num = cs,                                      /* 片选引脚编号 */ \
                       .dc_gpio_num = dc,                                      /* 数据/命令选择引脚编号 */ \
                                      .spi_mode = 3,                                          /* SPI模式3 */ \
                                                  .pclk_hz = 80 * 1000 * 1000,                            /* SPI时钟频率80MHz */ \
                                                             .trans_queue_depth = 10,                                /* 传输队列深度 */ \
                                                                                  .on_color_trans_done = cb,                              /* 颜色传输完成回调 */ \
                                                                                                         .user_ctx = cb_ctx,                                     /* 回调上下文 */ \
                                                                                                                     .lcd_cmd_bits = 8,                                      /* 命令位宽8位 */ \
                                                                                                                             .lcd_param_bits = 8,                                    /* 参数位宽8位 */ \
    }

/**
 * @brief LCD面板IO QSPI配置宏
 *
 * 用于快速初始化QSPI IO配置结构体，适用于SPD2010面板。
 * @param cs      片选引脚编号
 * @param cb      颜色传输完成回调函数
 * @param cb_ctx  回调函数上下文
 */
#define SPD2010_PANEL_IO_QSPI_CONFIG(cs, cb, cb_ctx)            \
    {                                                           \
        .cs_gpio_num = cs,                                      /* 片选引脚编号 */ \
                       .dc_gpio_num = -1,                                      /* QSPI模式下未用，设为-1 */ \
                                      .spi_mode = 3,                                          /* SPI模式3 */ \
                                                  .pclk_hz = 20 * 1000 * 1000,                            /* QSPI时钟频率20MHz */ \
                                                             .trans_queue_depth = 10,                                /* 传输队列深度 */ \
                                                                                  .on_color_trans_done = cb,                              /* 颜色传输完成回调 */ \
                                                                                                         .user_ctx = cb_ctx,                                     /* 回调上下文 */ \
                                                                                                                     .lcd_cmd_bits = 32,                                     /* 命令位宽32位 */ \
                                                                                                                             .lcd_param_bits = 8,                                    /* 参数位宽8位 */ \
                                                                                                                                     .flags = {                                              \
                                                                                                                                                                                             .quad_mode = true,                                  /* 启用四线模式 */ \
                                                                                                                                              },                                                      \
    }

#ifdef __cplusplus
}
#endif
