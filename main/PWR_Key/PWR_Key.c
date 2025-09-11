#include "PWR_Key.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "PWR_KEY";

// 电池状态变量：0-未初始化，1-按键被按下，2-按键释放后设备运行
static uint8_t BAT_State = 0; 
// 设备状态变量：0-正常运行，1-休眠，2-重启
static uint8_t Device_State = 0; 
// 长按计数器，用于计算按键按下的时间
static uint16_t Long_Press = 0;
// PWR任务句柄
static TaskHandle_t pwr_task_handle = NULL;

/**
 * @brief PWR任务函数
 * @param pvParameters 任务参数（未使用）
 * @note 这是PWR模块的主任务，负责监控电源按键状态
 */
static void pwr_task(void *pvParameters)
{
    ESP_LOGI(TAG, "PWR task started");
    
    while (1) {
        PWR_Loop();
        // 每100ms检查一次按键状态
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief 电源管理主循环函数
 * @note 该函数处理电源按键的状态变化逻辑
 */
void PWR_Loop(void)
{
  // 检查电池状态是否已初始化
  if(BAT_State){ 
    // 检查电源按键是否被按下（低电平有效）
    if(!gpio_get_level(PWR_KEY_Input_PIN)){   
      // 只有在设备正常运行状态下才处理长按逻辑
      if(BAT_State == 2){         
        // 长按计数器递增
        Long_Press ++;
        // 检查是否达到休眠时间阈值
        if(Long_Press >= Device_Sleep_Time){
          // 根据长按时间设置不同的设备状态
          if(Long_Press >= Device_Sleep_Time && Long_Press < Device_Restart_Time)
            Device_State = 1;  // 设置为休眠状态
          else if(Long_Press >= Device_Restart_Time && Long_Press < Device_Shutdown_Time)
            Device_State = 2;  // 设置为重启状态
          else if(Long_Press >= Device_Shutdown_Time)
            Shutdown();        // 直接执行关机操作
        }
      }
    }
    else{
      // 按键被释放
      if(BAT_State == 1)   
        BAT_State = 2;  // 从初始按下状态切换到正常运行状态
      Long_Press = 0;   // 重置长按计数器
    }
  }
}

/**
 * @brief 设备进入休眠模式
 * @note 此函数用于处理设备进入低功耗休眠状态的逻辑
 */
void Fall_Asleep(void)
{
  // TODO: 实现休眠逻辑
}

/**
 * @brief 重启设备
 * @note 此函数用于执行设备重启操作
 */
void Restart(void)                              
{
  // TODO: 实现重启逻辑
}

/**
 * @brief 关闭设备电源
 * @note 此函数执行设备的完全关机操作
 */
void Shutdown(void)
{
  // 关闭电源控制引脚，切断主电源
  gpio_set_level(PWR_Control_PIN, false);
  // 关闭LCD背光
  LCD_Backlight = 0;        
}

/**
 * @brief 配置GPIO引脚
 * @param pin GPIO引脚编号
 * @param Mode GPIO工作模式（输入/输出）
 */
void configure_GPIO(int pin, gpio_mode_t Mode)
{
    // 重置GPIO引脚到默认状态
    gpio_reset_pin(pin);                                     
    // 设置GPIO引脚的工作方向
    gpio_set_direction(pin, Mode);                          
    
}

/**
 * @brief 电源管理模块初始化
 * @note 初始化电源按键和控制引脚，检测开机按键状态，并启动PWR监控任务
 */
void PWR_Init(void) {
  ESP_LOGI(TAG, "Initializing PWR module");
  
  // 配置电源按键输入引脚为输入模式
  configure_GPIO(PWR_KEY_Input_PIN, GPIO_MODE_INPUT);    
  // 配置电源控制引脚为输出模式
  configure_GPIO(PWR_Control_PIN, GPIO_MODE_OUTPUT);
  // 初始状态关闭电源控制
  gpio_set_level(PWR_Control_PIN, false);
  // 延时100ms，等待硬件稳定
  vTaskDelay(pdMS_TO_TICKS(100));
  // 检查电源按键是否被按下（开机按键检测）
  if(!gpio_get_level(PWR_KEY_Input_PIN)) {   
    BAT_State = 1;                           // 设置电池状态为按键按下状态
    gpio_set_level(PWR_Control_PIN, true);   // 开启主电源
    ESP_LOGI(TAG, "Power key pressed, device powered on");
  }
  
  // 创建PWR监控任务
  BaseType_t ret = xTaskCreate(
      pwr_task,           // 任务函数
      "pwr_task",         // 任务名称
      2048,               // 栈大小
      NULL,               // 任务参数
      3,                  // 任务优先级
      &pwr_task_handle    // 任务句柄
  );
  
  if (ret == pdPASS) {
      ESP_LOGI(TAG, "PWR task created successfully");
  } else {
      ESP_LOGE(TAG, "Failed to create PWR task");
  }
}
