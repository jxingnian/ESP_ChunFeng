/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-28 15:16:27
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-28 15:32:51
 * @FilePath: \esp-chunfeng\main\LCD_Driver\Display_SPD2010.c
 * @Description: SPD2010 LCD驱动实现文件，包含面板初始化、绘图、控制等功能
 */
/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <sys/cdefs.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "esp_log.h"

#include "esp_lcd_spd2010.h"

// QSPI接口命令定义
#define LCD_OPCODE_WRITE_CMD        (0x02ULL) // 写命令操作码
#define LCD_OPCODE_READ_CMD         (0x0BULL) // 读命令操作码
#define LCD_OPCODE_WRITE_COLOR      (0x32ULL) // 写颜色数据操作码

// SPD2010专用命令定义
#define SPD2010_CMD_SET             (0xFF)   // 命令集切换命令
#define SPD2010_CMD_SET_BYTE0       (0x20)   // 命令集切换参数0
#define SPD2010_CMD_SET_BYTE1       (0x10)   // 命令集切换参数1
#define SPD2010_CMD_SET_USER        (0x00)   // 用户命令集标志

static const char *TAG = "spd2010"; // 日志标签

// 面板操作函数声明
static esp_err_t panel_spd2010_del(esp_lcd_panel_t *panel);
static esp_err_t panel_spd2010_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_spd2010_init(esp_lcd_panel_t *panel);
static esp_err_t panel_spd2010_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data);
static esp_err_t panel_spd2010_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_spd2010_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_spd2010_swap_xy(esp_lcd_panel_t *panel, bool swap_axes);
static esp_err_t panel_spd2010_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap);
static esp_err_t panel_spd2010_disp_on_off(esp_lcd_panel_t *panel, bool off);

// SPD2010面板对象结构体，包含所有面板相关参数
typedef struct {
    esp_lcd_panel_t base;                  // 基类，面板操作函数指针集合
    esp_lcd_panel_io_handle_t io;          // IO句柄
    int reset_gpio_num;                    // 复位引脚编号
    int x_gap;                             // X方向偏移
    int y_gap;                             // Y方向偏移
    uint8_t fb_bits_per_pixel;             // 帧缓像素位宽
    uint8_t madctl_val;                    // MADCTL寄存器当前值
    uint8_t colmod_val;                    // COLMOD寄存器当前值
    const spd2010_lcd_init_cmd_t *init_cmds; // 初始化命令序列指针
    uint16_t init_cmds_size;               // 初始化命令数量
    struct {
        unsigned int use_qspi_interface: 1; // 是否使用QSPI接口
        unsigned int reset_level: 1;        // 复位电平（高/低有效）
    } flags;
} spd2010_panel_t;

/**
 * @brief 创建SPD2010面板对象
 *
 * @param io IO句柄
 * @param panel_dev_config 面板配置参数
 * @param ret_panel 返回的面板句柄
 * @return esp_err_t
 */
esp_err_t esp_lcd_new_panel_spd2010(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    // 参数有效性检查
    ESP_RETURN_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    esp_err_t ret = ESP_OK;
    spd2010_panel_t *spd2010 = NULL;
    // 分配面板对象内存
    spd2010 = calloc(1, sizeof(spd2010_panel_t));
    ESP_GOTO_ON_FALSE(spd2010, ESP_ERR_NO_MEM, err, TAG, "no mem for spd2010 panel");

    // 配置复位引脚（如有）
    if (panel_dev_config->reset_gpio_num >= 0) {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num,
        };
        ESP_GOTO_ON_ERROR(gpio_config(&io_conf), err, TAG, "configure GPIO for RST line failed");
    }

    // 配置颜色顺序（RGB/BGR）
    switch (panel_dev_config->rgb_ele_order) {
    case LCD_RGB_ELEMENT_ORDER_RGB:
        spd2010->madctl_val = 0;
        break;
    case LCD_RGB_ELEMENT_ORDER_BGR:
        spd2010->madctl_val |= LCD_CMD_BGR_BIT;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported color element order");
        break;
    }

    // 配置像素格式
    uint8_t fb_bits_per_pixel = 0;
    switch (panel_dev_config->bits_per_pixel) {
    case 16: // RGB565
        spd2010->colmod_val = 0x55;
        fb_bits_per_pixel = 16;
        break;
    case 18: // RGB666
        spd2010->colmod_val = 0x66;
        // RGB666每像素3字节
        fb_bits_per_pixel = 24;
        break;
    case 24: // RGB888
        spd2010->colmod_val = 0x77;
        fb_bits_per_pixel = 24;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported pixel width");
        break;
    }

    // 填充面板对象参数
    spd2010->io = io;
    spd2010->reset_gpio_num = panel_dev_config->reset_gpio_num;
    spd2010->fb_bits_per_pixel = fb_bits_per_pixel;
    spd2010_vendor_config_t *vendor_config = (spd2010_vendor_config_t *)panel_dev_config->vendor_config;
    if (vendor_config) {
        spd2010->init_cmds = vendor_config->init_cmds;
        spd2010->init_cmds_size = vendor_config->init_cmds_size;
        spd2010->flags.use_qspi_interface = vendor_config->flags.use_qspi_interface;
    }
    spd2010->flags.reset_level = panel_dev_config->flags.reset_active_high;

    // 绑定面板操作函数
    spd2010->base.del = panel_spd2010_del;
    spd2010->base.reset = panel_spd2010_reset;
    spd2010->base.init = panel_spd2010_init;
    spd2010->base.draw_bitmap = panel_spd2010_draw_bitmap;
    spd2010->base.invert_color = panel_spd2010_invert_color;
    spd2010->base.set_gap = panel_spd2010_set_gap;
    spd2010->base.mirror = panel_spd2010_mirror;
    spd2010->base.swap_xy = panel_spd2010_swap_xy;
    spd2010->base.disp_on_off = panel_spd2010_disp_on_off;
    *ret_panel = &(spd2010->base);
    ESP_LOGD(TAG, "new spd2010 panel @%p", spd2010);

    // ESP_LOGI(TAG, "LCD panel create success, version: %d.%d.%d", ESP_LCD_SPD2010_VER_MAJOR, ESP_LCD_SPD2010_VER_MINOR,
    //          ESP_LCD_SPD2010_VER_PATCH);

    return ESP_OK;

