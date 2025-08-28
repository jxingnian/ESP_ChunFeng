/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-28 15:16:27
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-28 15:32:51
 * @FilePath: \esp-chunfeng\main\LCD_Driver\Display_SPD2010.c
 * @Description: SPD2010 LCD驱动
 *
 */
#include "Display_SPD2010.h"

// 日志标签，用于ESP_LOG系列函数输出日志
static const char *TAG_LCD = "SPD2010";

// LCD面板句柄，用于操作LCD面板
esp_lcd_panel_handle_t panel_handle = NULL;

/**
 * @brief SPD2010复位函数
 *
 * 通过控制EXIO2引脚实现SPD2010的硬件复位
 * 复位时序：拉低100ms -> 拉高100ms
 */
void SPD2010_Reset()
{
    Set_EXIO(TCA9554_EXIO2, false);       // 拉低复位引脚
    vTaskDelay(pdMS_TO_TICKS(100));       // 延时100ms
    Set_EXIO(TCA9554_EXIO2, true);        // 拉高复位引脚
    vTaskDelay(pdMS_TO_TICKS(100));       // 延时100ms
}

/**
 * @brief LCD初始化主函数
 *
 * 按顺序初始化LCD相关组件：
 * 1. SPD2010显示驱动初始化
 * 2. 背光初始化
 * 3. 触摸屏初始化
 */
void LCD_Init()
{
    SPD2010_Init();       // 初始化SPD2010显示驱动
    Backlight_Init();     // 初始化背光控制
    Touch_Init();         // 初始化触摸屏
}

/**
 * @brief 测试位图绘制函数
 *
 * 在LCD上绘制彩色条纹测试图案，用于验证LCD显示功能
 *
 * @param panel_handle LCD面板句柄
 */
static void test_draw_bitmap(esp_lcd_panel_handle_t panel_handle)
{
    // 计算每行像素数据大小
    uint16_t row_line = ((EXAMPLE_LCD_WIDTH / EXAMPLE_LCD_COLOR_BITS) << 1) >> 1;
    // 计算每像素字节数
    uint8_t byte_per_pixel = EXAMPLE_LCD_COLOR_BITS / 8;
    // 分配DMA兼容的颜色缓冲区
    uint8_t *color = (uint8_t *)heap_caps_calloc(1, row_line * EXAMPLE_LCD_HEIGHT * byte_per_pixel, MALLOC_CAP_DMA);

    // 绘制彩色条纹，每个颜色位对应一个条纹
    for (int j = 0; j < EXAMPLE_LCD_COLOR_BITS; j++) {
        // 填充当前条纹的颜色数据
        for (int i = 0; i < row_line * EXAMPLE_LCD_HEIGHT; i++) {
            for (int k = 0; k < byte_per_pixel; k++) {
                // 根据当前位生成颜色值并进行字节序转换
                color[i * byte_per_pixel + k] = (SPI_SWAP_DATA_TX(BIT(j), EXAMPLE_LCD_COLOR_BITS) >> (k * 8)) & 0xff;
            }
        }
        // 绘制当前条纹到LCD
        esp_lcd_panel_draw_bitmap(panel_handle, 0, j * row_line, EXAMPLE_LCD_HEIGHT, (j + 1) * row_line, color);
    }
    // 释放颜色缓冲区内存
    free(color);
}

/**
 * @brief QSPI接口初始化函数
 *
 * 初始化QSPI总线和LCD面板IO，配置SPD2010驱动
 *
 * @return int 初始化结果
 *         - 1: 初始化成功
 *         - 0: 初始化失败
 */
