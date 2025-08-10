#pragma once

/*********************
 *      头文件包含
 *********************/
// LVGL音乐演示主界面头文件
#include <demos/music/lv_demo_music.h>
// LVGL官方演示合集头文件
#include <demos/lv_demos.h>
// LVGL主头文件
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// SD卡驱动头文件（用于音乐文件存储与读取）
#include "SD_MMC.h"
// PCM5101音频解码芯片驱动头文件
#include "PCM5101.h"

/**********************
 *   全局变量声明
 **********************/

// 当前可用音乐曲目数量（全局变量，外部定义）
extern uint16_t ACTIVE_TRACK_CNT;   

/**********************
 *   全局函数声明
 **********************/

/**
 * @brief 图片缩放动画回调适配器
 * 
 * 用于动画系统，设置图片对象的缩放比例
 * @param obj   LVGL对象指针（通常为图片对象）
 * @param zoom  缩放比例（整数值）
 */
void _img_set_zoom_anim_cb(void * obj, int32_t zoom);

/**
 * @brief 对象X坐标动画回调适配器
 * 
 * 用于动画系统，设置对象的X坐标
 * @param obj   LVGL对象指针
 * @param x     新的X坐标
 */
void _obj_set_x_anim_cb(void * obj, int32_t x);

/**
 * @brief 创建音乐主界面
 * 
 * @param parent 父对象指针
 * @return lv_obj_t* 创建的主界面对象指针
 */
lv_obj_t * _lv_demo_music_main_create(lv_obj_t * parent);

/**
 * @brief 关闭音乐主界面
 */
void _lv_demo_music_main_close(void);

/**
 * @brief 切换到上一首/下一首专辑
 * 
 * @param next true: 下一首，false: 上一首
 */
void _lv_demo_music_album_next(bool next);

/**
 * @brief 播放指定ID的音乐
 * 
 * @param id 曲目ID
 */
void _lv_demo_music_play(uint32_t id);

/**
 * @brief 恢复播放
 */
void _lv_demo_music_resume(void);

/**
 * @brief 暂停播放
 */
void _lv_demo_music_pause(void);

/**********************
 *   静态函数声明
 **********************/

/**
 * @brief 创建音乐列表容器
 * 
 * @param parent 父对象指针
 * @return lv_obj_t* 列表容器对象指针
 */
lv_obj_t * create_List_box(lv_obj_t * parent);

/**
 * @brief 向列表添加一个曲目按钮
 * 
 * @param parent   父对象指针
 * @param track_id 曲目ID
 * @return lv_obj_t* 新增按钮对象指针
 */
lv_obj_t * add_list_btn(lv_obj_t * parent, uint32_t track_id);

/**
 * @brief 设置列表按钮选中状态
 * 
 * @param track_id 曲目ID
 * @param state    true: 选中，false: 取消选中
 */
void _lv_demo_music_list_btn_check(uint32_t track_id, bool state);

/**
 * @brief 列表按钮点击事件回调
 * 
 * @param e LVGL事件指针
 */
void btn_click_event_cb(lv_event_t * e);

/**
 * @brief 创建主内容容器
 * 
 * @param parent 父对象指针
 * @return lv_obj_t* 内容容器对象指针
 */
lv_obj_t * create_cont(lv_obj_t * parent);

/**
 * @brief 创建波形图片（频谱动画）
 * 
 * @param parent 父对象指针
 */
void create_wave_images(lv_obj_t * parent);

/**
 * @brief 创建标题栏
 * 
 * @param parent 父对象指针
 * @return lv_obj_t* 标题栏对象指针
 */
lv_obj_t * create_title_box(lv_obj_t * parent);

/**
 * @brief 创建图标栏
 * 
 * @param parent 父对象指针
 * @return lv_obj_t* 图标栏对象指针
 */
lv_obj_t * create_icon_box(lv_obj_t * parent);

/**
 * @brief 创建频谱显示对象
 * 
 * @param parent 父对象指针
 * @return lv_obj_t* 频谱对象指针
 */
lv_obj_t * create_spectrum_obj(lv_obj_t * parent);

/**
 * @brief 创建控制栏（播放/暂停/切歌等按钮）
 * 
 * @param parent 父对象指针
 * @return lv_obj_t* 控制栏对象指针
 */
lv_obj_t * create_ctrl_box(lv_obj_t * parent);

/**
 * @brief 创建进度条拖动柄
 * 
 * @param parent 父对象指针
 * @return lv_obj_t* 拖动柄对象指针
 */
lv_obj_t * create_handle(lv_obj_t * parent);

/**
 * @brief 加载指定ID的曲目信息
 * 
 * @param id 曲目ID
 */
void track_load(uint32_t id);

/**
 * @brief 创建专辑图片对象
 * 
 * @param parent 父对象指针
 * @return lv_obj_t* 专辑图片对象指针
 */
lv_obj_t * album_img_create(lv_obj_t * parent);

/**
 * @brief 专辑图片手势事件回调
 * 
 * @param e LVGL事件指针
 */
void album_gesture_event_cb(lv_event_t * e);

/**
 * @brief 播放按钮点击事件回调
 * 
 * @param e LVGL事件指针
 */
void play_event_click_cb(lv_event_t * e);

/**
 * @brief 上一首按钮点击事件回调
 * 
 * @param e LVGL事件指针
 */
void prev_click_event_cb(lv_event_t * e);

/**
 * @brief 下一首按钮点击事件回调
 * 
 * @param e LVGL事件指针
 */
void next_click_event_cb(lv_event_t * e);

/**
 * @brief 音量调节事件回调
 * 
 * @param e LVGL事件指针
 */
void volume_event_cb(lv_event_t * e);

/**
 * @brief 专辑淡入淡出动画回调
 * 
 * @param var 动画变量指针
 * @param v   动画值
 */
void album_fade_anim_cb(void * var, int32_t v);

/**
 * @brief LVGL定时器回调（用于界面刷新等）
 * 
 * @param t LVGL定时器指针
 */
void timer_cb(lv_timer_t * t);

/**
 * @brief 搜索SD卡中的音乐文件
 */
void LVGL_Search_Music(); 

/**
 * @brief 恢复音乐播放
 */
void LVGL_Resume_Music();

/**
 * @brief 暂停音乐播放
 */
void LVGL_Pause_Music();  

/**
 * @brief 播放指定ID的音乐
 * 
 * @param ID 曲目ID
 */
void LVGL_Play_Music(uint32_t ID);  

/**
 * @brief 音量调节
 * 
 * @param Volume 音量值（0~255）
 */
void LVGL_volume_adjustment(uint8_t Volume);

#ifdef __cplusplus
}
#endif