err:
    // 错误处理，释放资源
    if (spd2010) {
        if (panel_dev_config->reset_gpio_num >= 0) {
            gpio_reset_pin(panel_dev_config->reset_gpio_num);
        }
        free(spd2010);
    }
    return ret;
}

/**
 * @brief 发送参数命令（支持QSPI和普通接口）
 *
 * @param spd2010 面板对象
 * @param io IO句柄
 * @param lcd_cmd 命令字节
 * @param param 参数数据
 * @param param_size 参数长度
 * @return esp_err_t
 */
static esp_err_t tx_param(spd2010_panel_t *spd2010, esp_lcd_panel_io_handle_t io, int lcd_cmd, const void *param, size_t param_size)
{
    // QSPI接口命令格式特殊处理
    if (spd2010->flags.use_qspi_interface) {
        lcd_cmd &= 0xff;
        lcd_cmd <<= 8;
        lcd_cmd |= LCD_OPCODE_WRITE_CMD << 24;
    }
    return esp_lcd_panel_io_tx_param(io, lcd_cmd, param, param_size);
}

/**
 * @brief 发送颜色数据（支持QSPI和普通接口）
 *
 * @param spd2010 面板对象
 * @param io IO句柄
 * @param lcd_cmd 命令字节
 * @param param 颜色数据
 * @param param_size 数据长度
 * @return esp_err_t
 */
static esp_err_t tx_color(spd2010_panel_t *spd2010, esp_lcd_panel_io_handle_t io, int lcd_cmd, const void *param, size_t param_size)
{
    // QSPI接口命令格式特殊处理
    if (spd2010->flags.use_qspi_interface) {
        lcd_cmd &= 0xff;
        lcd_cmd <<= 8;
        lcd_cmd |= LCD_OPCODE_WRITE_COLOR << 24;
    }
    return esp_lcd_panel_io_tx_color(io, lcd_cmd, param, param_size);
}

/**
 * @brief 删除面板对象，释放资源
 *
 * @param panel 面板基类指针
 * @return esp_err_t
 */
static esp_err_t panel_spd2010_del(esp_lcd_panel_t *panel)
{
    spd2010_panel_t *spd2010 = __containerof(panel, spd2010_panel_t, base);

    // 释放复位引脚
    if (spd2010->reset_gpio_num >= 0) {
        gpio_reset_pin(spd2010->reset_gpio_num);
    }
    ESP_LOGD(TAG, "del spd2010 panel @%p", spd2010);
    free(spd2010);
    return ESP_OK;
}

/**
 * @brief 面板复位（硬件或软件）
 *
 * @param panel 面板基类指针
 * @return esp_err_t
 */
