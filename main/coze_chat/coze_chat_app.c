/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * coze_chat_app.c - Coze 聊天应用主程序
 * 
 * 功能概述：
 * 本文件实现了一个基于 ESP-Coze 组件的智能语音对话应用，支持两种工作模式：
 * 1. 按键触发模式 (CONFIG_KEY_PRESS_DIALOG_MODE)：按住按键说话，松开结束
 * 2. 语音唤醒模式 (CONFIG_VOICE_WAKEUP_MODE)：通过唤醒词激活对话
 * 3. 连续对话模式：持续监听和发送音频数据
 * 
 * 主要流程：
 * 音频采集 → 编码处理 → WebSocket发送 → Coze服务器处理 → 接收回复 → 音频播放
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>                  // 标准库：内存分配函数
#include "freertos/FreeRTOS.h"       // FreeRTOS 核心头文件
#include "freertos/task.h"           // 任务管理
#include "freertos/event_groups.h"   // 事件组，用于任务间同步

#include "esp_err.h"                 // ESP 错误码定义
#include "esp_log.h"                 // 日志输出
#include "esp_check.h"               // 错误检查宏
// #ifndef CONFIG_KEY_PRESS_DIALOG_MODE
// #include "esp_gmf_afe.h"            // 音频前端处理（AFE），用于语音唤醒和VAD
// #endif  /* CONFIG_KEY_PRESS_DIALOG_MODE */
// GMF OAL 头文件已移除，使用标准 FreeRTOS API 替代
// #include "esp_gmf_oal_sys.h"        // GMF 操作系统抽象层 - 系统接口
// #include "esp_gmf_oal_thread.h"     // GMF 操作系统抽象层 - 线程管理
// #include "esp_gmf_oal_mem.h"        // GMF 操作系统抽象层 - 内存管理

#include "iot_button.h"             // IoT 按键驱动
#include "button_adc.h"             // ADC 按键驱动
#include "esp_coze_chat.h"          // Coze 聊天组件核心头文件
// #include "audio_processor.h"        // 音频处理器接口
#include "audio_hal.h"
// 事件组位定义：按键录音状态标志
#define BUTTON_REC_READING (1 << 0)  // 当按键按下时设置，表示正在录音

// 日志标签
static char *TAG = "COZE_CHAT_APP";

/**
 * @brief Coze 聊天应用主结构体
 * 
 * 管理整个聊天应用的状态和资源：
 * - chat: Coze 聊天实例句柄
 * - wakeuped: 语音唤醒状态标志
 * - read_thread: 音频数据读取线程
 * - btn_thread: 按键事件处理线程  
 * - data_evt_group: 事件组，用于线程间同步
 * - btn_evt_q: 按键事件队列
 */
struct coze_chat_t {
    esp_coze_chat_handle_t chat;          // Coze 聊天句柄
    // bool                   wakeuped;      // 是否已被语音唤醒
    TaskHandle_t           read_thread;   // 音频数据读取任务句柄
    TaskHandle_t           btn_thread;    // 按键事件处理任务句柄
    EventGroupHandle_t     data_evt_group; // 事件组：控制录音状态
    QueueHandle_t          btn_evt_q;     // 按键事件队列
};

// 全局 Coze 聊天应用实例
static struct coze_chat_t coze_chat;

/**
 * @brief 音频事件回调函数
 * 
 * 处理来自 Coze 服务器的各种事件，包括：
 * - 语音合成开始/停止事件
 * - 自定义数据事件  
 * - 字幕事件
 * 
 * @param event 事件类型
 * @param data 事件关联的数据（通常为JSON格式）
 * @param ctx 用户上下文指针（本例中未使用）
 */
static void audio_event_callback(esp_coze_chat_event_t event, char *data, void *ctx)
{
    if (event == ESP_COZE_CHAT_EVENT_CHAT_SPEECH_STARTED) {
        // 服务器开始语音合成并播放
        ESP_LOGI(TAG, "chat start");
    } else if (event == ESP_COZE_CHAT_EVENT_CHAT_SPEECH_STOPED) {
        // 服务器停止语音合成
        ESP_LOGI(TAG, "chat stop");
    } else if (event == ESP_COZE_CHAT_EVENT_CHAT_CUSTOMER_DATA) {
        // 接收到自定义数据（JSON格式）
        ESP_LOGI(TAG, "Customer data: %s", data);
    } else if (event == ESP_COZE_CHAT_EVENT_CHAT_SUBTITLE_EVENT) {
        // 接收到字幕数据（当 enable_subtitle=true 时）
        ESP_LOGI(TAG, "Subtitle data: %s", data);
    }
}

