/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-30 16:00:00
 * @Description: 测试官方SPD2010组件的驱动实现
 */

#include "Display_SPD2010_Official.h"
#include "LVGL_Driver.h"
#include "esp_log.h"

static const char *TAG = "TEST_OFFICIAL";

/**
 * @brief 测试官方SPD2010驱动的初始化
 */
void test_official_spd2010_init(void)
{
    ESP_LOGI(TAG, "开始测试官方SPD2010驱动");
    
    // 初始化LCD（使用官方组件）
    LCD_Init_Official();
    
    // 初始化LVGL驱动
    esp_err_t ret = lvgl_driver_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LVGL驱动初始化失败: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "官方SPD2010驱动测试完成");
}

/**
 * @brief 测试背光控制
 */
void test_backlight_control(void)
{
    ESP_LOGI(TAG, "测试背光控制");
    
    // 测试不同亮度级别
    uint8_t brightness_levels[] = {0, 25, 50, 75, 100};
    
    for (int i = 0; i < sizeof(brightness_levels); i++) {
        ESP_LOGI(TAG, "设置背光亮度为: %d%%", brightness_levels[i]);
        Set_Backlight_Official(brightness_levels[i]);
        vTaskDelay(pdMS_TO_TICKS(1000));  // 延时1秒观察效果
    }
    
    // 恢复到默认亮度
    Set_Backlight_Official(70);
    ESP_LOGI(TAG, "背光控制测试完成");
}