static esp_err_t panel_spd2010_reset(esp_lcd_panel_t *panel)
{
    spd2010_panel_t *spd2010 = __containerof(panel, spd2010_panel_t, base);
    esp_lcd_panel_io_handle_t io = spd2010->io;

    // 硬件复位
    if (spd2010->reset_gpio_num >= 0) {
        gpio_set_level(spd2010->reset_gpio_num, spd2010->flags.reset_level);
        vTaskDelay(pdMS_TO_TICKS(10)); // 拉高/低10ms
        gpio_set_level(spd2010->reset_gpio_num, !spd2010->flags.reset_level);
        vTaskDelay(pdMS_TO_TICKS(120)); // 拉低/高120ms
    } else { // 软件复位
        ESP_RETURN_ON_ERROR(tx_param(spd2010, io, LCD_CMD_SWRESET, NULL, 0), TAG, "send command failed");
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    return ESP_OK;
}

/**
 * @brief SPD2010默认初始化命令序列
 *        该序列为厂商推荐，实际项目可根据屏厂提供的初始化表调整
 */
static const spd2010_lcd_init_cmd_t vendor_specific_init_default[] = {
//  {cmd, { data }, data_size, delay_ms}
    {0xFF, (uint8_t []){0x20, 0x10, 0x10}, 3, 0},
    {0x0C, (uint8_t []){0x11}, 1, 0},
    {0x10, (uint8_t []){0x02}, 1, 0},
    {0x11, (uint8_t []){0x11}, 1, 0},
    {0x15, (uint8_t []){0x42}, 1, 0},
    {0x16, (uint8_t []){0x11}, 1, 0},
    {0x1A, (uint8_t []){0x02}, 1, 0},
    {0x1B, (uint8_t []){0x11}, 1, 0},
    {0x61, (uint8_t []){0x80}, 1, 0},
    {0x62, (uint8_t []){0x80}, 1, 0},
    {0x54, (uint8_t []){0x44}, 1, 0},
    {0x58, (uint8_t []){0x88}, 1, 0},
    {0x5C, (uint8_t []){0xcc}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x10}, 3, 0},
    {0x20, (uint8_t []){0x80}, 1, 0},
    {0x21, (uint8_t []){0x81}, 1, 0},
    {0x22, (uint8_t []){0x31}, 1, 0},
    {0x23, (uint8_t []){0x20}, 1, 0},
    {0x24, (uint8_t []){0x11}, 1, 0},
    {0x25, (uint8_t []){0x11}, 1, 0},
    {0x26, (uint8_t []){0x12}, 1, 0},
    {0x27, (uint8_t []){0x12}, 1, 0},
    {0x30, (uint8_t []){0x80}, 1, 0},
    {0x31, (uint8_t []){0x81}, 1, 0},
    {0x32, (uint8_t []){0x31}, 1, 0},
    {0x33, (uint8_t []){0x20}, 1, 0},
    {0x34, (uint8_t []){0x11}, 1, 0},
    {0x35, (uint8_t []){0x11}, 1, 0},
    {0x36, (uint8_t []){0x12}, 1, 0},
    {0x37, (uint8_t []){0x12}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x10}, 3, 0},
    {0x41, (uint8_t []){0x11}, 1, 0},
    {0x42, (uint8_t []){0x22}, 1, 0},
    {0x43, (uint8_t []){0x33}, 1, 0},
    {0x49, (uint8_t []){0x11}, 1, 0},
    {0x4A, (uint8_t []){0x22}, 1, 0},
    {0x4B, (uint8_t []){0x33}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x15}, 3, 0},
    {0x00, (uint8_t []){0x00}, 1, 0},
    {0x01, (uint8_t []){0x00}, 1, 0},
    {0x02, (uint8_t []){0x00}, 1, 0},
    {0x03, (uint8_t []){0x00}, 1, 0},
    {0x04, (uint8_t []){0x10}, 1, 0},
    {0x05, (uint8_t []){0x0C}, 1, 0},
    {0x06, (uint8_t []){0x23}, 1, 0},
    {0x07, (uint8_t []){0x22}, 1, 0},
    {0x08, (uint8_t []){0x21}, 1, 0},
    {0x09, (uint8_t []){0x20}, 1, 0},
    {0x0A, (uint8_t []){0x33}, 1, 0},
    {0x0B, (uint8_t []){0x32}, 1, 0},
    {0x0C, (uint8_t []){0x34}, 1, 0},
    {0x0D, (uint8_t []){0x35}, 1, 0},
    {0x0E, (uint8_t []){0x01}, 1, 0},
    {0x0F, (uint8_t []){0x01}, 1, 0},
    {0x20, (uint8_t []){0x00}, 1, 0},
    {0x21, (uint8_t []){0x00}, 1, 0},
    {0x22, (uint8_t []){0x00}, 1, 0},
    {0x23, (uint8_t []){0x00}, 1, 0},
    {0x24, (uint8_t []){0x0C}, 1, 0},
    {0x25, (uint8_t []){0x10}, 1, 0},
    {0x26, (uint8_t []){0x20}, 1, 0},
    {0x27, (uint8_t []){0x21}, 1, 0},
    {0x28, (uint8_t []){0x22}, 1, 0},
    {0x29, (uint8_t []){0x23}, 1, 0},
    {0x2A, (uint8_t []){0x33}, 1, 0},
    {0x2B, (uint8_t []){0x32}, 1, 0},
    {0x2C, (uint8_t []){0x34}, 1, 0},
    {0x2D, (uint8_t []){0x35}, 1, 0},
    {0x2E, (uint8_t []){0x01}, 1, 0},
    {0x2F, (uint8_t []){0x01}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x16}, 3, 0},
    {0x00, (uint8_t []){0x00}, 1, 0},
    {0x01, (uint8_t []){0x00}, 1, 0},
    {0x02, (uint8_t []){0x00}, 1, 0},
    {0x03, (uint8_t []){0x00}, 1, 0},
    {0x04, (uint8_t []){0x08}, 1, 0},
    {0x05, (uint8_t []){0x04}, 1, 0},
    {0x06, (uint8_t []){0x19}, 1, 0},
    {0x07, (uint8_t []){0x18}, 1, 0},
    {0x08, (uint8_t []){0x17}, 1, 0},
    {0x09, (uint8_t []){0x16}, 1, 0},
    {0x0A, (uint8_t []){0x33}, 1, 0},
    {0x0B, (uint8_t []){0x32}, 1, 0},
    {0x0C, (uint8_t []){0x34}, 1, 0},
    {0x0D, (uint8_t []){0x35}, 1, 0},
    {0x0E, (uint8_t []){0x01}, 1, 0},
    {0x0F, (uint8_t []){0x01}, 1, 0},
    {0x20, (uint8_t []){0x00}, 1, 0},
    {0x21, (uint8_t []){0x00}, 1, 0},
    {0x22, (uint8_t []){0x00}, 1, 0},
    {0x23, (uint8_t []){0x00}, 1, 0},
    {0x24, (uint8_t []){0x04}, 1, 0},
    {0x25, (uint8_t []){0x08}, 1, 0},
    {0x26, (uint8_t []){0x16}, 1, 0},
    {0x27, (uint8_t []){0x17}, 1, 0},
    {0x28, (uint8_t []){0x18}, 1, 0},
    {0x29, (uint8_t []){0x19}, 1, 0},
    {0x2A, (uint8_t []){0x33}, 1, 0},
    {0x2B, (uint8_t []){0x32}, 1, 0},
    {0x2C, (uint8_t []){0x34}, 1, 0},
    {0x2D, (uint8_t []){0x35}, 1, 0},
    {0x2E, (uint8_t []){0x01}, 1, 0},
    {0x2F, (uint8_t []){0x01}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x12}, 3, 0},
    {0x00, (uint8_t []){0x99}, 1, 0},
    {0x2A, (uint8_t []){0x28}, 1, 0},
    {0x2B, (uint8_t []){0x0f}, 1, 0},
    {0x2C, (uint8_t []){0x16}, 1, 0},
    {0x2D, (uint8_t []){0x28}, 1, 0},
    {0x2E, (uint8_t []){0x0f}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0xA0}, 3, 0},
    {0x08, (uint8_t []){0xdc}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x45}, 3, 0},
    {0x01, (uint8_t []){0x9C}, 1, 0},
    {0x03, (uint8_t []){0x9C}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x42}, 3, 0},
    {0x05, (uint8_t []){0x2c}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x11}, 3, 0},
    {0x50, (uint8_t []){0x01}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0x2A, (uint8_t []){0x00, 0x00, 0x01, 0x9B}, 4, 0},
    {0x2B, (uint8_t []){0x00, 0x00, 0x01, 0x9B}, 4, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x40}, 3, 0},
    {0x86, (uint8_t []){0x00}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x12}, 3, 0},
    {0x0D, (uint8_t []){0x66}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x17}, 3, 0},
    {0x39, (uint8_t []){0x3c}, 1, 0},
    {0xff, (uint8_t []){0x20, 0x10, 0x31}, 3, 0},
    {0x38, (uint8_t []){0x03}, 1, 0},
    {0x39, (uint8_t []){0xf0}, 1, 0},
    {0x36, (uint8_t []){0x03}, 1, 0},
    {0x37, (uint8_t []){0xe8}, 1, 0},
    {0x34, (uint8_t []){0x03}, 1, 0},
    {0x35, (uint8_t []){0xCF}, 1, 0},
    {0x32, (uint8_t []){0x03}, 1, 0},
    {0x33, (uint8_t []){0xBA}, 1, 0},
    {0x30, (uint8_t []){0x03}, 1, 0},
    {0x31, (uint8_t []){0xA2}, 1, 0},
    {0x2e, (uint8_t []){0x03}, 1, 0},
    {0x2f, (uint8_t []){0x95}, 1, 0},
    {0x2c, (uint8_t []){0x03}, 1, 0},
    {0x2d, (uint8_t []){0x7e}, 1, 0},
    {0x2a, (uint8_t []){0x03}, 1, 0},
    {0x2b, (uint8_t []){0x62}, 1, 0},
    {0x28, (uint8_t []){0x03}, 1, 0},
    {0x29, (uint8_t []){0x44}, 1, 0},
    {0x26, (uint8_t []){0x02}, 1, 0},
    {0x27, (uint8_t []){0xfc}, 1, 0},
    {0x24, (uint8_t []){0x02}, 1, 0},
    {0x25, (uint8_t []){0xd0}, 1, 0},
    {0x22, (uint8_t []){0x02}, 1, 0},
    {0x23, (uint8_t []){0x98}, 1, 0},
    {0x20, (uint8_t []){0x02}, 1, 0},
    {0x21, (uint8_t []){0x6f}, 1, 0},
    {0x1e, (uint8_t []){0x02}, 1, 0},
    {0x1f, (uint8_t []){0x32}, 1, 0},
    {0x1c, (uint8_t []){0x01}, 1, 0},
    {0x1d, (uint8_t []){0xf6}, 1, 0},
    {0x1a, (uint8_t []){0x01}, 1, 0},
    {0x1b, (uint8_t []){0xb8}, 1, 0},
    {0x18, (uint8_t []){0x01}, 1, 0},
    {0x19, (uint8_t []){0x6E}, 1, 0},
    {0x16, (uint8_t []){0x01}, 1, 0},
    {0x17, (uint8_t []){0x41}, 1, 0},
    {0x14, (uint8_t []){0x00}, 1, 0},
    {0x15, (uint8_t []){0xfd}, 1, 0},
    {0x12, (uint8_t []){0x00}, 1, 0},
    {0x13, (uint8_t []){0xCf}, 1, 0},
    {0x10, (uint8_t []){0x00}, 1, 0},
    {0x11, (uint8_t []){0x98}, 1, 0},
    {0x0e, (uint8_t []){0x00}, 1, 0},
    {0x0f, (uint8_t []){0x89}, 1, 0},
    {0x0c, (uint8_t []){0x00}, 1, 0},
    {0x0d, (uint8_t []){0x79}, 1, 0},
    {0x0a, (uint8_t []){0x00}, 1, 0},
    {0x0b, (uint8_t []){0x67}, 1, 0},
    {0x08, (uint8_t []){0x00}, 1, 0},
    {0x09, (uint8_t []){0x55}, 1, 0},
    {0x06, (uint8_t []){0x00}, 1, 0},
    {0x07, (uint8_t []){0x3F}, 1, 0},
    {0x04, (uint8_t []){0x00}, 1, 0},
    {0x05, (uint8_t []){0x28}, 1, 0},
    {0x02, (uint8_t []){0x00}, 1, 0},
    {0x03, (uint8_t []){0x0E}, 1, 0},
    {0xff, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0xff, (uint8_t []){0x20, 0x10, 0x32}, 3, 0},
    {0x38, (uint8_t []){0x03}, 1, 0},
    {0x39, (uint8_t []){0xf0}, 1, 0},
    {0x36, (uint8_t []){0x03}, 1, 0},
    {0x37, (uint8_t []){0xe8}, 1, 0},
    {0x34, (uint8_t []){0x03}, 1, 0},
    {0x35, (uint8_t []){0xCF}, 1, 0},
    {0x32, (uint8_t []){0x03}, 1, 0},
    {0x33, (uint8_t []){0xBA}, 1, 0},
    {0x30, (uint8_t []){0x03}, 1, 0},
    {0x31, (uint8_t []){0xA2}, 1, 0},
    {0x2e, (uint8_t []){0x03}, 1, 0},
    {0x2f, (uint8_t []){0x95}, 1, 0},
    {0x2c, (uint8_t []){0x03}, 1, 0},
    {0x2d, (uint8_t []){0x7e}, 1, 0},
    {0x2a, (uint8_t []){0x03}, 1, 0},
    {0x2b, (uint8_t []){0x62}, 1, 0},
    {0x28, (uint8_t []){0x03}, 1, 0},
    {0x29, (uint8_t []){0x44}, 1, 0},
    {0x26, (uint8_t []){0x02}, 1, 0},
    {0x27, (uint8_t []){0xfc}, 1, 0},
    {0x24, (uint8_t []){0x02}, 1, 0},
    {0x25, (uint8_t []){0xd0}, 1, 0},
    {0x22, (uint8_t []){0x02}, 1, 0},
    {0x23, (uint8_t []){0x98}, 1, 0},
    {0x20, (uint8_t []){0x02}, 1, 0},
    {0x21, (uint8_t []){0x6f}, 1, 0},
    {0x1e, (uint8_t []){0x02}, 1, 0},
    {0x1f, (uint8_t []){0x32}, 1, 0},
    {0x1c, (uint8_t []){0x01}, 1, 0},
    {0x1d, (uint8_t []){0xf6}, 1, 0},
    {0x1a, (uint8_t []){0x01}, 1, 0},
    {0x1b, (uint8_t []){0xb8}, 1, 0},
    {0x18, (uint8_t []){0x01}, 1, 0},
    {0x19, (uint8_t []){0x6E}, 1, 0},
    {0x16, (uint8_t []){0x01}, 1, 0},
    {0x17, (uint8_t []){0x41}, 1, 0},
    {0x14, (uint8_t []){0x00}, 1, 0},
    {0x15, (uint8_t []){0xfd}, 1, 0},
    {0x12, (uint8_t []){0x00}, 1, 0},
    {0x13, (uint8_t []){0xCf}, 1, 0},
    {0x10, (uint8_t []){0x00}, 1, 0},
    {0x11, (uint8_t []){0x98}, 1, 0},
    {0x0e, (uint8_t []){0x00}, 1, 0},
    {0x0f, (uint8_t []){0x89}, 1, 0},
    {0x0c, (uint8_t []){0x00}, 1, 0},
    {0x0d, (uint8_t []){0x79}, 1, 0},
    {0x0a, (uint8_t []){0x00}, 1, 0},
    {0x0b, (uint8_t []){0x67}, 1, 0},
    {0x08, (uint8_t []){0x00}, 1, 0},
    {0x09, (uint8_t []){0x55}, 1, 0},
    {0x06, (uint8_t []){0x00}, 1, 0},
    {0x07, (uint8_t []){0x3F}, 1, 0},
    {0x04, (uint8_t []){0x00}, 1, 0},
    {0x05, (uint8_t []){0x28}, 1, 0},
    {0x02, (uint8_t []){0x00}, 1, 0},
    {0x03, (uint8_t []){0x0E}, 1, 0},
    {0xff, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x11}, 3, 0},
    {0x60, (uint8_t []){0x01}, 1, 0},
    {0x65, (uint8_t []){0x03}, 1, 0},
    {0x66, (uint8_t []){0x38}, 1, 0},
    {0x67, (uint8_t []){0x04}, 1, 0},
    {0x68, (uint8_t []){0x34}, 1, 0},
    {0x69, (uint8_t []){0x03}, 1, 0},
    {0x61, (uint8_t []){0x03}, 1, 0},
    {0x62, (uint8_t []){0x38}, 1, 0},
    {0x63, (uint8_t []){0x04}, 1, 0},
    {0x64, (uint8_t []){0x34}, 1, 0},
    {0x0A, (uint8_t []){0x11}, 1, 0},
    {0x0B, (uint8_t []){0x20}, 1, 0},
    {0x0c, (uint8_t []){0x20}, 1, 0},
    {0x55, (uint8_t []){0x06}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x42}, 3, 0},
    {0x05, (uint8_t []){0x3D}, 1, 0},
    {0x06, (uint8_t []){0x03}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x12}, 3, 0},
    {0x1F, (uint8_t []){0xDC}, 1, 0},
    {0xff, (uint8_t []){0x20, 0x10, 0x17}, 3, 0},
    {0x11, (uint8_t []){0xAA}, 1, 0},
    {0x16, (uint8_t []){0x12}, 1, 0},
    {0x0B, (uint8_t []){0xC3}, 1, 0},
    {0x10, (uint8_t []){0x0E}, 1, 0},
    {0x14, (uint8_t []){0xAA}, 1, 0},
    {0x18, (uint8_t []){0xA0}, 1, 0},
    {0x1A, (uint8_t []){0x80}, 1, 0},
    {0x1F, (uint8_t []){0x80}, 1, 0},
    {0xff, (uint8_t []){0x20, 0x10, 0x11}, 3, 0},
    {0x30, (uint8_t []){0xEE}, 1, 0},
    {0xff, (uint8_t []){0x20, 0x10, 0x12}, 3, 0},
    {0x15, (uint8_t []){0x0F}, 1, 0},
    {0xff, (uint8_t []){0x20, 0x10, 0x2D}, 3, 0},
    {0x01, (uint8_t []){0x3E}, 1, 0},
    {0xff, (uint8_t []){0x20, 0x10, 0x40}, 3, 0},
    {0x83, (uint8_t []){0xC4}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x12}, 3, 0},
    {0x00, (uint8_t []){0xCC}, 1, 0},
    {0x36, (uint8_t []){0xA0}, 1, 0},
    {0x2A, (uint8_t []){0x2D}, 1, 0},
    {0x2B, (uint8_t []){0x1e}, 1, 0},
    {0x2C, (uint8_t []){0x26}, 1, 0},
    {0x2D, (uint8_t []){0x2D}, 1, 0},
    {0x2E, (uint8_t []){0x1e}, 1, 0},
    {0x1F, (uint8_t []){0xE6}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0xA0}, 3, 0},
    {0x08, (uint8_t []){0xE6}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x12}, 3, 0},
    {0x10, (uint8_t []){0x0F}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x18}, 3, 0},
    {0x01, (uint8_t []){0x01}, 1, 0},
    {0x00, (uint8_t []){0x1E}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x43}, 3, 0},
    {0x03, (uint8_t []){0x04}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x18}, 3, 0},
    {0x3A, (uint8_t []){0x01}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x50}, 3, 0},
    {0x05, (uint8_t []){0x08}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x50}, 3, 0},
    {0x00, (uint8_t []){0xA6}, 1, 0},
    {0x01, (uint8_t []){0xA6}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x50}, 3, 0},
    {0x08, (uint8_t []){0x55}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x10}, 3, 0},
    {0x0B, (uint8_t []){0x43}, 1, 0},
    {0x0C, (uint8_t []){0x12}, 1, 0},
    {0x10, (uint8_t []){0x01}, 1, 0},
    {0x11, (uint8_t []){0x12}, 1, 0},
    {0x15, (uint8_t []){0x00}, 1, 0},
    {0x16, (uint8_t []){0x00}, 1, 0},
    {0x1A, (uint8_t []){0x00}, 1, 0},
    {0x1B, (uint8_t []){0x00}, 1, 0},
    {0x61, (uint8_t []){0x00}, 1, 0},
    {0x62, (uint8_t []){0x00}, 1, 0},
    {0x51, (uint8_t []){0x11}, 1, 0},
    {0x55, (uint8_t []){0x55}, 1, 0},
    {0x58, (uint8_t []){0x00}, 1, 0},
    {0x5C, (uint8_t []){0x00}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x10}, 3, 0},
    {0x20, (uint8_t []){0x81}, 1, 0},
    {0x21, (uint8_t []){0x82}, 1, 0},
    {0x22, (uint8_t []){0x72}, 1, 0},
    {0x30, (uint8_t []){0x00}, 1, 0},
    {0x31, (uint8_t []){0x00}, 1, 0},
    {0x32, (uint8_t []){0x00}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x10}, 3, 0},
    {0x44, (uint8_t []){0x44}, 1, 0},
    {0x45, (uint8_t []){0x55}, 1, 0},
    {0x46, (uint8_t []){0x66}, 1, 0},
    {0x47, (uint8_t []){0x77}, 1, 0},
    {0x49, (uint8_t []){0x00}, 1, 0},
    {0x4A, (uint8_t []){0x00}, 1, 0},
    {0x4B, (uint8_t []){0x00}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x17}, 3, 0},
    {0x37, (uint8_t []){0x00}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x15}, 3, 0},
    {0x04, (uint8_t []){0x08}, 1, 0},
    {0x05, (uint8_t []){0x04}, 1, 0},
    {0x06, (uint8_t []){0x1C}, 1, 0},
    {0x07, (uint8_t []){0x1A}, 1, 0},
    {0x08, (uint8_t []){0x18}, 1, 0},
    {0x09, (uint8_t []){0x16}, 1, 0},
    {0x24, (uint8_t []){0x05}, 1, 0},
    {0x25, (uint8_t []){0x09}, 1, 0},
    {0x26, (uint8_t []){0x17}, 1, 0},
    {0x27, (uint8_t []){0x19}, 1, 0},
    {0x28, (uint8_t []){0x1B}, 1, 0},
    {0x29, (uint8_t []){0x1D}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x16}, 3, 0},
    {0x04, (uint8_t []){0x09}, 1, 0},
    {0x05, (uint8_t []){0x05}, 1, 0},
    {0x06, (uint8_t []){0x1D}, 1, 0},
    {0x07, (uint8_t []){0x1B}, 1, 0},
    {0x08, (uint8_t []){0x19}, 1, 0},
    {0x09, (uint8_t []){0x17}, 1, 0},
    {0x24, (uint8_t []){0x04}, 1, 0},
    {0x25, (uint8_t []){0x08}, 1, 0},
    {0x26, (uint8_t []){0x16}, 1, 0},
    {0x27, (uint8_t []){0x18}, 1, 0},
    {0x28, (uint8_t []){0x1A}, 1, 0},
    {0x29, (uint8_t []){0x1C}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x18}, 3, 0},
    {0x1F, (uint8_t []){0x02}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x11}, 3, 0},
    {0x15, (uint8_t []){0x99}, 1, 0},
    {0x16, (uint8_t []){0x99}, 1, 0},
    {0x1C, (uint8_t []){0x88}, 1, 0},
    {0x1D, (uint8_t []){0x88}, 1, 0},
    {0x1E, (uint8_t []){0x88}, 1, 0},
    {0x13, (uint8_t []){0xf0}, 1, 0},
    {0x14, (uint8_t []){0x34}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x12}, 3, 0},
    {0x12, (uint8_t []){0x89}, 1, 0},
    {0x06, (uint8_t []){0x06}, 1, 0},
    {0x18, (uint8_t []){0x00}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x11}, 3, 0},
    {0x0A, (uint8_t []){0x00}, 1, 0},
    {0x0B, (uint8_t []){0xF0}, 1, 0},
    {0x0c, (uint8_t []){0xF0}, 1, 0},
    {0x6A, (uint8_t []){0x10}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x11}, 3, 0},
    {0x08, (uint8_t []){0x70}, 1, 0},
    {0x09, (uint8_t []){0x00}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0x35, (uint8_t []){0x00}, 1, 0},
    // {0x3A, (uint8_t []){0x05}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x12}, 3, 0},
    {0x21, (uint8_t []){0x70}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x2D}, 3, 0},
    {0x02, (uint8_t []){0x00}, 1, 0},
    {0xFF, (uint8_t []){0x20, 0x10, 0x00}, 3, 0},
    {0x11, (uint8_t []){0x00}, 0, 120},
};