/**
 * @brief 音频数据回调函数
 * 
 * 接收来自 Coze 服务器的下行音频数据（AI 语音回复），
 * 并将其传递给音频播放模块进行播放。
 * 
 * @param data 音频数据缓冲区指针
 * @param len 音频数据长度（字节）
 * @param ctx 用户上下文指针（本例中未使用）
 */
static void audio_data_callback(char *data, int len, void *ctx)
{
    // 将接收到的音频数据送入播放器
    // audio_playback_feed_data((uint8_t *)data, len);
    // 使用自定义音频HAL直接播放音频数据
    // 数据格式：16位PCM，单声道，16kHz采样率
    size_t sample_count = len / sizeof(int16_t);  // 计算采样数
    esp_err_t ret = audio_hal_write((const int16_t *)data, sample_count, 100);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Audio HAL write failed: %s", esp_err_to_name(ret));
    }
}

/**
 * @brief 初始化 Coze 聊天客户端
 * 
 * 配置并初始化 Coze 聊天实例：
 * 1. 设置基本配置（机器人ID、访问令牌等）
 * 2. 注册回调函数
 * 3. 根据编译配置选择工作模式
 * 4. 启动聊天会话
 * 
 * @return ESP_OK 成功，ESP_FAIL 失败
 */
static esp_err_t init_coze_chat()
{
    // 检查可用内存
    size_t free_heap = esp_get_free_heap_size();
    size_t min_free_heap = esp_get_minimum_free_heap_size();
    
    // 检查 PSRAM 内存状态
    size_t psram_size = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t psram_used = psram_size - psram_free;
    size_t psram_largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    
    ESP_LOGI(TAG, "Memory status before init: free=%zu, min_free=%zu", free_heap, min_free_heap);
    ESP_LOGI(TAG, "PSRAM status before init: total=%zu, used=%zu, free=%zu, largest_block=%zu", 
             psram_size, psram_used, psram_free, psram_largest_block);
    
    // 使用默认配置初始化
    esp_coze_chat_config_t chat_config = ESP_COZE_CHAT_DEFAULT_CONFIG();
    
    // 基本配置
    chat_config.enable_subtitle = true;                    // 启用字幕功能
    chat_config.bot_id = "7507830126416560143";              // 从配置中获取机器人ID
    chat_config.access_token = "pat_A7Tk6tCS6qyAEvQg1hlkKAYjXq0TUUmUFUw65jiQjoU9Q4nEPvtMC9A08d75gBUn";  // 从配置中获取访问令牌
    chat_config.audio_callback = audio_data_callback;     // 设置音频数据回调
    chat_config.event_callback = audio_event_callback;    // 设置事件回调
    
// #ifdef CONFIG_KEY_PRESS_DIALOG_MODE
    // 按键模式配置：优化内存使用
    chat_config.websocket_buffer_size = 4096;              // 保持较小的WebSocket缓冲区
    chat_config.mode = ESP_COZE_CHAT_NORMAL_MODE;
    
    // 优化任务配置以减少内存使用
    chat_config.pull_task_stack_size = 3072;               // 减小拉取任务栈大小
    chat_config.push_task_stack_size = 3072;               // 减小推送任务栈大小
    chat_config.pull_task_caps = MALLOC_CAP_8BIT;          // 使用内部RAM，不依赖SPIRAM
    chat_config.push_task_caps = MALLOC_CAP_8BIT;          // 使用内部RAM，不依赖SPIRAM
// #endif /* CONFIG_KEY_PRESS_DIALOG_MODE */

    esp_err_t ret = ESP_OK;
    
    // 再次检查内存状态
    ESP_LOGI(TAG, "Memory before esp_coze_chat_init: free=%zu", esp_get_free_heap_size());
    
    // 初始化聊天实例
    ret = esp_coze_chat_init(&chat_config, &coze_chat.chat);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_coze_chat_init failed, err: %d", ret);
        ESP_LOGE(TAG, "Memory after failed init: free=%zu, min_free=%zu", 
                esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "esp_coze_chat_init success, memory: free=%zu", esp_get_free_heap_size());
    
    // 启动聊天会话（建立WebSocket连接）
    esp_coze_chat_start(coze_chat.chat);
    return ESP_OK;
}

