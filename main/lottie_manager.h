/*
 * LVGL Lottie动画管理器
 * 支持从Flash加载JSON到PSRAM，实现多动画切换
 */

#ifndef LOTTIE_MANAGER_H
#define LOTTIE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>

// 动画ID定义
typedef enum {
    LOTTIE_ANIM_APPROVE = 0,    // 成功/批准动画
    LOTTIE_ANIM_LOADING,        // 加载动画
    LOTTIE_ANIM_ERROR,          // 错误动画
    LOTTIE_ANIM_COUNT           // 动画总数
} lottie_anim_id_t;

// 动画状态
typedef enum {
    LOTTIE_STATE_IDLE = 0,      // 空闲
    LOTTIE_STATE_LOADING,       // 加载中
    LOTTIE_STATE_PLAYING,       // 播放中
    LOTTIE_STATE_PAUSED,        // 暂停
    LOTTIE_STATE_ERROR          // 错误
} lottie_state_t;

// 动画信息结构体
typedef struct {
    const char* name;           // 动画名称
    const char* flash_data;     // Flash中的JSON数据
    size_t data_size;           // 数据大小
    char* psram_buffer;         // PSRAM中的缓冲区
    bool loaded;                // 是否已加载到PSRAM
} lottie_anim_info_t;

/**
 * 初始化Lottie管理器
 * @return true 成功，false 失败
 */
bool lottie_manager_init(void);

/**
 * 创建Lottie动画控件
 * @param parent 父对象
 * @param width 动画宽度
 * @param height 动画高度
 * @return 创建的Lottie对象，失败返回NULL
 */
lv_obj_t* lottie_manager_create(lv_obj_t* parent, int32_t width, int32_t height);

/**
 * 播放指定动画
 * @param lottie Lottie对象
 * @param anim_id 动画ID
 * @return true 成功，false 失败
 */
bool lottie_manager_play(lv_obj_t* lottie, lottie_anim_id_t anim_id);

/**
 * 暂停动画
 * @param lottie Lottie对象
 */
void lottie_manager_pause(lv_obj_t* lottie);

/**
 * 恢复动画
 * @param lottie Lottie对象
 */
void lottie_manager_resume(lv_obj_t* lottie);

/**
 * 停止动画
 * @param lottie Lottie对象
 */
void lottie_manager_stop(lv_obj_t* lottie);

/**
 * 获取动画状态
 * @param lottie Lottie对象
 * @return 动画状态
 */
lottie_state_t lottie_manager_get_state(lv_obj_t* lottie);

/**
 * 清理资源
 */
void lottie_manager_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // LOTTIE_MANAGER_H
