/*
 * LVGL Lottie动画测试程序
 * 演示如何使用Lottie管理器播放动画
 */

#include "lottie_test.h"
#include "lottie_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "LOTTIE_TEST";

static lv_obj_t* current_lottie = NULL;
static lv_obj_t* info_label = NULL;

// 动画切换任务
static void animation_switch_task(void* pvParameters)
{
    lottie_anim_id_t current_anim = LOTTIE_ANIM_APPROVE;
    
    while (1) {
        if (current_lottie) {
            // 线程安全：在LVGL锁保护下执行UI操作
            if (lv_lock_isr() == LV_RESULT_OK) {
                // 切换动画
                switch (current_anim) {
                    case LOTTIE_ANIM_APPROVE:
                        ESP_LOGI(TAG, "Switching to Loading animation");
                        lottie_manager_play(current_lottie, LOTTIE_ANIM_LOADING);
                        if (info_label) {
                            lv_label_set_text(info_label, "Loading Animation");
                        }
                        current_anim = LOTTIE_ANIM_LOADING;
                        break;
                        
                    case LOTTIE_ANIM_LOADING:
                        ESP_LOGI(TAG, "Switching to Approve animation");
                        lottie_manager_play(current_lottie, LOTTIE_ANIM_APPROVE);
                        if (info_label) {
                            lv_label_set_text(info_label, "Approve Animation");
                        }
                        current_anim = LOTTIE_ANIM_APPROVE;
                        break;
                        
                    default:
                        current_anim = LOTTIE_ANIM_APPROVE;
                        break;
                }
                lv_unlock();
            } else {
                ESP_LOGW(TAG, "Failed to acquire LVGL lock, skipping animation switch");
            }
        }
        
        // 每5秒切换一次动画
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void lottie_test_init(void)
{
    ESP_LOGI(TAG, "Initializing Lottie test...");
    
    // 初始化Lottie管理器
    if (!lottie_manager_init()) {
        ESP_LOGE(TAG, "Failed to initialize Lottie manager");
        return;
    }
    
    lv_lock();
    // 创建信息标签
    info_label = lv_label_create(lv_screen_active());
    lv_label_set_text(info_label, "Lottie Test Starting...");
    lv_obj_align(info_label, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_color(info_label, lv_color_white(), 0);
    
    // 创建Lottie动画控件 (128x128像素)
    current_lottie = lottie_manager_create(lv_screen_active(), 128, 128);
    if (!current_lottie) {
        ESP_LOGE(TAG, "Failed to create Lottie object");
        lv_label_set_text(info_label, "Failed to create Lottie");
        return;
    }
    // 开始播放第一个动画
    if (lottie_manager_play(current_lottie, LOTTIE_ANIM_LOADING)) {
        lv_label_set_text(info_label, "Approve Animation");
        ESP_LOGI(TAG, "Started first animation");
    } else {
        ESP_LOGE(TAG, "Failed to start first animation");
        lv_label_set_text(info_label, "Failed to start animation");
        return;
    }
    lv_unlock();
    
    // 创建动画切换任务 - 增大栈空间并降低优先级
    // xTaskCreate(animation_switch_task, "anim_switch", 8192, NULL, 3, NULL);
    
    ESP_LOGI(TAG, "Lottie test initialized successfully");
}

void lottie_test_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing Lottie test...");
    
    if (current_lottie) {
        lottie_manager_stop(current_lottie);
        lv_obj_delete(current_lottie);
        current_lottie = NULL;
    }
    
    if (info_label) {
        lv_obj_delete(info_label);
        info_label = NULL;
    }
    
    lottie_manager_deinit();
    
    ESP_LOGI(TAG, "Lottie test deinitialized");
}

void lottie_test_play_approve(void)
{
    if (current_lottie) {
        lottie_manager_play(current_lottie, LOTTIE_ANIM_APPROVE);
        if (info_label) {
            lv_label_set_text(info_label, "Approve Animation");
        }
    }
}

void lottie_test_play_loading(void)
{
    if (current_lottie) {
        lottie_manager_play(current_lottie, LOTTIE_ANIM_LOADING);
        if (info_label) {
            lv_label_set_text(info_label, "Loading Animation");
        }
    }
}
