/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-28 15:16:27
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-28 15:32:51
 * @FilePath: \esp-chunfeng\main\LCD_Driver\Display_SPD2010.c
 * @Description:
 *
 */
#include "LVGL_Driver.h"

// LVGL 日志标签
static const char *TAG_LVGL = "LVGL";

// LVGL 显示缓冲区结构体，包含内部图形缓冲区
lv_disp_draw_buf_t disp_buf;
// LVGL 显示驱动结构体，包含回调函数等
lv_disp_drv_t disp_drv;
// LVGL 输入设备驱动结构体
lv_indev_drv_t indev_drv;

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
 * @param disp_drv 显示驱动指针
 * @param area 刷新区域指针
 */
void Lvgl_port_rounder_callback(struct _lv_disp_drv_t *disp_drv, lv_area_t *area)
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
 * @param drv LVGL 显示驱动指针
 * @param area 刷新区域
 * @param color_map 要刷新的像素数据
 */
void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    // 获取 LCD 面板句柄
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // 将缓冲区内容拷贝到显示屏指定区域
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
    // 通知 LVGL 刷新完成
    lv_disp_flush_ready(drv);
}

/**
 * @brief LVGL 触摸读取回调函数，读取触摸点坐标
 * @param drv 输入设备驱动指针
 * @param data 触摸数据结构体指针
 */
void example_touchpad_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
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
 * @param drv LVGL 显示驱动指针
 */
void example_lvgl_port_update_callback(lv_disp_drv_t *drv)
{
    // 获取 LCD 面板句柄
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;

    // 根据旋转角度设置屏幕的XY交换和镜像
    switch (drv->rotated) {
    case LV_DISP_ROT_NONE:
        // 不旋转
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, true, false);
        break;
    case LV_DISP_ROT_90:
        // 顺时针旋转90度
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, true, true);
        break;
    case LV_DISP_ROT_180:
        // 旋转180度
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, false, true);
        break;
    case LV_DISP_ROT_270:
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

    // 分配两个显示缓冲区，使用 PSRAM
    lv_color_t *buf1 = heap_caps_malloc(LVGL_BUF_LEN * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(buf1);
    lv_color_t *buf2 = heap_caps_malloc(LVGL_BUF_LEN * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(buf2);

    // 初始化 LVGL 绘图缓冲区
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LVGL_BUF_LEN);

    ESP_LOGI(TAG_LVGL, "Register display driver to LVGL");
    // 初始化显示驱动结构体
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_WIDTH;   // 设置屏幕水平分辨率
    disp_drv.ver_res = EXAMPLE_LCD_HEIGHT;  // 设置屏幕垂直分辨率
    // disp_drv.rotated = LV_DISP_ROT_90;   // 可选：设置初始旋转
    disp_drv.flush_cb = example_lvgl_flush_cb; // 刷新回调
    disp_drv.drv_update_cb = example_lvgl_port_update_callback; // 驱动参数更新回调
    disp_drv.rounder_cb = Lvgl_port_rounder_callback; // 区域对齐回调
    disp_drv.draw_buf = &disp_buf; // 设置绘图缓冲区
    disp_drv.user_data = panel_handle; // 用户数据，存储 LCD 面板句柄

    ESP_LOGI(TAG_LVGL, "Register display indev to LVGL");
    // 注册显示驱动到 LVGL，返回显示对象指针
    disp = lv_disp_drv_register(&disp_drv);

    // 初始化输入设备驱动结构体
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER; // 输入类型为指针（触摸）
    indev_drv.disp = disp;                  // 关联显示对象
    indev_drv.read_cb = example_touchpad_read; // 触摸读取回调
    lv_indev_drv_register(&indev_drv);         // 注册输入设备驱动

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
