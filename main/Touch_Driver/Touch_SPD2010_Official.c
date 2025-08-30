/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-30 16:30:00
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-30 16:30:00
 * @FilePath: \esp-chunfeng\main\Touch_Driver\Touch_SPD2010_Official.c
 * @Description: 使用官方esp_lcd_touch_spd2010组件的触摸驱动实现
 */

#include "Touch_SPD2010_Official.h"

static const char *TAG = "TOUCH_SPD2010_OFFICIAL";

// 全局触摸句柄
esp_lcd_touch_handle_t touch_handle = NULL;
esp_lcd_panel_io_handle_t touch_io_handle = NULL;



/**
 * @brief 触摸中断回调函数
 * @param tp 触摸句柄
 */
static void touch_interrupt_callback(esp_lcd_touch_handle_t tp)
{
    // 在中断中可以设置信号量或标志位
    // 这里暂时留空，具体实现根据需要
    ESP_LOGD(TAG, "触摸中断触发");
}

/**
 * @brief 初始化SPD2010触摸控制器（官方组件版本）
 * @return esp_err_t 初始化结果
 */
esp_err_t Touch_Init_Official(void)
{
    ESP_LOGI(TAG, "开始初始化SPD2010触摸控制器（官方组件版本）");
    
    ESP_LOGI(TAG, "创建触摸面板IO");
    
    // 使用官方配置宏创建I2C IO配置
    const esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_SPD2010_CONFIG();
    
    // 创建触摸面板IO句柄
    esp_err_t ret = esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)TOUCH_I2C_PORT, &tp_io_config, &touch_io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "创建触摸面板IO失败: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "触摸面板IO创建成功");
    
    ESP_LOGI(TAG, "配置触摸控制器参数");
    
    // 配置触摸控制器参数
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = TOUCH_MAX_X,
        .y_max = TOUCH_MAX_Y,
        .rst_gpio_num = TOUCH_RST_PIN,
        .int_gpio_num = TOUCH_INT_PIN,
        .levels = {
            .reset = 0,        // 复位低电平有效
            .interrupt = 0,    // 中断低电平有效
        },
        .flags = {
            .swap_xy = TOUCH_SWAP_XY,
            .mirror_x = TOUCH_MIRROR_X,
            .mirror_y = TOUCH_MIRROR_Y,
        },
        .interrupt_callback = touch_interrupt_callback,
    };
    
    ESP_LOGI(TAG, "创建SPD2010触摸控制器");
    
    // 创建SPD2010触摸控制器
    ret = esp_lcd_touch_new_i2c_spd2010(touch_io_handle, &tp_cfg, &touch_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "创建SPD2010触摸控制器失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "SPD2010触摸控制器初始化成功");
    return ESP_OK;
}

/**
 * @brief 反初始化触摸驱动
 */
void Touch_Deinit_Official(void)
{
    ESP_LOGI(TAG, "反初始化SPD2010触摸驱动");
    
    if (touch_handle) {
        esp_lcd_touch_del(touch_handle);
        touch_handle = NULL;
    }
    
    if (touch_io_handle) {
        esp_lcd_panel_io_del(touch_io_handle);
        touch_io_handle = NULL;
    }
    
    // 注意：不要卸载I2C驱动，因为可能有其他设备在使用
    ESP_LOGI(TAG, "SPD2010触摸驱动反初始化完成");
}

/**
 * @brief 读取触摸数据（官方组件版本）
 * @param touch_x X坐标数组
 * @param touch_y Y坐标数组
 * @param strength 触摸强度数组（可为NULL）
 * @param touch_count 触摸点数量指针
 * @param max_points 最大触摸点数
 * @return bool 是否有触摸点
 */
bool Touch_Get_xy_Official(uint16_t *touch_x, uint16_t *touch_y, uint16_t *strength, 
                          uint8_t *touch_count, uint8_t max_points)
{
    if (!touch_handle || !touch_x || !touch_y || !touch_count) {
        return false;
    }
    
    // 读取触摸数据
    esp_err_t ret = esp_lcd_touch_read_data(touch_handle);
    if (ret != ESP_OK) {
        ESP_LOGD(TAG, "读取触摸数据失败: %s", esp_err_to_name(ret));
        return false;
    }
    
    // 获取触摸坐标
    bool touch_pressed = esp_lcd_touch_get_coordinates(touch_handle, touch_x, touch_y, 
                                                      strength, touch_count, max_points);
    
    if (touch_pressed && *touch_count > 0) {
        ESP_LOGD(TAG, "检测到 %d 个触摸点", *touch_count);
        for (int i = 0; i < *touch_count && i < max_points; i++) {
            ESP_LOGD(TAG, "触摸点 %d: (%d, %d)", i, touch_x[i], touch_y[i]);
        }
    }
    
    return touch_pressed;
}

/**
 * @brief 获取触摸句柄
 * @return esp_lcd_touch_handle_t 触摸句柄
 */
esp_lcd_touch_handle_t Touch_Get_Handle_Official(void)
{
    return touch_handle;
}

/**
 * @brief 检查触摸控制器是否已初始化
 * @return bool 是否已初始化
 */
bool Touch_Is_Initialized_Official(void)
{
    return (touch_handle != NULL);
}
