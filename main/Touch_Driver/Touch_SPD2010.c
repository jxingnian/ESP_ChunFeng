/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-28 15:16:27
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-28 15:32:51
 * @FilePath: \esp-chunfeng\main\LCD_Driver\Display_SPD2010.c
 * @Description: SPD2010 触摸驱动实现
 *
 */
#include "Touch_SPD2010.h"

// 全局触摸数据结构体，保存当前触摸点信息
SPD2010_Touch touch_data = {0};
// 触摸中断标志
uint8_t Touch_interrupts = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 通过I2C读取触摸芯片寄存器
 * @param Driver_addr  设备I2C地址
 * @param Reg_addr     寄存器地址（16位）
 * @param Reg_data     读取数据缓冲区
 * @param Length       读取长度
 * @return esp_err_t   ESP_OK表示成功
 */
esp_err_t I2C_Read_Touch(uint8_t Driver_addr, uint16_t Reg_addr, uint8_t *Reg_data, uint32_t Length)
{
    uint8_t buf_Addr[2];
    buf_Addr[0] = (uint8_t)(Reg_addr >> 8);   // 高8位
    buf_Addr[1] = (uint8_t)Reg_addr;          // 低8位
    // 先写寄存器地址，再读数据
    return i2c_master_write_read_device(I2C_MASTER_NUM, Driver_addr, buf_Addr, 2, Reg_data, Length, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * @brief 通过I2C写入触摸芯片寄存器
 * @param Driver_addr  设备I2C地址
 * @param Reg_addr     寄存器地址（16位）
 * @param Reg_data     待写入数据缓冲区
 * @param Length       写入长度
 * @return esp_err_t   ESP_OK表示成功
 */
esp_err_t I2C_Write_Touch(uint8_t Driver_addr, uint16_t Reg_addr, uint8_t *Reg_data, uint32_t Length)
{
    uint8_t buf[Length + 2];
    buf[0] = (uint8_t)(Reg_addr >> 8);   // 高8位
    buf[1] = (uint8_t)Reg_addr;          // 低8位
    // 将数据拷贝到buf[2]之后
    memcpy(&buf[2], Reg_data, Length);
    // 一次性写入寄存器地址和数据
    return i2c_master_write_to_device(I2C_MASTER_NUM, Driver_addr, buf, Length + 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 触摸芯片初始化
 * @return uint8_t  总是返回true
 */
uint8_t Touch_Init(void)
{
    SPD2010_Touch_Reset();   // 复位触摸芯片
    SPD2010_Read_cfg();      // 读取配置（固件版本等）
    return true;
}

/**
 * @brief 复位SPD2010触摸控制器
 * @return uint8_t  总是返回true
 */
uint8_t SPD2010_Touch_Reset(void)
{
    Set_EXIO(TCA9554_EXIO1, false);              // 拉低复位引脚
    vTaskDelay(pdMS_TO_TICKS(50));               // 延时50ms
    Set_EXIO(TCA9554_EXIO1, true);               // 拉高复位引脚
    vTaskDelay(pdMS_TO_TICKS(50));               // 延时50ms
    return true;
}

/**
 * @brief 读取SPD2010配置（固件版本等）
 * @return uint16_t 固定返回1
 */
uint16_t SPD2010_Read_cfg(void)
{
    read_fw_version();   // 读取固件版本
    return 1;
}

/**
 * @brief 读取触摸数据并更新全局touch_data
 *        只保存最大支持点数以内的触摸点
 */
void Touch_Read_Data(void)
{
    uint8_t touch_cnt = 0;
    SPD2010_Touch touch = {0};
    tp_read_data(&touch);   // 读取触摸点数据到临时结构体

    // 限制触摸点数量不超过最大支持点数
    touch_cnt = (touch.touch_num > CONFIG_ESP_LCD_TOUCH_MAX_POINTS ? CONFIG_ESP_LCD_TOUCH_MAX_POINTS : touch.touch_num);
    touch_data.touch_num = touch_cnt;

    // 拷贝每个触摸点的坐标和压力值到全局touch_data
    for (int i = 0; i < touch_cnt; i++) {
        touch_data.rpt[i].x = touch.rpt[i].x;
        touch_data.rpt[i].y = touch.rpt[i].y;
        touch_data.rpt[i].weight = touch.rpt[i].weight;
    }
}

/**
 * @brief 获取所有触摸点的坐标和压力
 * @param x           x坐标数组
 * @param y           y坐标数组
 * @param strength    压力数组（可为NULL）
 * @param point_num   实际返回的点数
 * @param max_point_num 允许返回的最大点数
 * @return bool       是否有触摸点
 */
bool Touch_Get_xy(uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num)
{
    Touch_Read_Data(); // 读取最新触摸数据

    // 计算实际返回的点数
    *point_num = (touch_data.touch_num > max_point_num ? max_point_num : touch_data.touch_num);

    // 拷贝每个点的坐标和压力
    for (size_t i = 0; i < *point_num; i++) {
        x[i] = touch_data.rpt[i].x;
        y[i] = touch_data.rpt[i].y;
        if (strength) {
            strength[i] = touch_data.rpt[i].weight;
        }
    }
    // 清除全局触摸点数，防止重复读取
    touch_data.touch_num = 0;
    return (*point_num > 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 发送“点模式”命令到触摸芯片
 * @return esp_err_t
 */
esp_err_t write_tp_point_mode_cmd()
{
    uint8_t sample_data[4];
    sample_data[0] = 0x50; // 命令高字节
    sample_data[1] = 0x00; // 命令低字节
    sample_data[2] = 0x00; // 数据
    sample_data[3] = 0x00; // 数据
    // 发送命令
    I2C_Write_Touch(SPD2010_ADDR, (((uint16_t)sample_data[0] << 8) | (sample_data[1])), &sample_data[2], 2);
    esp_rom_delay_us(200); // 延时200us
    return ESP_OK;
}

/**
 * @brief 发送“开始采样”命令到触摸芯片
 * @return esp_err_t
 */
esp_err_t write_tp_start_cmd()
{
    uint8_t sample_data[4];
    sample_data[0] = 0x46;
    sample_data[1] = 0x00;
    sample_data[2] = 0x00;
    sample_data[3] = 0x00;
    I2C_Write_Touch(SPD2010_ADDR, (((uint16_t)sample_data[0] << 8) | (sample_data[1])), &sample_data[2], 2);
    esp_rom_delay_us(200);
    return ESP_OK;
}

/**
 * @brief 发送“CPU启动”命令到触摸芯片
 * @return esp_err_t
 */
esp_err_t write_tp_cpu_start_cmd()
{
    uint8_t sample_data[4];
    sample_data[0] = 0x04;
    sample_data[1] = 0x00;
    sample_data[2] = 0x01;
    sample_data[3] = 0x00;
    I2C_Write_Touch(SPD2010_ADDR, (((uint16_t)sample_data[0] << 8) | (sample_data[1])), &sample_data[2], 2);
    esp_rom_delay_us(200);
    return ESP_OK;
}

/**
 * @brief 发送“清除中断”命令到触摸芯片
 * @return esp_err_t
 */
esp_err_t write_tp_clear_int_cmd()
{
    uint8_t sample_data[4];
    sample_data[0] = 0x02;
    sample_data[1] = 0x00;
    sample_data[2] = 0x01;
    sample_data[3] = 0x00;
    I2C_Write_Touch(SPD2010_ADDR, (((uint16_t)sample_data[0] << 8) | (sample_data[1])), &sample_data[2], 2);
    esp_rom_delay_us(200);
    return ESP_OK;
}

/**
 * @brief 读取触摸状态和数据长度
 * @param tp_status  状态结构体指针
 * @return esp_err_t
 */
esp_err_t read_tp_status_length(tp_status_t *tp_status)
{
    uint8_t sample_data[4];
    sample_data[0] = 0x20; // 状态寄存器高字节
    sample_data[1] = 0x00; // 状态寄存器低字节
    // 读取4字节状态数据
    I2C_Read_Touch(SPD2010_ADDR, (((uint16_t)sample_data[0] << 8) | (sample_data[1])), sample_data, 4);
    esp_rom_delay_us(200);

    // 解析状态低字节
    tp_status->status_low.pt_exist = (sample_data[0] & 0x01);      // 是否有触摸点
    tp_status->status_low.gesture = (sample_data[0] & 0x02);       // 是否有手势
    tp_status->status_low.aux = ((sample_data[0] & 0x08));         // 辅助标志

    // 解析状态高字节
    tp_status->status_high.tic_busy = ((sample_data[1] & 0x80) >> 7);      // 控制器忙
    tp_status->status_high.tic_in_bios = ((sample_data[1] & 0x40) >> 6);   // BIOS模式
    tp_status->status_high.tic_in_cpu = ((sample_data[1] & 0x20) >> 5);    // CPU模式
    tp_status->status_high.tint_low = ((sample_data[1] & 0x10) >> 4);      // 低电平中断
    tp_status->status_high.cpu_run = ((sample_data[1] & 0x08) >> 3);       // CPU运行

    // 读取数据长度（低字节在[2]，高字节在[3]）
    tp_status->read_len = (sample_data[3] << 8 | sample_data[2]);
    return ESP_OK;
}

/**
 * @brief 读取HDP数据包（触摸点详细信息）
 * @param tp_status  状态结构体指针
 * @param touch      触摸点结构体指针
 * @return esp_err_t
 */
esp_err_t read_tp_hdp(tp_status_t *tp_status, SPD2010_Touch *touch)
{
    uint8_t sample_data[4 + (10 * 6)]; // 4字节头 + 最多10个手指*6字节
    uint8_t i, offset;
    uint8_t check_id;

    sample_data[0] = 0x00;
    sample_data[1] = 0x03;
    // 读取HDP数据包
    I2C_Read_Touch(SPD2010_ADDR, (((uint16_t)sample_data[0] << 8) | (sample_data[1])), sample_data, tp_status->read_len);

    check_id = sample_data[4]; // 第一个触摸点的ID或手势标志

    // 判断是否为有效触摸点数据
    if ((check_id <= 0x0A) && tp_status->status_low.pt_exist) {
        // 计算触摸点数量
        touch->touch_num = ((tp_status->read_len - 4) / 6);
        touch->gesture = 0x00;
        // 解析每个触摸点的数据
        for (i = 0; i < touch->touch_num; i++) {
            offset = i * 6;
            touch->rpt[i].id = sample_data[4 + offset]; // 点ID
            // X坐标：高4位在[7+offset]，低8位在[5+offset]
            touch->rpt[i].x = (((sample_data[7 + offset] & 0xF0) << 4) | sample_data[5 + offset]);
            // Y坐标：高4位在[7+offset]，低8位在[6+offset]
            touch->rpt[i].y = (((sample_data[7 + offset] & 0x0F) << 8) | sample_data[6 + offset]);
            touch->rpt[i].weight = sample_data[8 + offset]; // 压力值
        }
        // 滑动手势识别辅助
        if ((touch->rpt[0].weight != 0) && (touch->down != 1)) {
            touch->down = 1;
            touch->up = 0;
            touch->down_x = touch->rpt[0].x;
            touch->down_y = touch->rpt[0].y;
        } else if ((touch->rpt[0].weight == 0) && (touch->down == 1)) {
            touch->up = 1;
            touch->down = 0;
            touch->up_x = touch->rpt[0].x;
            touch->up_y = touch->rpt[0].y;
        }
        // // 调试输出每个手指信息
        // for (uint8_t finger_num = 0; finger_num < touch->touch_num; finger_num++) {
        //     printf("ID[%d], X[%d], Y[%d], Weight[%d]\n",
        //         touch->rpt[finger_num].id,
        //         touch->rpt[finger_num].x,
        //         touch->rpt[finger_num].y,
        //         touch->rpt[finger_num].weight);
        // }
    }
    // 判断是否为手势包
    else if ((check_id == 0xF6) && tp_status->status_low.gesture) {
        touch->touch_num = 0x00;
        touch->up = 0;
        touch->down = 0;
        touch->gesture = sample_data[6] & 0x07; // 低3位为手势类型
        printf("gesture : 0x%02x\n", touch->gesture);
    }
    // 其他情况，清空触摸点
    else {
        touch->touch_num = 0x00;
        touch->gesture = 0x00;
    }
    return ESP_OK;
}

/**
 * @brief 读取HDP状态包
 * @param tp_hdp_status  状态结构体指针
 * @return esp_err_t
 */
esp_err_t read_tp_hdp_status(tp_hdp_status_t *tp_hdp_status)
{
    uint8_t sample_data[8];
    sample_data[0] = 0xFC;
    sample_data[1] = 0x02;
    // 读取8字节HDP状态包
    I2C_Read_Touch(SPD2010_ADDR, (((uint16_t)sample_data[0] << 8) | (sample_data[1])), sample_data, 8);
    tp_hdp_status->status = sample_data[5]; // 状态字节
    // 下一个包的长度（低字节在[2]，高字节在[3]）
    tp_hdp_status->next_packet_len = (sample_data[2] | sample_data[3] << 8);
    return ESP_OK;
}

/**
 * @brief 读取HDP剩余数据包
 * @param tp_hdp_status  状态结构体指针
 * @return esp_err_t
 */
esp_err_t Read_HDP_REMAIN_DATA(tp_hdp_status_t *tp_hdp_status)
{
    uint8_t sample_data[32];
    sample_data[0] = 0x00;
    sample_data[1] = 0x03;
    // 读取剩余数据包
    I2C_Read_Touch(SPD2010_ADDR, (((uint16_t)sample_data[0] << 8) | (sample_data[1])), sample_data, tp_hdp_status->next_packet_len);
    return ESP_OK;
}

/**
 * @brief 读取SPD2010固件版本信息
 * @return esp_err_t
 */
esp_err_t read_fw_version()
{
    uint8_t sample_data[18];
    uint16_t DVer;
    uint32_t Dummy, PID, ICName_H, ICName_L;
    sample_data[0] = 0x26;
    sample_data[1] = 0x00;
    // 读取18字节固件信息
    I2C_Read_Touch(SPD2010_ADDR, (((uint16_t)sample_data[0] << 8) | (sample_data[1])), sample_data, 18);

    // 解析各项信息
    Dummy = ((sample_data[0] << 24) | (sample_data[1] << 16) | (sample_data[3] << 8) | (sample_data[0]));
    DVer = ((sample_data[5] << 8) | (sample_data[4]));
    PID = ((sample_data[9] << 24) | (sample_data[8] << 16) | (sample_data[7] << 8) | (sample_data[6]));
    ICName_L = ((sample_data[13] << 24) | (sample_data[12] << 16) | (sample_data[11] << 8) | (sample_data[10]));    // "2010"
    ICName_H = ((sample_data[17] << 24) | (sample_data[16] << 16) | (sample_data[15] << 8) | (sample_data[14]));    // "SPD"
    printf("Dummy[%ld], DVer[%d], PID[%ld], Name[%ld-%ld]\r\n", Dummy, DVer, PID, ICName_H, ICName_L);
    return ESP_OK;
}

/**
 * @brief 读取触摸点数据主流程
 * @param touch 触摸点结构体指针
 * @return esp_err_t
 */
esp_err_t tp_read_data(SPD2010_Touch *touch)
{
    tp_status_t tp_status = {0};
    tp_hdp_status_t tp_hdp_status = {0};

    // 读取触摸状态和数据长度
    read_tp_status_length(&tp_status);

    // 1. 控制器处于BIOS模式，需启动CPU
    if (tp_status.status_high.tic_in_bios) {
        write_tp_clear_int_cmd();      // 清除中断
        write_tp_cpu_start_cmd();      // 启动CPU
    }
    // 2. 控制器处于CPU模式，需切换到点模式并启动
    else if (tp_status.status_high.tic_in_cpu) {
        write_tp_point_mode_cmd();     // 切换点模式
        write_tp_start_cmd();          // 启动采样
        write_tp_clear_int_cmd();      // 清除中断
    }
    // 3. CPU已运行但无数据，清除中断
    else if (tp_status.status_high.cpu_run && tp_status.read_len == 0) {
        write_tp_clear_int_cmd();
    }
    // 4. 有触摸点或手势，读取HDP数据
    else if (tp_status.status_low.pt_exist || tp_status.status_low.gesture) {
        read_tp_hdp(&tp_status, touch);    // 读取HDP数据
    hdp_done_check:
        read_tp_hdp_status(&tp_hdp_status); // 读取HDP状态
        if (tp_hdp_status.status == 0x82) {
            write_tp_clear_int_cmd();      // 清除中断
        } else if (tp_hdp_status.status == 0x00) {
            Read_HDP_REMAIN_DATA(&tp_hdp_status); // 读取剩余数据
            goto hdp_done_check;                  // 检查状态直到完成
        }
    }
    // 5. CPU运行且AUX标志，清除中断
    else if (tp_status.status_high.cpu_run && tp_status.status_low.aux) {
        write_tp_clear_int_cmd();
    }

    return ESP_OK;
}
