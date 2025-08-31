/*
 * LVGL Lottie动画管理器实现
 */

#include "lottie_manager.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>

static const char* TAG = "LOTTIE_MANAGER";

// 将JSON数据存储在Flash中（只读常量）
static const char lottie_approve_json[] = 
    "{\"v\":\"4.8.0\",\"meta\":{\"g\":\"LottieFiles AE 1.0.0\",\"a\":\"\",\"k\":\"\",\"d\":\"\",\"tc\":\"\"},"
    "\"fr\":60,\"ip\":0,\"op\":60,\"w\":720,\"h\":720,\"nm\":\"Success\",\"ddd\":0,\"assets\":[],"
    "\"layers\":[{\"ddd\":0,\"ind\":1,\"ty\":4,\"nm\":\"Shape Layer 4\",\"sr\":1,"
    "\"ks\":{\"o\":{\"a\":0,\"k\":100,\"ix\":11},\"r\":{\"a\":0,\"k\":0,\"ix\":10},"
    "\"p\":{\"a\":0,\"k\":[336,396,0],\"ix\":2},\"a\":{\"a\":0,\"k\":[0,0,0],\"ix\":1},"
    "\"s\":{\"a\":0,\"k\":[100,100,100],\"ix\":6}},\"ao\":0,"
    "\"shapes\":[{\"ty\":\"gr\",\"it\":[{\"ind\":0,\"ty\":\"sh\",\"ix\":1,"
    "\"ks\":{\"a\":0,\"k\":{\"i\":[[0,0],[0,0],[0,0]],\"o\":[[0,0],[0,0],[0,0]],"
    "\"v\":[[-123,-66],[6,45],[321,-264]],\"c\":false},\"ix\":2},"
    "\"nm\":\"Path 1\",\"mn\":\"ADBE Vector Shape - Group\",\"hd\":false},"
    "{\"ty\":\"st\",\"c\":{\"a\":0,\"k\":[0.298039215686,0.686274509804,0.313725490196,1],\"ix\":3},"
    "\"o\":{\"a\":0,\"k\":100,\"ix\":4},\"w\":{\"a\":0,\"k\":52,\"ix\":5},"
    "\"lc\":2,\"lj\":2,\"bm\":0,\"nm\":\"Stroke 1\",\"mn\":\"ADBE Vector Graphic - Stroke\",\"hd\":false},"
    "{\"ty\":\"tr\",\"p\":{\"a\":0,\"k\":[0,0],\"ix\":2},\"a\":{\"a\":0,\"k\":[0,0],\"ix\":1},"
    "\"s\":{\"a\":0,\"k\":[100,100],\"ix\":3},\"r\":{\"a\":0,\"k\":0,\"ix\":6},"
    "\"o\":{\"a\":0,\"k\":100,\"ix\":7},\"sk\":{\"a\":0,\"k\":0,\"ix\":4},"
    "\"sa\":{\"a\":0,\"k\":0,\"ix\":5},\"nm\":\"Transform\"}],"
    "\"nm\":\"Shape 1\",\"np\":3,\"cix\":2,\"bm\":0,\"ix\":1,\"mn\":\"ADBE Vector Group\",\"hd\":false},"
    "{\"ty\":\"tm\",\"s\":{\"a\":0,\"k\":0,\"ix\":1},"
    "\"e\":{\"a\":1,\"k\":[{\"i\":{\"x\":[0.368],\"y\":[1]},\"o\":{\"x\":[0.251],\"y\":[0]},"
    "\"t\":10,\"s\":[0]},{\"t\":45,\"s\":[92]}],\"ix\":2},"
    "\"o\":{\"a\":0,\"k\":0,\"ix\":3},\"m\":1,\"ix\":2,\"nm\":\"Trim Paths 1\","
    "\"mn\":\"ADBE Vector Filter - Trim\",\"hd\":false}],\"ip\":10,\"op\":60,\"st\":0,\"bm\":0}],"
    "\"markers\":[]}";

