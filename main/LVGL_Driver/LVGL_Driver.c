/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-30 11:30:00
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-30 14:45:21
 * @FilePath: \esp-chunfeng\main\LVGL_Driver\LVGL_Driver.c
 * @Description: LVGL 9.2.2 驱动实现 - 为ESP32S3 + SPD2010显示屏设计
 */

#include "LVGL_Driver.h"

/*********************
 * 静态变量定义
 *********************/

static const char *TAG = "LVGL_DRIVER";

// LVGL 全局对象
lv_display_t *g_lvgl_display = NULL;
lv_indev_t *g_lvgl_indev = NULL;

// ESP定时器句柄
static esp_timer_handle_t lvgl_tick_timer = NULL;

// 显示缓冲区
static uint8_t *lvgl_draw_buf1 = NULL;
static uint8_t *lvgl_draw_buf2 = NULL;

/*********************
 * 静态函数声明
 *********************/

static esp_err_t lvgl_display_init(void);
static esp_err_t lvgl_indev_init(void);
static esp_err_t lvgl_tick_timer_init(void);
static void lvgl_cleanup_resources(void);

/*********************
 * 回调函数实现
 *********************/

void lvgl_tick_inc_cb(void *arg)
{
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    // 获取LCD面板句柄
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
    if (!panel_handle) {
        ESP_LOGE(TAG, "Panel handle is NULL");
        lv_display_flush_ready(disp);
        return;
    }

    // 获取区域坐标
    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t x2 = area->x2;
    int32_t y2 = area->y2;
    
    // LVGL9中已移除rounder回调，需要在flush回调中手动对齐
    // 为SPD2010进行4字节对齐优化（很重要！）
    x1 = (x1 >> 2) << 2;           // 对齐到4的倍数
    x2 = ((x2 >> 2) << 2) + 3;     // 对齐到4N+3

    // 计算像素数量
    int32_t w = x2 - x1 + 1;
    int32_t h = y2 - y1 + 1;
    int32_t pixel_count = w * h;

    // LVGL9中px_map已经是按显示器颜色格式(RGB565)排列的数据
    // 只需进行SPD2010需要的字节序交换
    uint16_t *pixels = (uint16_t *)px_map;
    
    // 为SPD2010进行字节序交换 (RGB565格式大小端转换)
    // 优化：使用更高效的批量转换
    for (int32_t i = 0; i < pixel_count; i++) {
        uint16_t pixel = pixels[i];
        pixels[i] = __builtin_bswap16(pixel);  // 使用内建函数进行字节交换，更高效
    }

    // 发送到LCD
    esp_err_t ret = esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2 + 1, y2 + 1, px_map);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to draw bitmap: %s", esp_err_to_name(ret));
    }

    // 通知LVGL刷新完成
    lv_display_flush_ready(disp);
}



