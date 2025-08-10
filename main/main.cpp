/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-09 18:34:37
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-10 20:31:40
 * @FilePath: \esp-brookesia-chunfeng\main\main.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi_manager.h"
#include "Display_SPD2010.h"
#include "BAT_Driver.h"
#include "PWR_Key.h"
#include "PCF85063.h"

extern float BAT_analogVolts;

extern "C" void app_main()
{    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    wifi_init_softap();     //WIFI
    I2C_Init();             //I2C
    LCD_Init();             //LCD
    Set_Backlight(100);     //LCD背光
    LVGL_Init();            //LVGL
    BAT_Init();             //电池电压
    PWR_Init();             //电源管理
    PCF85063_Init();        //日期时间
    // datetime_t time;

    
    while (1)
    {
        /* 电源管理 */
        // PWR_Loop();      
        /* 日期时间 */
        // PCF85063_Read_Time(&time);
        // printf("time: %d-%d-%d %d:%d:%d\r\n", time.year, time.month, time.day, time.hour, time.minute, time.second);
        /* 电池电压 */
        // printf("BAT_analogVolts: %.2f V\r\n", BAT_analogVolts);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