// 简单的加载动画JSON
static const char lottie_loading_json[] = 
    "{\"assets\":[],\"layers\":[{\"ddd\":0,\"ind\":0,\"ty\":4,\"nm\":\"å½¢ç¶å¾å± 5\",\"ks\":{\"o\":{\"k\":[{\"i\":{\"x\":[0.833],\"y\":[0.833]},\"o\":{\"x\":[0.333],\"y\":[0]},\"n\":[\"0p833_0p833_0p333_0\"],\"t\":8,\"s\":[100],\"e\":[30]},{\"i\":{\"x\":[0.833],\"y\":[0.833]},\"o\":{\"x\":[0.333],\"y\":[0]},\"n\":[\"0p833_0p833_0p333_0\"],\"t\":24,\"s\":[30],\"e\":[100]},{\"t\":40}]},\"r\":{\"k\":0},\"p\":{\"k\":[187.875,77.125,0]},\"a\":{\"k\":[-76.375,-2.875,0]},\"s\":{\"k\":[{\"i\":{\"x\":[0.833,0.833,0.833],\"y\":[0.833,0.833,0.833]},\"o\":{\"x\":[0.333,0.333,0.333],\"y\":[0,0,0.333]},\"n\":[\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0p333\"],\"t\":8,\"s\":[100,100,100],\"e\":[200,200,100]},{\"i\":{\"x\":[0.833,0.833,0.833],\"y\":[0.833,0.833,0.833]},\"o\":{\"x\":[0.333,0.333,0.333],\"y\":[0,0,0.333]},\"n\":[\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0p333\"],\"t\":24,\"s\":[200,200,100],\"e\":[100,100,100]},{\"t\":40}]}},\"ao\":0,\"shapes\":[{\"ty\":\"gr\",\"it\":[{\"d\":1,\"ty\":\"el\",\"s\":{\"k\":[18,18]},\"p\":{\"k\":[0,0]},\"nm\":\"æ¤­åè·¯å¾ 1\",\"mn\":\"ADBE Vector Shape - Ellipse\"},{\"ty\":\"st\",\"c\":{\"k\":[1,1,1,1]},\"o\":{\"k\":100},\"w\":{\"k\":0},\"lc\":1,\"lj\":1,\"ml\":4,\"nm\":\"æè¾¹ 1\",\"mn\":\"ADBE Vector Graphic - Stroke\"},{\"ty\":\"fl\",\"c\":{\"k\":[0.87,0.42,0.56,1]},\"o\":{\"k\":100},\"nm\":\"å¡«å 1\",\"mn\":\"ADBE Vector Graphic - Fill\"},{\"ty\":\"tr\",\"p\":{\"k\":[-76.482,-3.482],\"ix\":2},\"a\":{\"k\":[0,0],\"ix\":1},\"s\":{\"k\":[100,100],\"ix\":3},\"r\":{\"k\":0,\"ix\":6},\"o\":{\"k\":100,\"ix\":7},\"sk\":{\"k\":0,\"ix\":4},\"sa\":{\"k\":0,\"ix\":5},\"nm\":\"åæ¢\"}],\"nm\":\"æ¤­å 1\",\"np\":3,\"mn\":\"ADBE Vector Group\"}],\"ip\":0,\"op\":40,\"st\":0,\"bm\":0,\"sr\":1},{\"ddd\":0,\"ind\":1,\"ty\":4,\"nm\":\"å½¢ç¶å¾å± 4\",\"ks\":{\"o\":{\"k\":[{\"i\":{\"x\":[0.833],\"y\":[0.833]},\"o\":{\"x\":[0.333],\"y\":[0]},\"n\":[\"0p833_0p833_0p333_0\"],\"t\":6,\"s\":[100],\"e\":[30]},{\"i\":{\"x\":[0.833],\"y\":[0.833]},\"o\":{\"x\":[0.333],\"y\":[0]},\"n\":[\"0p833_0p833_0p333_0\"],\"t\":22,\"s\":[30],\"e\":[100]},{\"t\":36}]},\"r\":{\"k\":0},\"p\":{\"k\":[162.125,76.625,0]},\"a\":{\"k\":[-76.375,-2.875,0]},\"s\":{\"k\":[{\"i\":{\"x\":[0.833,0.833,0.833],\"y\":[0.833,0.833,0.833]},\"o\":{\"x\":[0.333,0.333,0.333],\"y\":[0,0,0.333]},\"n\":[\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0p333\"],\"t\":6,\"s\":[100,100,100],\"e\":[200,200,100]},{\"i\":{\"x\":[0.833,0.833,0.833],\"y\":[0.833,0.833,0.833]},\"o\":{\"x\":[0.333,0.333,0.333],\"y\":[0,0,0.333]},\"n\":[\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0p333\"],\"t\":22,\"s\":[200,200,100],\"e\":[100,100,100]},{\"t\":36}]}},\"ao\":0,\"shapes\":[{\"ty\":\"gr\",\"it\":[{\"d\":1,\"ty\":\"el\",\"s\":{\"k\":[18,18]},\"p\":{\"k\":[0,0]},\"nm\":\"æ¤­åè·¯å¾ 1\",\"mn\":\"ADBE Vector Shape - Ellipse\"},{\"ty\":\"st\",\"c\":{\"k\":[1,1,1,1]},\"o\":{\"k\":100},\"w\":{\"k\":0},\"lc\":1,\"lj\":1,\"ml\":4,\"nm\":\"æè¾¹ 1\",\"mn\":\"ADBE Vector Graphic - Stroke\"},{\"ty\":\"fl\",\"c\":{\"k\":[0.81,0.55,0.82,1]},\"o\":{\"k\":100},\"nm\":\"å¡«å 1\",\"mn\":\"ADBE Vector Graphic - Fill\"},{\"ty\":\"tr\",\"p\":{\"k\":[-76.482,-3.482],\"ix\":2},\"a\":{\"k\":[0,0],\"ix\":1},\"s\":{\"k\":[100,100],\"ix\":3},\"r\":{\"k\":0,\"ix\":6},\"o\":{\"k\":100,\"ix\":7},\"sk\":{\"k\":0,\"ix\":4},\"sa\":{\"k\":0,\"ix\":5},\"nm\":\"åæ¢\"}],\"nm\":\"æ¤­å 1\",\"np\":3,\"mn\":\"ADBE Vector Group\"}],\"ip\":0,\"op\":40,\"st\":0,\"bm\":0,\"sr\":1},{\"ddd\":0,\"ind\":2,\"ty\":4,\"nm\":\"å½¢ç¶å¾å± 3\",\"ks\":{\"o\":{\"k\":[{\"i\":{\"x\":[0.833],\"y\":[0.833]},\"o\":{\"x\":[0.333],\"y\":[0]},\"n\":[\"0p833_0p833_0p333_0\"],\"t\":4,\"s\":[100],\"e\":[30]},{\"i\":{\"x\":[0.833],\"y\":[0.833]},\"o\":{\"x\":[0.333],\"y\":[0]},\"n\":[\"0p833_0p833_0p333_0\"],\"t\":20,\"s\":[30],\"e\":[100]},{\"t\":32}]},\"r\":{\"k\":0},\"p\":{\"k\":[135.625,76.625,0]},\"a\":{\"k\":[-76.375,-2.875,0]},\"s\":{\"k\":[{\"i\":{\"x\":[0.833,0.833,0.833],\"y\":[0.833,0.833,0.833]},\"o\":{\"x\":[0.333,0.333,0.333],\"y\":[0,0,0.333]},\"n\":[\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0p333\"],\"t\":4,\"s\":[100,100,100],\"e\":[200,200,100]},{\"i\":{\"x\":[0.833,0.833,0.833],\"y\":[0.833,0.833,0.833]},\"o\":{\"x\":[0.333,0.333,0.333],\"y\":[0,0,0.333]},\"n\":[\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0p333\"],\"t\":20,\"s\":[200,200,100],\"e\":[100,100,100]},{\"t\":32}]}},\"ao\":0,\"shapes\":[{\"ty\":\"gr\",\"it\":[{\"d\":1,\"ty\":\"el\",\"s\":{\"k\":[18,18]},\"p\":{\"k\":[0,0]},\"nm\":\"æ¤­åè·¯å¾ 1\",\"mn\":\"ADBE Vector Shape - Ellipse\"},{\"ty\":\"st\",\"c\":{\"k\":[1,1,1,1]},\"o\":{\"k\":100},\"w\":{\"k\":0},\"lc\":1,\"lj\":1,\"ml\":4,\"nm\":\"æè¾¹ 1\",\"mn\":\"ADBE Vector Graphic - Stroke\"},{\"ty\":\"fl\",\"c\":{\"k\":[0.47,0.31,0.62,1]},\"o\":{\"k\":100},\"nm\":\"å¡«å 1\",\"mn\":\"ADBE Vector Graphic - Fill\"},{\"ty\":\"tr\",\"p\":{\"k\":[-76.482,-3.482],\"ix\":2},\"a\":{\"k\":[0,0],\"ix\":1},\"s\":{\"k\":[100,100],\"ix\":3},\"r\":{\"k\":0,\"ix\":6},\"o\":{\"k\":100,\"ix\":7},\"sk\":{\"k\":0,\"ix\":4},\"sa\":{\"k\":0,\"ix\":5},\"nm\":\"åæ¢\"}],\"nm\":\"æ¤­å 1\",\"np\":3,\"mn\":\"ADBE Vector Group\"}],\"ip\":0,\"op\":40,\"st\":0,\"bm\":0,\"sr\":1},{\"ddd\":0,\"ind\":3,\"ty\":4,\"nm\":\"å½¢ç¶å¾å± 2\",\"ks\":{\"o\":{\"k\":[{\"i\":{\"x\":[0.833],\"y\":[0.833]},\"o\":{\"x\":[0.333],\"y\":[0]},\"n\":[\"0p833_0p833_0p333_0\"],\"t\":2,\"s\":[100],\"e\":[30]},{\"i\":{\"x\":[0.833],\"y\":[0.833]},\"o\":{\"x\":[0.333],\"y\":[0]},\"n\":[\"0p833_0p833_0p333_0\"],\"t\":16,\"s\":[30],\"e\":[100]},{\"t\":28}]},\"r\":{\"k\":0},\"p\":{\"k\":[109.375,76.625,0]},\"a\":{\"k\":[-76.625,-3.125,0]},\"s\":{\"k\":[{\"i\":{\"x\":[0.833,0.833,0.833],\"y\":[0.833,0.833,0.833]},\"o\":{\"x\":[0.333,0.333,0.333],\"y\":[0,0,0.333]},\"n\":[\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0p333\"],\"t\":2,\"s\":[100,100,100],\"e\":[200,200,100]},{\"i\":{\"x\":[0.833,0.833,0.833],\"y\":[0.833,0.833,0.833]},\"o\":{\"x\":[0.333,0.333,0.333],\"y\":[0,0,0.333]},\"n\":[\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0p333\"],\"t\":16,\"s\":[200,200,100],\"e\":[100,100,100]},{\"t\":28}]}},\"ao\":0,\"shapes\":[{\"ty\":\"gr\",\"it\":[{\"d\":1,\"ty\":\"el\",\"s\":{\"k\":[18,18]},\"p\":{\"k\":[0,0]},\"nm\":\"æ¤­åè·¯å¾ 1\",\"mn\":\"ADBE Vector Shape - Ellipse\"},{\"ty\":\"st\",\"c\":{\"k\":[1,1,1,1]},\"o\":{\"k\":100},\"w\":{\"k\":0},\"lc\":1,\"lj\":1,\"ml\":4,\"nm\":\"æè¾¹ 1\",\"mn\":\"ADBE Vector Graphic - Stroke\"},{\"ty\":\"fl\",\"c\":{\"k\":[0.54,0.81,0.89,1]},\"o\":{\"k\":100},\"nm\":\"å¡«å 1\",\"mn\":\"ADBE Vector Graphic - Fill\"},{\"ty\":\"tr\",\"p\":{\"k\":[-76.482,-3.482],\"ix\":2},\"a\":{\"k\":[0,0],\"ix\":1},\"s\":{\"k\":[100,100],\"ix\":3},\"r\":{\"k\":0,\"ix\":6},\"o\":{\"k\":100,\"ix\":7},\"sk\":{\"k\":0,\"ix\":4},\"sa\":{\"k\":0,\"ix\":5},\"nm\":\"åæ¢\"}],\"nm\":\"æ¤­å 1\",\"np\":3,\"mn\":\"ADBE Vector Group\"}],\"ip\":0,\"op\":40,\"st\":0,\"bm\":0,\"sr\":1},{\"ddd\":0,\"ind\":4,\"ty\":4,\"nm\":\"å½¢ç¶å¾å± 1\",\"ks\":{\"o\":{\"k\":[{\"i\":{\"x\":[0.833],\"y\":[0.833]},\"o\":{\"x\":[0.333],\"y\":[0]},\"n\":[\"0p833_0p833_0p333_0\"],\"t\":0,\"s\":[100],\"e\":[30]},{\"i\":{\"x\":[0.833],\"y\":[0.833]},\"o\":{\"x\":[0.333],\"y\":[0]},\"n\":[\"0p833_0p833_0p333_0\"],\"t\":12,\"s\":[30],\"e\":[100]},{\"t\":24}]},\"r\":{\"k\":0},\"p\":{\"k\":[82.625,76.625,0]},\"a\":{\"k\":[-76.625,-3.375,0]},\"s\":{\"k\":[{\"i\":{\"x\":[0.833,0.833,0.833],\"y\":[0.833,0.833,0.833]},\"o\":{\"x\":[0.333,0.333,0.333],\"y\":[0,0,0.333]},\"n\":[\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0p333\"],\"t\":0,\"s\":[100,100,100],\"e\":[200,200,100]},{\"i\":{\"x\":[0.833,0.833,0.833],\"y\":[0.833,0.833,0.833]},\"o\":{\"x\":[0.333,0.333,0.333],\"y\":[0,0,0.333]},\"n\":[\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0\",\"0p833_0p833_0p333_0p333\"],\"t\":12,\"s\":[200,200,100],\"e\":[100,100,100]},{\"t\":24}]}},\"ao\":0,\"shapes\":[{\"ty\":\"gr\",\"it\":[{\"d\":1,\"ty\":\"el\",\"s\":{\"k\":[18,18]},\"p\":{\"k\":[0,0]},\"nm\":\"æ¤­åè·¯å¾ 1\",\"mn\":\"ADBE Vector Shape - Ellipse\"},{\"ty\":\"st\",\"c\":{\"k\":[1,1,1,1]},\"o\":{\"k\":100},\"w\":{\"k\":0},\"lc\":1,\"lj\":1,\"ml\":4,\"nm\":\"æè¾¹ 1\",\"mn\":\"ADBE Vector Graphic - Stroke\"},{\"ty\":\"fl\",\"c\":{\"k\":[0.34,0.45,0.78,1]},\"o\":{\"k\":100},\"nm\":\"å¡«å 1\",\"mn\":\"ADBE Vector Graphic - Fill\"},{\"ty\":\"tr\",\"p\":{\"k\":[-76.482,-3.482],\"ix\":2},\"a\":{\"k\":[0,0],\"ix\":1},\"s\":{\"k\":[100,100],\"ix\":3},\"r\":{\"k\":0,\"ix\":6},\"o\":{\"k\":100,\"ix\":7},\"sk\":{\"k\":0,\"ix\":4},\"sa\":{\"k\":0,\"ix\":5},\"nm\":\"åæ¢\"}],\"nm\":\"æ¤­å 1\",\"np\":3,\"mn\":\"ADBE Vector Group\"}],\"ip\":0,\"op\":40,\"st\":0,\"bm\":0,\"sr\":1}],\"v\":\"4.5.4\",\"ddd\":0,\"ip\":0,\"op\":40,\"fr\":24,\"w\":280,\"h\":160}";