/**
 * @brief 面板初始化，发送初始化命令序列
 *
 * @param panel 面板基类指针
 * @return esp_err_t
 */
static esp_err_t panel_spd2010_init(esp_lcd_panel_t *panel)
{
    spd2010_panel_t *spd2010 = __containerof(panel, spd2010_panel_t, base);
    esp_lcd_panel_io_handle_t io = spd2010->io;
    const spd2010_lcd_init_cmd_t *init_cmds = NULL;
    uint16_t init_cmds_size = 0;
    bool is_user_set = true;         // 是否处于用户命令集
    bool is_cmd_overwritten = false; // 标记MADCTL/COLMOD是否被外部初始化覆盖

    // 先切换到用户命令集，并设置MADCTL/COLMOD
    ESP_RETURN_ON_ERROR(tx_param(spd2010, io, SPD2010_CMD_SET, (uint8_t[]) {
        SPD2010_CMD_SET_BYTE0, SPD2010_CMD_SET_BYTE1, SPD2010_CMD_SET_USER
    }, 3), TAG, "send command failed");
    ESP_RETURN_ON_ERROR(tx_param(spd2010, io, LCD_CMD_MADCTL, (uint8_t[]) {
        spd2010->madctl_val,
    }, 1), TAG, "send command failed");
    ESP_RETURN_ON_ERROR(tx_param(spd2010, io, LCD_CMD_COLMOD, (uint8_t[]) {
        spd2010->colmod_val,
    }, 1), TAG, "send command failed");

    // 选择初始化命令表（优先使用用户自定义）
    if (spd2010->init_cmds) {
        init_cmds = spd2010->init_cmds;
        init_cmds_size = spd2010->init_cmds_size;
    } else {
        init_cmds = vendor_specific_init_default;
        init_cmds_size = sizeof(vendor_specific_init_default) / sizeof(spd2010_lcd_init_cmd_t);
    }

    // 依次发送初始化命令
    for (int i = 0; i < init_cmds_size; i++) {
        // 检查是否有外部初始化覆盖MADCTL/COLMOD
        if (is_user_set && (init_cmds[i].data_bytes > 0)) {
            switch (init_cmds[i].cmd) {
            case LCD_CMD_MADCTL:
                is_cmd_overwritten = true;
                spd2010->madctl_val = ((uint8_t *)init_cmds[i].data)[0];
                break;
            case LCD_CMD_COLMOD:
                is_cmd_overwritten = true;
                spd2010->colmod_val = ((uint8_t *)init_cmds[i].data)[0];
                break;
            default:
                is_cmd_overwritten = false;
                break;
            }

            if (is_cmd_overwritten) {
                is_cmd_overwritten = false;
                ESP_LOGW(TAG, "The %02Xh command has been used and will be overwritten by external initialization sequence",
                         init_cmds[i].cmd);
            }
        }

        // 发送命令及参数
        ESP_RETURN_ON_ERROR(tx_param(spd2010, io, init_cmds[i].cmd, init_cmds[i].data, init_cmds[i].data_bytes), TAG,
                            "send command failed");
        vTaskDelay(pdMS_TO_TICKS(init_cmds[i].delay_ms));

        // 检查是否切换命令集
        if ((init_cmds[i].cmd == SPD2010_CMD_SET) && (init_cmds[i].data_bytes > 2)) {
            is_user_set = (((uint8_t *)init_cmds[i].data)[2] == SPD2010_CMD_SET_USER);
        }
    }
    ESP_LOGD(TAG, "send init commands success");

    return ESP_OK;
}

