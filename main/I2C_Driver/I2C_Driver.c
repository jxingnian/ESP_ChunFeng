/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-28 15:16:27
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-28 15:32:51
 * @FilePath: \esp-chunfeng\main\LCD_Driver\Display_SPD2010.c
 * @Description: I2C驱动实现文件，包含I2C初始化、读写操作接口
 */
#include "I2C_Driver.h"

// 定义I2C传输缓冲区的最小大小，需为每个命令分配一个i2c_cmd_desc_t结构体：
// start + 写(设备地址) + 写缓冲区 + start + 写(设备地址) + 读缓冲区 + 读缓冲区(NACK) + stop
#define I2C_TRANS_BUF_MINIMUM_SIZE     (sizeof(i2c_cmd_desc_t) + \
                                        sizeof(i2c_cmd_link_t) * 8)

static const char *I2C_TAG = "I2C"; // I2C日志标签

/**
 * @brief I2C主机初始化函数
 *
 * 该函数用于初始化I2C主机模式，配置I2C相关参数并安装I2C驱动。
 *
 * @return esp_err_t 返回ESP_OK表示初始化成功，否则返回错误码
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM; // I2C主机端口号

    // 配置I2C参数结构体
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,              // 设置为主机模式
        .sda_io_num = I2C_SDA_IO,             // SDA引脚号
        .scl_io_num = I2C_SCL_IO,             // SCL引脚号
        .sda_pullup_en = GPIO_PULLUP_ENABLE,  // 启用SDA上拉
        .scl_pullup_en = GPIO_PULLUP_ENABLE,  // 启用SCL上拉
        .master.clk_speed = I2C_MASTER_FREQ_HZ, // 设置I2C时钟频率
    };

    // 应用I2C参数配置
    i2c_param_config(i2c_master_port, &conf);

    // 安装I2C驱动，禁用接收和发送缓冲区
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

/**
 * @brief I2C初始化接口
 *
 * 调用i2c_master_init()进行I2C初始化，并输出初始化成功日志。
 */
void I2C_Init(void)
{
    /********************* I2C *********************/
    ESP_ERROR_CHECK(i2c_master_init()); // 检查I2C初始化是否成功
    ESP_LOGI(I2C_TAG, "I2C initialized successfully");  // 打印初始化成功日志
}

/**
 * @brief I2C写操作
 *
 * 向指定I2C设备的寄存器写入数据。
 *
 * @param Driver_addr  目标I2C设备地址
 * @param Reg_addr     目标寄存器地址（8位）
 * @param Reg_data     待写入的数据指针
 * @param Length       写入数据长度
 * @return esp_err_t   返回ESP_OK表示写入成功，否则返回错误码
 */
esp_err_t I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length)
{
    uint8_t buf[Length + 1]; // 创建缓冲区，首字节为寄存器地址，后面为数据

    buf[0] = Reg_addr; // 设置寄存器地址
    // 将Reg_data拷贝到buf[1]开始的位置
    memcpy(&buf[1], Reg_data, Length);
    // 调用ESP-IDF I2C写接口，向设备写入数据
    return i2c_master_write_to_device(I2C_MASTER_NUM, Driver_addr, buf, Length + 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * @brief I2C读操作
 *
 * 从指定I2C设备的寄存器读取数据。
 *
 * @param Driver_addr  目标I2C设备地址
 * @param Reg_addr     目标寄存器地址（8位）
 * @param Reg_data     读取数据存放的缓冲区指针
 * @param Length       读取数据长度
 * @return esp_err_t   返回ESP_OK表示读取成功，否则返回错误码
 */
esp_err_t I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length)
{
    // 调用ESP-IDF I2C读接口，先写寄存器地址，再读数据
    return i2c_master_write_read_device(I2C_MASTER_NUM, Driver_addr, &Reg_addr, 1, Reg_data, Length, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}
