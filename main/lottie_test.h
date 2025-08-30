/*
 * LVGL Lottie动画测试程序头文件
 */

#ifndef LOTTIE_TEST_H
#define LOTTIE_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 初始化Lottie测试
 * 创建动画控件并开始播放
 */
void lottie_test_init(void);

/**
 * 清理Lottie测试资源
 */
void lottie_test_deinit(void);

/**
 * 播放批准动画
 */
void lottie_test_play_approve(void);

/**
 * 播放加载动画
 */
void lottie_test_play_loading(void);

#ifdef __cplusplus
}
#endif

#endif // LOTTIE_TEST_H
