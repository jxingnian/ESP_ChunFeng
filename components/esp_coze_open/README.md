# ESP扣子聊天组件 (ESP Coze Chat Component)

基于扣子API的双向流式语音对话组件，适用于ESP32平台的ESP-IDF 5.5框架。

## 功能特性

- ✅ 双向流式语音对话
- ✅ WebSocket连接管理
- ✅ 自动重连机制
- ✅ 音频流处理
- ✅ 事件驱动架构
- ✅ 会话管理
- ✅ 统计信息收集
- ✅ 线程安全设计
- ✅ 低耦合架构

## API接口

### 初始化和销毁

```c
// 初始化扣子聊天客户端
esp_err_t esp_coze_chat_init(const esp_coze_chat_config_t *config, esp_coze_chat_handle_t *handle);

// 启动连接
esp_err_t esp_coze_chat_start(esp_coze_chat_handle_t handle);

// 停止连接
esp_err_t esp_coze_chat_stop(esp_coze_chat_handle_t handle);

// 销毁客户端
esp_err_t esp_coze_chat_destroy(esp_coze_chat_handle_t handle);
```

### 消息发送

```c
// 发送音频数据
esp_err_t esp_coze_chat_send_audio(esp_coze_chat_handle_t handle, const uint8_t *audio_data, size_t data_len);

// 发送文本消息
esp_err_t esp_coze_chat_send_text(esp_coze_chat_handle_t handle, const char *message);
```

### 语音控制

```c
// 开始语音输入
esp_err_t esp_coze_chat_start_speech_input(esp_coze_chat_handle_t handle);

// 停止语音输入
esp_err_t esp_coze_chat_stop_speech_input(esp_coze_chat_handle_t handle);

// 中断当前对话
esp_err_t esp_coze_chat_interrupt_conversation(esp_coze_chat_handle_t handle);
```

### 配置和状态管理

```c
// 更新会话配置
esp_err_t esp_coze_chat_update_config(esp_coze_chat_handle_t handle, const esp_coze_chat_config_t *config);

// 获取会话信息
esp_err_t esp_coze_chat_get_session_info(esp_coze_chat_handle_t handle, esp_coze_session_info_t *session_info);

// 获取连接状态
bool esp_coze_chat_is_connected(esp_coze_chat_handle_t handle);

// 获取统计信息
esp_err_t esp_coze_chat_get_stats(esp_coze_chat_handle_t handle, esp_coze_stats_t *stats);

// 重置统计信息
esp_err_t esp_coze_chat_reset_stats(esp_coze_chat_handle_t handle);
```

### 工具函数

```c
// 设置日志级别
esp_err_t esp_coze_chat_set_log_level(esp_log_level_t level);

// 获取版本信息
const char* esp_coze_chat_get_version(void);
```

## 使用示例

### 基本使用

```c
#include "esp_coze_chat.h"

static esp_coze_chat_handle_t coze_handle = NULL;

// 事件回调函数
static void coze_event_callback(esp_coze_chat_handle_t handle, esp_coze_event_data_t *event_data, void *user_data)
{
    switch (event_data->event_type) {
        case ESP_COZE_EVENT_CONNECTED:
            ESP_LOGI("APP", "连接成功");
            break;
            
        case ESP_COZE_EVENT_MESSAGE_RECEIVED:
            ESP_LOGI("APP", "收到消息: %s", event_data->message);
            break;
            
        case ESP_COZE_EVENT_ASR_RESULT:
            ESP_LOGI("APP", "语音识别: %s", event_data->message);
            break;
            
        case ESP_COZE_EVENT_TTS_AUDIO:
            ESP_LOGI("APP", "收到TTS音频，长度: %d", event_data->data_len);
            // 播放音频数据
            break;
            
        default:
            break;
    }
}

void app_main(void)
{
    // 配置参数
    esp_coze_chat_config_t config = {
        .ws_base_url = "wss://ws.coze.cn/v1/chat",
        .access_token = "YOUR_ACCESS_TOKEN",
        .bot_id = "YOUR_BOT_ID",
        .device_id = "esp32_device_001",
        .audio_config = {
            .format = ESP_COZE_AUDIO_FORMAT_PCM,
            .sample_rate = ESP_COZE_SAMPLE_RATE_16000,
            .channels = 1,
            .bits_per_sample = 16,
        },
        .event_callback = coze_event_callback,
        .auto_reconnect = true,
        .max_reconnect_attempts = 5,
    };
    
    // 初始化
    ESP_ERROR_CHECK(esp_coze_chat_init(&config, &coze_handle));
    
    // 启动连接
    ESP_ERROR_CHECK(esp_coze_chat_start(coze_handle));
    
    // 发送文本消息
    esp_coze_chat_send_text(coze_handle, "你好，扣子！");
    
    // 开始语音输入
    esp_coze_chat_start_speech_input(coze_handle);
    
    // 发送音频数据（在实际应用中，这通常在音频采集回调中进行）
    uint8_t audio_buffer[1024];
    // ... 填充音频数据 ...
    esp_coze_chat_send_audio(coze_handle, audio_buffer, sizeof(audio_buffer));
    
    // 停止语音输入
    esp_coze_chat_stop_speech_input(coze_handle);
}
```