/**
 * @brief 按键事件回调函数（按键触发模式）
 * 
 * 当按键状态发生变化时被调用，将按键事件发送到队列中
 * 供按键事件处理任务处理。
 * 
 * @param arg 按键句柄
 * @param data 用户数据指针（本例中未使用）
 */
static void button_event_cb(void *arg, void *data)
{
    button_event_t button_event = iot_button_get_event(arg);
    // 将按键事件发送到队列（非阻塞）
    xQueueSend(coze_chat.btn_evt_q, &button_event, 0);
}

/**
 * @brief 按键事件处理任务（按键触发模式）
 * 
 * 持续监听按键事件队列，根据按键状态控制录音：
 * - 按键按下：开始录音，设置录音标志
 * - 按键抬起：停止录音，通知服务器音频传输完成
 * 
 * @param pv 任务参数（本例中未使用）
 */
static void btn_event_task(void *pv)
{
    button_event_t btn_evt;
    while (1) {
        // 等待按键事件（阻塞等待）
        if (xQueueReceive(coze_chat.btn_evt_q, &btn_evt, portMAX_DELAY) == pdTRUE) {
            switch (btn_evt) {
                case BUTTON_PRESS_DOWN:
                    // 按键按下：取消之前的音频传输，开始新的录音
                    esp_coze_chat_send_audio_cancel(coze_chat.chat);
                    xEventGroupSetBits(coze_chat.data_evt_group, BUTTON_REC_READING);
                    break;
                case BUTTON_PRESS_UP:
                    // 按键抬起：停止录音，通知服务器音频传输完成
                    xEventGroupClearBits(coze_chat.data_evt_group, BUTTON_REC_READING);
                    esp_coze_chat_send_audio_complete(coze_chat.chat);
                    break;
                default:
                    break;
            }
        }
    }
}

/**
 * @brief 音频数据读取任务
 * 
 * 核心音频处理任务，根据不同工作模式采集和发送音频数据：
 * 
 * 1. 按键触发模式：等待按键按下事件，然后读取较小的音频块（640字节）
 * 2. 语音唤醒模式：持续读取音频，但只在唤醒状态下发送到服务器
 * 3. 连续对话模式：持续读取和发送音频数据
 * 
 * @param pv 任务参数（本例中未使用）
 */
static void audio_data_read_task(void *pv)
{
// #if defined CONFIG_KEY_PRESS_DIALOG_MODE
    // 按键模式：较小的缓冲区（640字节，约40ms@16kHz）
    uint8_t *data = (uint8_t *)calloc(1, 640);
// #else
//     // 唤醒/连续模式：较大的缓冲区（12KB，约750ms@16kHz）
//     uint8_t *data = (uint8_t *)calloc(1, 4096 * 3);
// #endif
    if (data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate audio buffer");
        vTaskDelete(NULL);
        return;
    }
    
    int ret = 0;
    
    while (true) {
// #if defined CONFIG_KEY_PRESS_DIALOG_MODE
        // 按键模式：等待按键按下信号
        xEventGroupWaitBits(coze_chat.data_evt_group, BUTTON_REC_READING, pdFALSE, pdFALSE, portMAX_DELAY);
        
        // 使用自定义音频HAL读取麦克风数据
        size_t samples_got = 0;
        esp_err_t read_ret = audio_hal_read((int16_t *)data, 640 / sizeof(int16_t), &samples_got, 100);
        if (read_ret == ESP_OK && samples_got > 0) {
            ret = samples_got * sizeof(int16_t);  // 转换为字节数
            // 立即发送音频数据到服务器
            esp_coze_chat_send_audio_data(coze_chat.chat, (char *)data, ret);
        }
    }
    
    // 清理资源（正常情况下不会执行到这里）
    free(data);
    vTaskDelete(NULL);
}