// 动画信息数组
static lottie_anim_info_t animations[LOTTIE_ANIM_COUNT] = {
    [LOTTIE_ANIM_APPROVE] = {
        .name = "Approve Animation",
        .flash_data = lottie_approve_json,
        .data_size = sizeof(lottie_approve_json),
        .psram_buffer = NULL,
        .loaded = false
    },
    [LOTTIE_ANIM_LOADING] = {
        .name = "Loading Animation", 
        .flash_data = lottie_loading_json,
        .data_size = sizeof(lottie_loading_json),
        .psram_buffer = NULL,
        .loaded = false
    },
    [LOTTIE_ANIM_ERROR] = {
        .name = "Error Animation",
        .flash_data = NULL, // 暂时没有实现
        .data_size = 0,
        .psram_buffer = NULL,
        .loaded = false
    }
};

static bool manager_initialized = false;

bool lottie_manager_init(void)
{
    if (manager_initialized) {
        ESP_LOGW(TAG, "Lottie manager already initialized");
        return true;
    }

#if !LV_USE_LOTTIE
    ESP_LOGE(TAG, "LVGL Lottie support not enabled");
    return false;
#endif

    ESP_LOGI(TAG, "Initializing Lottie manager...");
    
    // 检查PSRAM是否可用
    size_t psram_size = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    if (psram_size == 0) {
        ESP_LOGE(TAG, "PSRAM not available");
        return false;
    }
    
    ESP_LOGI(TAG, "PSRAM available: %zu bytes", psram_size);
    manager_initialized = true;
    
    return true;
}