### 应用层封装

组件还提供了应用层封装，使用更加简便：

```c
#include "coze_chat_app.h"

void app_main(void)
{
    // 初始化（使用默认配置）
    ESP_ERROR_CHECK(coze_chat_app_init());
    
    // 等待连接
    while (!coze_chat_app_is_connected()) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // 发送消息
    coze_chat_app_send_text("你好，我是ESP32设备！");
    
    // 开始语音对话
    coze_chat_app_start_speech();
    
    // ... 应用逻辑 ...
    
    // 停止语音
    coze_chat_app_stop_speech();
    
    // 清理资源
    coze_chat_app_deinit();
}
```

## 配置说明

### 必需配置

- `access_token`: 扣子平台的访问令牌
- `bot_id`: 智能体ID

### 可选配置

- `ws_base_url`: WebSocket基础URL（默认：wss://ws.coze.cn/v1/chat）
- `workflow_id`: 工作流ID（对话流模式需要）
- `device_id`: 设备唯一标识符
- `conversation_id`: 会话ID
- `audio_config`: 音频配置
- `event_callback`: 事件回调函数
- `connect_timeout_ms`: 连接超时时间（默认：10秒）
- `keepalive_idle_timeout_ms`: 保活超时时间（默认：30秒）
- `auto_reconnect`: 是否自动重连（默认：false）
- `max_reconnect_attempts`: 最大重连次数（默认：5）

## 事件类型

- `ESP_COZE_EVENT_CONNECTED`: WebSocket连接成功
- `ESP_COZE_EVENT_DISCONNECTED`: WebSocket连接断开
- `ESP_COZE_EVENT_ERROR`: 发生错误
- `ESP_COZE_EVENT_CONVERSATION_START`: 对话开始
- `ESP_COZE_EVENT_CONVERSATION_END`: 对话结束
- `ESP_COZE_EVENT_AUDIO_START`: 音频开始
- `ESP_COZE_EVENT_AUDIO_END`: 音频结束
- `ESP_COZE_EVENT_MESSAGE_RECEIVED`: 收到消息
- `ESP_COZE_EVENT_ASR_RESULT`: ASR识别结果
- `ESP_COZE_EVENT_TTS_AUDIO`: TTS音频数据
- `ESP_COZE_EVENT_SPEECH_START`: 语音开始
- `ESP_COZE_EVENT_SPEECH_END`: 语音结束

## 依赖组件

- `esp_websocket_client`: WebSocket客户端
- `esp_http_client`: HTTP客户端
- `json`: JSON解析
- `mbedtls`: 加密库

## 注意事项

1. 使用前需要在扣子平台获取访问令牌和智能体ID
2. 确保设备已连接到互联网
3. 音频数据格式需要与配置一致
4. 事件回调函数中不要执行耗时操作
5. 组件会自动管理内存，无需手动释放事件数据

## 版本信息

当前版本：1.0.0

## 许可证

Apache License 2.0
