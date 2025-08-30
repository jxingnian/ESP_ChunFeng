/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-28 15:16:27
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-30 09:32:41
 * @FilePath: \esp-chunfeng\main\LVGL_Driver\LVGL_Driver copy.c
 * @Description:
 *
 */
#include "LVGL_Driver.h"

// LVGL 日志标签
static const char *TAG_LVGL = "LVGL";

// LVGL 9.x 不再需要这些全局结构体变量
// 显示和输入设备对象将在初始化函数中创建

/**
 * @brief LVGL 定时器回调函数，定时增加 LVGL 的 tick 计数
 * @param arg 未使用
 */
void example_increase_lvgl_tick(void *arg)
{
    // 通知 LVGL 已经过了多少毫秒
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

/**
 * @brief LVGL 区域对齐回调函数，用于对齐刷新区域到4字节边界
 * @param disp 显示对象指针
 * @param area 刷新区域指针
 */
void Lvgl_port_rounder_callback(lv_display_t *disp, lv_area_t *area)
{
    uint16_t x1 = area->x1;
    uint16_t x2 = area->x2;

    // 将起始坐标向下对齐到4的倍数
    area->x1 = (x1 >> 2) << 2;

    // 将结束坐标向上对齐到4N+3
    area->x2 = ((x2 >> 2) << 2) + 3;
}

/**
 * @brief LVGL 刷新回调函数，将缓冲区内容刷新到屏幕指定区域
 * @param disp LVGL 显示对象指针
 * @param area 刷新区域
 * @param px_map 要刷新的像素数据（字节数组）
 */
void example_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    // 获取 LCD 面板句柄
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) lv_display_get_user_data(disp);
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    
    // 在LVGL 9.x中，px_map是uint8_t*类型，但esp_lcd_panel_draw_bitmap可能期望特定的数据格式
    // 如果颜色显示不正确，可能需要进行字节序转换
    
    // 启用字节序交换来解决颜色显示问题（蓝色显示为黄色）
    // 计算像素数量
    int32_t w = (offsetx2 - offsetx1 + 1);
    int32_t h = (offsety2 - offsety1 + 1);
    int32_t pixel_count = w * h;
    
    // 对RGB565格式进行字节序交换
    uint16_t *px_16 = (uint16_t *)px_map;
    for(int32_t i = 0; i < pixel_count; i++) {
        // 交换字节序: 高字节<->低字节
        uint16_t pixel = px_16[i];
        px_16[i] = ((pixel & 0xFF) << 8) | ((pixel & 0xFF00) >> 8);
    }
    
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
    
    // 通知 LVGL 刷新完成
    lv_display_flush_ready(disp);
}

/**
 * @brief LVGL 触摸读取回调函数，读取触摸点坐标
 * @param indev 输入设备指针
 * @param data 触摸数据结构体指针
 */
void example_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    uint16_t touchpad_x[5] = {0}; // 存储最多5个触摸点的X坐标
    uint16_t touchpad_y[5] = {0}; // 存储最多5个触摸点的Y坐标
    uint8_t touchpad_cnt = 0;     // 触摸点数量

    // 获取触摸点坐标
    bool touchpad_pressed = Touch_Get_xy(touchpad_x, touchpad_y, NULL, &touchpad_cnt, CONFIG_ESP_LCD_TOUCH_MAX_POINTS);

    // 如果有触摸点，填充数据
    if (touchpad_pressed && touchpad_cnt > 0) {
        data->point.x = touchpad_x[0];
        data->point.y = touchpad_y[0];
        data->state = LV_INDEV_STATE_PR; // 按下状态
    } else {
        data->state = LV_INDEV_STATE_REL; // 松开状态
    }
}

/**
 * @brief LVGL 显示驱动参数更新回调，屏幕旋转时调用
 * @param disp LVGL 显示对象指针
 */
void example_lvgl_port_update_callback(lv_display_t *disp)
{
    // 获取 LCD 面板句柄
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) lv_display_get_user_data(disp);

    // 根据旋转角度设置屏幕的XY交换和镜像
    switch (lv_display_get_rotation(disp)) {
    case LV_DISPLAY_ROTATION_0:
        // 不旋转
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, true, false);
        break;
    case LV_DISPLAY_ROTATION_90:
        // 顺时针旋转90度
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, true, true);
        break;
    case LV_DISPLAY_ROTATION_180:
        // 旋转180度
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, false, true);
        break;
    case LV_DISPLAY_ROTATION_270:
        // 逆时针旋转90度
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, false, false);
        break;
    }
}

// LVGL 显示对象指针
lv_disp_t *disp;

/**
 * @brief LVGL 初始化函数，初始化 LVGL 库、显示驱动、输入驱动及定时器
 */
void LVGL_Init(void)
{
    ESP_LOGI(TAG_LVGL, "Initialize LVGL library");
    // 初始化 LVGL 库
    lv_init();

    ESP_LOGI(TAG_LVGL, "Create LVGL display");
    // 创建 LVGL 显示对象
    disp = lv_display_create(EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT);

    // 分配两个显示缓冲区，使用 PSRAM，注意LVGL 9.x缓冲区大小以字节为单位
    size_t buffer_size = LVGL_BUF_LEN * sizeof(lv_color_t);
    uint8_t *buf1 = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    assert(buf1);
    uint8_t *buf2 = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    assert(buf2);

    // 设置 LVGL 显示缓冲区（LVGL 9.x新API）
    lv_display_set_buffers(disp, buf1, buf2, buffer_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    ESP_LOGI(TAG_LVGL, "Configure display driver");
    // 设置颜色格式 - 如果颜色显示不正确（如蓝色显示为黄色），
    // 这通常是字节序问题，尝试以下解决方案：
    
    // 使用RGB565格式配合字节序交换来解决颜色问题
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    
    // 如果颜色还是不对，可以尝试其他方案：
    // 方案2: lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    // 方案3: lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB888); // 需要更多内存
    // 方案4: 在刷新回调中启用字节序交换（见下面注释的代码）
    
    // 设置刷新回调
    lv_display_set_flush_cb(disp, example_lvgl_flush_cb);
    
    // 注意：LVGL 9.x 中移除了一些回调函数：
    // - rounder_cb (区域对齐回调) 已被移除
    // - driver_update_cb (驱动更新回调) 已被移除
    // 这些功能现在由LVGL内部处理或通过其他方式实现
    
    // 存储 LCD 面板句柄到用户数据
    lv_display_set_user_data(disp, panel_handle);

    ESP_LOGI(TAG_LVGL, "Create input device");
    // 创建输入设备（LVGL 9.x新API）
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);  // 设置为触摸输入
    lv_indev_set_read_cb(indev, example_touchpad_read); // 设置读取回调

    /********************* LVGL *********************/
    ESP_LOGI(TAG_LVGL, "Install LVGL tick timer");
    // 创建 LVGL tick 定时器，周期为 EXAMPLE_LVGL_TICK_PERIOD_MS 毫秒
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };

    esp_timer_handle_t lvgl_tick_timer = NULL;
    // 创建定时器
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    // 启动定时器，周期性触发
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));
}
