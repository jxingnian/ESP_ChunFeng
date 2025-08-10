/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-09 18:34:37
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-10 19:27:03
 * @FilePath: \esp-brookesia-chunfeng\main\main.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-02 15:54:07
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-10 15:10:48
 * @FilePath: \esp-brookesia-chunfeng\main\main.cpp
 * @Description: 
 * 
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved. 
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "wifi_manager.h"
extern "C" void app_main()
{    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    wifi_init_softap();
    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