void lvgl_touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    uint16_t touch_x[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint16_t touch_y[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint8_t touch_count = 0;

    // 读取触摸数据
    bool touch_pressed = Touch_Get_xy(touch_x, touch_y, NULL, &touch_count, CONFIG_ESP_LCD_TOUCH_MAX_POINTS);

    if (touch_pressed && touch_count > 0) {
        // 有触摸点，使用第一个触摸点
        data->point.x = touch_x[0];
        data->point.y = touch_y[0];
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        // 无触摸点
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/*********************
 * 静态函数实现
 *********************/

static esp_err_t lvgl_display_init(void)
{
    ESP_LOGI(TAG, "Initializing LVGL display");

    // 创建显示对象
    g_lvgl_display = lv_display_create(EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT);
    if (!g_lvgl_display) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        return ESP_FAIL;
    }

    // 分配显示缓冲区 (使用PSRAM)
    // LVGL9中缓冲区大小以字节为单位，对于RGB565每像素2字节
    size_t buffer_size = LVGL_BUFFER_SIZE * 2;  // RGB565每像素2字节
    
    lvgl_draw_buf1 = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    if (!lvgl_draw_buf1) {
        ESP_LOGE(TAG, "Failed to allocate draw buffer 1");
        return ESP_ERR_NO_MEM;
    }

    lvgl_draw_buf2 = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    if (!lvgl_draw_buf2) {
        ESP_LOGE(TAG, "Failed to allocate draw buffer 2");
        free(lvgl_draw_buf1);
        lvgl_draw_buf1 = NULL;
        return ESP_ERR_NO_MEM;
    }

    // 设置显示缓冲区 - LVGL9中buffer_size参数是字节数
    lv_display_set_buffers(g_lvgl_display, lvgl_draw_buf1, lvgl_draw_buf2, 
                          buffer_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // 设置颜色格式为RGB565（与SPD2010匹配）
    lv_display_set_color_format(g_lvgl_display, LV_COLOR_FORMAT_RGB565);

    // 设置刷新回调
    lv_display_set_flush_cb(g_lvgl_display, lvgl_flush_cb);

    // 设置用户数据 (LCD面板句柄)
    lv_display_set_user_data(g_lvgl_display, panel_handle);

    ESP_LOGI(TAG, "LVGL display initialized successfully");
    return ESP_OK;
}

static esp_err_t lvgl_indev_init(void)
{
    ESP_LOGI(TAG, "Initializing LVGL input device");

    // 创建输入设备
    g_lvgl_indev = lv_indev_create();
    if (!g_lvgl_indev) {
        ESP_LOGE(TAG, "Failed to create LVGL input device");
        return ESP_FAIL;
    }

    // 设置输入设备类型和回调
    lv_indev_set_type(g_lvgl_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(g_lvgl_indev, lvgl_touch_read_cb);

    ESP_LOGI(TAG, "LVGL input device initialized successfully");
    return ESP_OK;
}

static esp_err_t lvgl_tick_timer_init(void)
{
    ESP_LOGI(TAG, "Initializing LVGL tick timer");

    // 创建定时器配置
    const esp_timer_create_args_t timer_args = {
        .callback = lvgl_tick_inc_cb,
        .name = "lvgl_tick"
    };

    // 创建定时器
    esp_err_t ret = esp_timer_create(&timer_args, &lvgl_tick_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create tick timer: %s", esp_err_to_name(ret));
        return ret;
    }

    // 启动定时器
    ret = esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start tick timer: %s", esp_err_to_name(ret));
        esp_timer_delete(lvgl_tick_timer);
        lvgl_tick_timer = NULL;
        return ret;
    }

    ESP_LOGI(TAG, "LVGL tick timer started successfully");
    return ESP_OK;
}

static void lvgl_cleanup_resources(void)
{
    // 停止并删除定时器
    if (lvgl_tick_timer) {
        esp_timer_stop(lvgl_tick_timer);
        esp_timer_delete(lvgl_tick_timer);
        lvgl_tick_timer = NULL;
    }

    // 删除输入设备
    if (g_lvgl_indev) {
        lv_indev_delete(g_lvgl_indev);
        g_lvgl_indev = NULL;
    }

    // 删除显示对象
    if (g_lvgl_display) {
        lv_display_delete(g_lvgl_display);
        g_lvgl_display = NULL;
    }

    // 释放缓冲区
    if (lvgl_draw_buf1) {
        free(lvgl_draw_buf1);
        lvgl_draw_buf1 = NULL;
    }
    if (lvgl_draw_buf2) {
        free(lvgl_draw_buf2);
        lvgl_draw_buf2 = NULL;
    }
}

/*********************
 * 公共函数实现
 *********************/

esp_err_t lvgl_driver_init(void)
{
    ESP_LOGI(TAG, "Initializing LVGL driver (version %d.%d.%d)", 
             lv_version_major(), lv_version_minor(), lv_version_patch());

    // 初始化LVGL库
    lv_init();

    // 初始化显示驱动
    esp_err_t ret = lvgl_display_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display");
        goto error;
    }

    // 初始化输入设备
    ret = lvgl_indev_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize input device");
        goto error;
    }

    // 初始化tick定时器
    ret = lvgl_tick_timer_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize tick timer");
        goto error;
    }

    ESP_LOGI(TAG, "LVGL driver initialized successfully");
    return ESP_OK;

error:
    lvgl_cleanup_resources();
    return ret;
}

void lvgl_driver_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing LVGL driver");
    lvgl_cleanup_resources();
    ESP_LOGI(TAG, "LVGL driver deinitialized");
}