/**
 * @brief 绘制区域图像（写入帧缓）
 *
 * @param panel 面板基类指针
 * @param x_start 区域起始X
 * @param y_start 区域起始Y
 * @param x_end   区域结束X
 * @param y_end   区域结束Y
 * @param color_data 颜色数据指针
 * @return esp_err_t
 */
static esp_err_t panel_spd2010_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    spd2010_panel_t *spd2010 = __containerof(panel, spd2010_panel_t, base);
    // 区域有效性断言
    assert((x_start < x_end) && (y_start < y_end) && "start position must be smaller than end position");
    esp_lcd_panel_io_handle_t io = spd2010->io;

    // 加上偏移
    x_start += spd2010->x_gap;
    x_end += spd2010->x_gap;
    y_start += spd2010->y_gap;
    y_end += spd2010->y_gap;

    // 设置列地址（CASET）
    ESP_RETURN_ON_ERROR(tx_param(spd2010, io, LCD_CMD_CASET, (uint8_t[]) {
        (x_start >> 8) & 0xFF,
        x_start & 0xFF,
        ((x_end - 1) >> 8) & 0xFF,
        (x_end - 1) & 0xFF,
    }, 4), TAG, "send command failed");
    // 设置行地址（RASET）
    ESP_RETURN_ON_ERROR(tx_param(spd2010, io, LCD_CMD_RASET, (uint8_t[]) {
        (y_start >> 8) & 0xFF,
        y_start & 0xFF,
        ((y_end - 1) >> 8) & 0xFF,
        (y_end - 1) & 0xFF,
    }, 4), TAG, "send command failed");
    // 计算区域像素数据长度
    size_t len = (x_end - x_start) * (y_end - y_start) * spd2010->fb_bits_per_pixel / 8;
    // 发送像素数据
    ESP_RETURN_ON_ERROR(tx_color(spd2010, io, LCD_CMD_RAMWR, color_data, len), TAG, "send color failed");

    return ESP_OK;
}