static bool load_animation_to_psram(lottie_anim_id_t anim_id)
{
    if (anim_id >= LOTTIE_ANIM_COUNT) {
        ESP_LOGE(TAG, "Invalid animation ID: %d", anim_id);
        return false;
    }
    
    lottie_anim_info_t* anim = &animations[anim_id];
    
    if (anim->loaded) {
        ESP_LOGD(TAG, "Animation %s already loaded", anim->name);
        return true;
    }
    
    if (!anim->flash_data) {
        ESP_LOGE(TAG, "Animation %s has no data", anim->name);
        return false;
    }
    
    // 在PSRAM中分配内存
    anim->psram_buffer = heap_caps_malloc(anim->data_size, MALLOC_CAP_SPIRAM);
    if (!anim->psram_buffer) {
        ESP_LOGE(TAG, "Failed to allocate PSRAM for %s (%zu bytes)", 
                 anim->name, anim->data_size);
        return false;
    }
    
    // 从Flash复制到PSRAM
    memcpy(anim->psram_buffer, anim->flash_data, anim->data_size);
    anim->loaded = true;
    
    ESP_LOGI(TAG, "Loaded animation %s to PSRAM (%zu bytes)", 
             anim->name, anim->data_size);
    
    return true;
}

lv_obj_t* lottie_manager_create(lv_obj_t* parent, int32_t width, int32_t height)
{
    if (!manager_initialized) {
        ESP_LOGE(TAG, "Lottie manager not initialized");
        return NULL;
    }

#if !LV_USE_LOTTIE
    ESP_LOGE(TAG, "LVGL Lottie support not enabled");
    return NULL;
#else
    
    lv_obj_t* lottie = lv_lottie_create(parent ? parent : lv_screen_active());
    if (!lottie) {
        ESP_LOGE(TAG, "Failed to create lottie object");
        return NULL;
    }
    
    // 设置缓冲区
#if LV_DRAW_BUF_ALIGN == 4 && LV_DRAW_BUF_STRIDE_ALIGN == 1
    // 在PSRAM中分配缓冲区
    uint8_t* buf = heap_caps_malloc(width * height * 4, MALLOC_CAP_SPIRAM);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate buffer in PSRAM");
        lv_obj_delete(lottie);
        return NULL;
    }
    lv_lottie_set_buffer(lottie, width, height, buf);
    ESP_LOGI(TAG, "Allocated %dx%d buffer in PSRAM", width, height);