int QSPI_Init(void)
{
    // QSPI总线配置结构体
    static const spi_bus_config_t host_config = {
        .data0_io_num = ESP_PANEL_LCD_SPI_IO_DATA0,     // 数据线0 GPIO引脚
        .data1_io_num = ESP_PANEL_LCD_SPI_IO_DATA1,     // 数据线1 GPIO引脚
        .sclk_io_num = ESP_PANEL_LCD_SPI_IO_SCK,        // 时钟线 GPIO引脚
        .data2_io_num = ESP_PANEL_LCD_SPI_IO_DATA2,     // 数据线2 GPIO引脚
        .data3_io_num = ESP_PANEL_LCD_SPI_IO_DATA3,     // 数据线3 GPIO引脚
        .data4_io_num = -1,                             // 数据线4 未使用
        .data5_io_num = -1,                             // 数据线5 未使用
        .data6_io_num = -1,                             // 数据线6 未使用
        .data7_io_num = -1,                             // 数据线7 未使用
        .max_transfer_sz = ESP_PANEL_HOST_SPI_MAX_TRANSFER_SIZE,  // 最大传输大小
        .flags = SPICOMMON_BUSFLAG_MASTER,              // 主机模式标志
        .intr_flags = 0,                                // 中断标志
    };

    // 初始化SPI总线
    if (spi_bus_initialize(ESP_PANEL_HOST_SPI_ID_DEFAULT, &host_config, SPI_DMA_CH_AUTO) != ESP_OK) {
        printf("The SPI initialization failed.\r\n");
        return 0;
    }
    printf("The SPI initialization succeeded.\r\n");

    // LCD面板IO配置结构体
    const esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = ESP_PANEL_LCD_SPI_IO_CS,         // 片选引脚
        .dc_gpio_num = -1,                              // 数据/命令引脚（QSPI模式下不使用）
        .spi_mode = ESP_PANEL_LCD_SPI_MODE,             // SPI模式
        .pclk_hz = ESP_PANEL_LCD_SPI_CLK_HZ,            // 时钟频率
        .trans_queue_depth = ESP_PANEL_LCD_SPI_TRANS_QUEUE_SZ,  // 传输队列深度
        .on_color_trans_done = NULL,                    // 颜色传输完成回调
        .user_ctx = NULL,                               // 用户上下文
        .lcd_cmd_bits = ESP_PANEL_LCD_SPI_CMD_BITS,     // 命令位数
        .lcd_param_bits = ESP_PANEL_LCD_SPI_PARAM_BITS, // 参数位数
        .flags = {                                      // 标志位配置
            .dc_low_on_data = 0,                          // 数据时DC引脚电平
            .octal_mode = 0,                              // 八线模式
            .quad_mode = 1,                               // 四线模式（QSPI）
            .sio_mode = 0,                                // 单线模式
            .lsb_first = 0,                               // 最低位优先
            .cs_high_active = 0,                          // 片选高电平有效
        },
    };

    // 创建LCD面板IO句柄
    esp_lcd_panel_io_handle_t io_handle = NULL;
    if (esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)ESP_PANEL_HOST_SPI_ID_DEFAULT, &io_config, &io_handle) != ESP_OK) {
        printf("Failed to set LCD communication parameters -- SPI\r\n");
        return 0;
    }
    printf("LCD communication parameters are set successfully -- SPI\r\n");

    printf("Install LCD driver of SPD2010\r\n");

    // SPD2010厂商特定配置
    spd2010_vendor_config_t vendor_config = {
        .flags = {
            .use_qspi_interface = 1,                      // 使用QSPI接口
        },
    };

    // LCD面板设备配置
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_LCD_PIN_NUM_RST,      // 复位引脚（-1表示不使用）
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,     // RGB像素顺序
        // .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,   // 数据字节序（已注释）
        .bits_per_pixel = EXAMPLE_LCD_COLOR_BITS,       // 每像素位数
        .flags = {                                      // 标志位配置
            .reset_active_high = 0,                       // 复位低电平有效
        },
        .vendor_config = (void *) &vendor_config,      // 厂商特定配置
    };

    // 创建SPD2010面板
    esp_lcd_new_panel_spd2010(io_handle, &panel_config, &panel_handle);

    // 复位LCD面板
    esp_lcd_panel_reset(panel_handle);
    // 初始化LCD面板
    esp_lcd_panel_init(panel_handle);
    // esp_lcd_panel_invert_color(panel_handle,false);  // 颜色反转（已注释）

    // 打开显示
    esp_lcd_panel_disp_on_off(panel_handle, true);
    // 绘制测试图案
    test_draw_bitmap(panel_handle);
    return 1;
}

/**
 * @brief SPD2010初始化函数
 *
 * 执行SPD2010的完整初始化流程：
 * 1. 硬件复位
 * 2. QSPI接口初始化
 */
void SPD2010_Init()
{
    SPD2010_Reset();      // 执行硬件复位
    if (!QSPI_Init()) {   // 初始化QSPI接口
        printf("SPD2010 Failed to be initialized\r\n");
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 背光控制程序

// LCD背光亮度全局变量，默认70%
uint8_t LCD_Backlight = 70;
// LEDC通道配置结构体
static ledc_channel_config_t ledc_channel;

/**
 * @brief 背光初始化函数
 *
 * 配置GPIO和LEDC定时器，初始化PWM背光控制
 * 使用LEDC模块生成PWM信号控制背光亮度
 */
void Backlight_Init(void)
{
    ESP_LOGI(TAG_LCD, "Turn off LCD backlight");

    // 配置背光控制GPIO
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,                           // 输出模式
        .pin_bit_mask = 1ULL << EXAMPLE_LCD_PIN_NUM_BK_LIGHT  // 背光引脚位掩码
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    // 配置LEDC定时器
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT,   // 13位占空比分辨率
        .freq_hz = 5000,                        // PWM频率5kHz
        .speed_mode = LEDC_LS_MODE,             // 低速模式
        .timer_num = LEDC_HS_TIMER,             // 定时器编号
        .clk_cfg = LEDC_AUTO_CLK                // 自动时钟配置
    };
    ledc_timer_config(&ledc_timer);

    // 配置LEDC通道
    ledc_channel.channel    = LEDC_HS_CH0_CHANNEL;      // 通道编号
    ledc_channel.duty       = 0;                        // 初始占空比为0（关闭背光）
    ledc_channel.gpio_num   = EXAMPLE_LCD_PIN_NUM_BK_LIGHT;  // GPIO引脚
    ledc_channel.speed_mode = LEDC_LS_MODE;             // 低速模式
    ledc_channel.timer_sel  = LEDC_HS_TIMER;            // 选择定时器
    ledc_channel_config(&ledc_channel);

    // 安装LEDC渐变功能（中断标志为0）
    ledc_fade_func_install(0);

    // 设置默认背光亮度（0~100）
    Set_Backlight(LCD_Backlight);
}

/**
 * @brief 设置背光亮度函数
 *
 * 通过调整PWM占空比来控制背光亮度
 *
 * @param Light 背光亮度值，范围0-100
 *              - 0: 完全关闭背光
 *              - 100: 最大亮度
 */
void Set_Backlight(uint8_t Light)
{
    // 限制亮度值在有效范围内
    if (Light > Backlight_MAX) Light = Backlight_MAX;

    // 计算PWM占空比
    // 使用反向逻辑：最大占空比对应最大亮度
    // 公式：Duty = LEDC_MAX_Duty - 81*(Backlight_MAX-Light)
    uint16_t Duty = LEDC_MAX_Duty - (81 * (Backlight_MAX - Light));

    // 特殊处理：亮度为0时完全关闭
    if (Light == 0)
        Duty = 0;

    // 设置LEDC通道占空比
    ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, Duty);
    // 更新占空比设置
    ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
}
// 背光控制程序结束