/**
 * @brief 颜色反转控制
 *
 * @param panel 面板基类指针
 * @param invert_color_data 是否反转
 * @return esp_err_t
 */
static esp_err_t panel_spd2010_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    spd2010_panel_t *spd2010 = __containerof(panel, spd2010_panel_t, base);
    esp_lcd_panel_io_handle_t io = spd2010->io;
    int command = 0;
    if (invert_color_data) {
        command = LCD_CMD_INVON;  // 反转
    } else {
        command = LCD_CMD_INVOFF; // 不反转
    }
    ESP_RETURN_ON_ERROR(tx_param(spd2010, io, command, NULL, 0), TAG, "send command failed");
    return ESP_OK;
}

/**
 * @brief 镜像显示控制
 *
 * @param panel 面板基类指针
 * @param mirror_x X方向镜像
 * @param mirror_y Y方向镜像
 * @return esp_err_t
 */
static esp_err_t panel_spd2010_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    spd2010_panel_t *spd2010 = __containerof(panel, spd2010_panel_t, base);
    esp_lcd_panel_io_handle_t io = spd2010->io;
    // 设置MADCTL寄存器的镜像位
    if (mirror_x) {
        spd2010->madctl_val |= BIT(1);
    } else {
        spd2010->madctl_val &= ~BIT(1);
    }
    if (mirror_y) {
        spd2010->madctl_val |= BIT(0);
    } else {
        spd2010->madctl_val &= ~BIT(0);
    }
    ESP_RETURN_ON_ERROR(tx_param(spd2010, io, LCD_CMD_MADCTL, (uint8_t[]) {
        spd2010->madctl_val
    }, 1), TAG, "send command failed");
    return ESP_OK;
}