#else
    // 使用draw_buf
    static lv_draw_buf_t draw_buf;
    lv_draw_buf_init(&draw_buf, width, height, LV_COLOR_FORMAT_ARGB8888, 0, NULL, 0);
    lv_lottie_set_draw_buf(lottie, &draw_buf);
#endif
    
    lv_obj_center(lottie);
    
    ESP_LOGI(TAG, "Created lottie object %dx%d", width, height);
    return lottie;
    
#endif
}

bool lottie_manager_play(lv_obj_t* lottie, lottie_anim_id_t anim_id)
{
    if (!lottie || !manager_initialized) {
        ESP_LOGE(TAG, "Invalid parameters");
        return false;
    }

#if !LV_USE_LOTTIE
    ESP_LOGE(TAG, "LVGL Lottie support not enabled");
    return false;
#else
    
    // 加载动画到PSRAM
    if (!load_animation_to_psram(anim_id)) {
        ESP_LOGE(TAG, "Failed to load animation %d", anim_id);
        return false;
    }
    
    lottie_anim_info_t* anim = &animations[anim_id];
    
    // 设置动画数据
    lv_lottie_set_src_data(lottie, anim->psram_buffer, anim->data_size-1); // -1 因为不需要null终止符
    
    ESP_LOGI(TAG, "Started playing animation: %s", anim->name);
    return true;
    
#endif
}

