/*
 * @Author: xingnian j_xingnian@163.com
 * @Date: 2025-08-21 17:22:36
 * @LastEditors: xingnian j_xingnian@163.com
 * @LastEditTime: 2025-08-27 15:37:05
 * @FilePath: \esp-brookesia-chunfeng\main\coze_chat\coze_chat_app.h
 * @Description: Coze聊天应用程序头文件
 *
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  初始化Coze聊天应用程序
 *
 * @return
 *       - ESP_OK  成功
 *       - Other   失败时返回相应的esp_err_t错误代码
 */
esp_err_t coze_chat_app_init(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