/**
 * @brief 交换XY轴（SPD2010不支持）
 *
 * @param panel 面板基类指针
 * @param swap_axes 是否交换
 * @return esp_err_t
 */
static esp_err_t panel_spd2010_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    ESP_LOGE(TAG, "swap_xy is not supported by this panel");
    return ESP_ERR_NOT_SUPPORTED;
}

/**
 * @brief 设置显示区域偏移
 *
 * @param panel 面板基类指针
 * @param x_gap X方向偏移
 * @param y_gap Y方向偏移
 * @return esp_err_t
 */
static esp_err_t panel_spd2010_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
{
    spd2010_panel_t *spd2010 = __containerof(panel, spd2010_panel_t, base);
    spd2010->x_gap = x_gap;
    spd2010->y_gap = y_gap;
    return ESP_OK;
}

/**
 * @brief 控制显示开关
 *
 * @param panel 面板基类指针
 * @param on_off true: 开显示, false: 关显示
 * @return esp_err_t
 */
static esp_err_t panel_spd2010_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    spd2010_panel_t *spd2010 = __containerof(panel, spd2010_panel_t, base);
    esp_lcd_panel_io_handle_t io = spd2010->io;
    int command = 0;

    if (on_off) {
        command = LCD_CMD_DISPON;  // 开显示
    } else {
        command = LCD_CMD_DISPOFF; // 关显示
    }
    ESP_RETURN_ON_ERROR(tx_param(spd2010, io, command, NULL, 0), TAG, "send command failed");
    return ESP_OK;
}