/**
 * @brief 初始化音频管道
 * 
 * 根据配置的工作模式初始化相应的音频组件：
 * 1. 音频HAL：初始化I2S音频硬件
 * 2. 音频管理器：负责整体音频系统管理（如果需要）
 * 3. 音频录制器：根据模式配置不同的回调函数
 * 4. 音频播放器：播放来自服务器的语音回复
 * 5. 提示音播放器：播放系统提示音（仅在非按键模式）
 */
static void audio_pipe_open()
{
    // 初始化音频HAL
    esp_err_t ret = audio_hal_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Audio HAL init failed: %s", esp_err_to_name(ret));
        return;
    }
}

/**
 * @brief Coze 聊天应用初始化函数
 * 
 * 应用程序的主要初始化入口，完成以下初始化步骤：
 * 1. 设置日志级别
 * 2. 初始化按键处理（仅在按键模式）
 * 3. 初始化 Coze 聊天客户端
 * 4. 初始化音频处理管道
 * 5. 创建音频数据读取任务
 * 
 * @return ESP_OK 成功，其他值表示失败
 */
esp_err_t coze_chat_app_init(void)
{
    // 设置全局日志级别为 INFO
    esp_log_level_set("*", ESP_LOG_INFO);
    
    // 初始化唤醒状态为 false（当前使用按键模式，暂时不需要）
    // coze_chat.wakeuped = false;

// #if CONFIG_KEY_PRESS_DIALOG_MODE
    /** 按键模式初始化 - 适用于 ESP32-S3-Korvo2 开发板 */
    button_handle_t btn = NULL;
    const button_config_t btn_cfg = {0};  // 使用默认按键配置
    
    // ADC 按键配置：通过 ADC 检测按键状态
    button_adc_config_t btn_adc_cfg = {
        .unit_id = ADC_UNIT_1,    // 使用 ADC1
        .adc_channel = 4,         // ADC 通道 4
        .button_index = 0,        // 按键索引
        .min = 2310,             // ADC 最小值（按键按下时的 ADC 读数范围）
        .max = 2510              // ADC 最大值
    };
    
    // 创建 ADC 按键设备
    iot_button_new_adc_device(&btn_cfg, &btn_adc_cfg, &btn);
    
    // 注册按键按下事件回调
    ESP_ERROR_CHECK(iot_button_register_cb(btn, BUTTON_PRESS_DOWN, NULL, button_event_cb, NULL));
    // 注册按键抬起事件回调
    ESP_ERROR_CHECK(iot_button_register_cb(btn, BUTTON_PRESS_UP, NULL, button_event_cb, NULL));
    
    // 创建事件组：用于控制录音状态
    coze_chat.data_evt_group = xEventGroupCreate();
    // 创建按键事件队列：容量为2个事件
    coze_chat.btn_evt_q = xQueueCreate(2, sizeof(button_event_t));
    
    // 创建按键事件处理任务
    BaseType_t ret = xTaskCreatePinnedToCore(
        btn_event_task,          // 任务函数
        "btn_event_task",        // 任务名称
        2048,                    // 栈大小（字节）- 减小栈大小
        NULL,                    // 任务参数
        12,                      // 任务优先级
        &coze_chat.btn_thread,   // 任务句柄
        1                        // 运行在核心1上
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button event task");
        return ESP_FAIL;
    }

// #endif  /* CONFIG_KEY_PRESS_DIALOG_MODE */

    // 初始化 Coze 聊天客户端
    init_coze_chat();

    // 初始化音频处理管道
    audio_pipe_open();

    // 创建音频数据读取任务：负责持续读取音频数据并发送到服务器
    ret = xTaskCreatePinnedToCore(
        audio_data_read_task,    // 任务函数
        "audio_data_read_task",  // 任务名称
        2048,                    // 栈大小（字节）- 减小栈大小
        NULL,                    // 任务参数
        12,                      // 任务优先级
        &coze_chat.read_thread,  // 任务句柄
        1                        // 运行在核心1上
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create audio read task");
        return ESP_FAIL;
    }

    return ESP_OK;
}