void lottie_manager_pause(lv_obj_t* lottie)
{
    if (!lottie) return;
    
#if LV_USE_LOTTIE
    lv_anim_t* anim = lv_lottie_get_anim(lottie);
    if (anim) {
        lv_anim_set_repeat_count(anim, 0);
    }
#endif
}

void lottie_manager_resume(lv_obj_t* lottie)
{
    if (!lottie) return;
    
#if LV_USE_LOTTIE
    lv_anim_t* anim = lv_lottie_get_anim(lottie);
    if (anim) {
        lv_anim_set_repeat_count(anim, LV_ANIM_REPEAT_INFINITE);
    }
#endif
}

void lottie_manager_stop(lv_obj_t* lottie)
{
    if (!lottie) return;
    
#if LV_USE_LOTTIE
    lv_anim_t* anim = lv_lottie_get_anim(lottie);
    if (anim) {
        lv_anim_delete(anim, NULL);
    }
#endif
}

lottie_state_t lottie_manager_get_state(lv_obj_t* lottie)
{
    if (!lottie) return LOTTIE_STATE_ERROR;
    
#if LV_USE_LOTTIE
    lv_anim_t* anim = lv_lottie_get_anim(lottie);
    if (anim) {
        return LOTTIE_STATE_PLAYING;
    }
#endif
    
    return LOTTIE_STATE_IDLE;
}

void lottie_manager_deinit(void)
{
    if (!manager_initialized) return;
    
    ESP_LOGI(TAG, "Deinitializing Lottie manager...");
    
    // 释放所有PSRAM缓冲区
    for (int i = 0; i < LOTTIE_ANIM_COUNT; i++) {
        if (animations[i].psram_buffer) {
            heap_caps_free(animations[i].psram_buffer);
            animations[i].psram_buffer = NULL;
            animations[i].loaded = false;
        }
    }
    
    manager_initialized = false;
    ESP_LOGI(TAG, "Lottie manager deinitialized");
}
