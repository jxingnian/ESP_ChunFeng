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
    int cmd;                /*!< LCD具体命令字节（如寄存器地址或命令码） */
    const void *data;       /*!< 指向命令参数数据的缓冲区指针，数据内容依赖于cmd */
    size_t data_bytes;      /*!< data缓冲区的字节数（即参数长度） */
    unsigned int delay_ms;  /*!< 发送本命令后需要延时的毫秒数，0表示无需延时 */
} spd2010_lcd_init_cmd_t;

/**
 * @brief LCD面板厂商配置结构体
 *
 * 用于指定初始化命令数组、命令数量及接口类型（SPI/QSPI）。
 * 该结构体需传递给esp_lcd_panel_dev_config_t的vendor_config字段。
 */
typedef struct {
    const spd2010_lcd_init_cmd_t *init_cmds;    /*!< 指向初始化命令数组的指针。
                                                 *   该数组建议声明为static const，并放在函数外部。
                                                 *   参考源文件中的vendor_specific_init_default。
                                                 */
    uint16_t init_cmds_size;    /*!< 上述命令数组的元素数量（即命令条数） */
    struct {
        unsigned int use_qspi_interface: 1;     /*!< 是否使用QSPI接口：1=QSPI，0=SPI（默认SPI） */
    } flags;
} spd2010_vendor_config_t;

/**
 * @brief 创建SPD2010型号LCD面板对象
 *
 * @param[in]  io LCD面板IO句柄
 * @param[in]  panel_dev_config 面板设备通用配置（通过vendor_config选择QSPI或自定义初始化命令）
 * @param[out] ret_panel 返回的LCD面板句柄
 * @return
 *      - ESP_OK: 创建成功
 *      - 其它:   创建失败
 */
esp_err_t esp_lcd_new_panel_spd2010(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel);

/**
 * @brief LCD面板总线（SPI）配置结构体宏
 *
 * 用于快速初始化SPI总线相关参数。
 * @param sclk         SPI时钟引脚编号
 * @param mosi         SPI MOSI引脚编号
 * @param max_trans_sz 最大传输字节数
 */
#define SPD2010_PANEL_BUS_SPI_CONFIG(sclk, mosi, max_trans_sz)  \
    {                                                           \
        .sclk_io_num = sclk,            /* SPI时钟引脚 */       \
        .mosi_io_num = mosi,            /* SPI MOSI引脚 */      \
        .miso_io_num = -1,              /* 未使用MISO */        \
        .quadhd_io_num = -1,            /* 未使用QSPI HD */     \
        .quadwp_io_num = -1,            /* 未使用QSPI WP */     \
        .max_transfer_sz = max_trans_sz,/* 最大传输长度 */       \
    }

/**
 * @brief LCD面板总线（QSPI）配置结构体宏
 *
 * 用于快速初始化QSPI总线相关参数。
 * @param sclk         QSPI时钟引脚编号
 * @param d0~d3        QSPI数据线引脚编号
 * @param max_trans_sz 最大传输字节数
 */
#define SPD2010_PANEL_BUS_QSPI_CONFIG(sclk, d0, d1, d2, d3, max_trans_sz) \
    {                                                           \
        .sclk_io_num = sclk,            /* QSPI时钟引脚 */      \
        .data0_io_num = d0,             /* QSPI D0引脚 */       \
        .data1_io_num = d1,             /* QSPI D1引脚 */       \
        .data2_io_num = d2,             /* QSPI D2引脚 */       \
        .data3_io_num = d3,             /* QSPI D3引脚 */       \
        .max_transfer_sz = max_trans_sz,/* 最大传输长度 */       \
    }

/**
 * @brief LCD面板IO（SPI）配置结构体宏
 *
 * 用于快速初始化SPI接口的面板IO参数。
 * @param cs      片选引脚编号
 * @param dc      数据/命令选择引脚编号
 * @param cb      颜色传输完成回调函数
 * @param cb_ctx  回调函数上下文指针
 */
#define SPD2010_PANEL_IO_SPI_CONFIG(cs, dc, cb, cb_ctx)         \
    {                                                           \
        .cs_gpio_num = cs,              /* 片选引脚 */          \
        .dc_gpio_num = dc,              /* 数据/命令引脚 */     \
        .spi_mode = 3,                  /* SPI模式3 */          \
        .pclk_hz = 80 * 1000 * 1000,    /* SPI时钟80MHz */      \
        .trans_queue_depth = 10,        /* 传输队列深度 */       \
        .on_color_trans_done = cb,      /* 颜色传输完成回调 */   \
        .user_ctx = cb_ctx,             /* 回调上下文 */         \
        .lcd_cmd_bits = 8,              /* 命令位宽8位 */        \
        .lcd_param_bits = 8,            /* 参数位宽8位 */        \
    }

/**
 * @brief LCD面板IO（QSPI）配置结构体宏
 *
 * 用于快速初始化QSPI接口的面板IO参数。
 * @param cs      片选引脚编号
 * @param cb      颜色传输完成回调函数
 * @param cb_ctx  回调函数上下文指针
 */
#define SPD2010_PANEL_IO_QSPI_CONFIG(cs, cb, cb_ctx)            \
    {                                                           \
        .cs_gpio_num = cs,              /* 片选引脚 */          \
        .dc_gpio_num = -1,              /* QSPI无DC引脚 */      \
        .spi_mode = 3,                  /* SPI模式3 */          \
        .pclk_hz = 20 * 1000 * 1000,    /* QSPI时钟20MHz */     \
        .trans_queue_depth = 10,        /* 传输队列深度 */       \
        .on_color_trans_done = cb,      /* 颜色传输完成回调 */   \
        .user_ctx = cb_ctx,             /* 回调上下文 */         \
        .lcd_cmd_bits = 32,             /* 命令位宽32位 */       \
        .lcd_param_bits = 8,            /* 参数位宽8位 */        \
        .flags = {                                              \
            .quad_mode = true,          /* 启用四线模式 */       \
        },                                                      \
    }

#ifdef __cplusplus
}
#endif
