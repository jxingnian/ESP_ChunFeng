#include "BAT_Driver.h"

// ADC调试日志标签
const static char *ADC_TAG = "ADC";

// 电池电压全局变量，单位为伏特
float BAT_analogVolts = 0;

/*---------------------------------------------------------------
        ADC 校准初始化函数
  说明：根据支持的校准方案（曲线拟合或线性拟合）初始化ADC校准句柄。
  参数：
    unit        - ADC单元号（如ADC_UNIT_1）
    channel     - ADC通道号
    atten       - ADC衰减设置
    out_handle  - 输出的校准句柄指针
  返回值：
    true  - 校准成功
    false - 校准失败或不支持
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;      // 校准句柄
    esp_err_t ret = ESP_FAIL;             // 返回值初始化为失败
    bool calibrated = false;              // 校准标志

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    // 优先尝试曲线拟合校准方案
    if (!calibrated) {
        ESP_LOGI(ADC_TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        // 创建曲线拟合校准句柄
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    // 如果不支持曲线拟合，则尝试线性拟合校准方案
    if (!calibrated) {
        ESP_LOGI(ADC_TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        // 创建线性拟合校准句柄
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle; // 输出校准句柄
    if (ret == ESP_OK) {
        ESP_LOGI(ADC_TAG, "Calibration Success"); // 校准成功
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(ADC_TAG, "eFuse not burnt, skip software calibration"); // 未烧录eFuse，跳过软件校准
    } else {
        ESP_LOGE(ADC_TAG, "Invalid arg or no memory"); // 参数错误或内存不足
    }

    return calibrated;
}

// ADC相关全局变量
adc_oneshot_unit_handle_t adc1_handle;           // ADC1单次采样句柄
bool do_calibration1_chan3;                      // 校准标志
adc_cali_handle_t adc1_cali_chan3_handle = NULL; // ADC1通道校准句柄

int adc_raw[2][10];      // ADC原始数据缓存（未实际用到多通道/多组，仅用[0][0]）
int voltage[2][10];      // 校准后电压值缓存（未实际用到多通道/多组，仅用[0][0]）

/**
 * @brief ADC初始化函数
 * 
 * 1. 初始化ADC1单次采样单元
 * 2. 配置ADC1指定通道的采样参数
 * 3. 初始化ADC校准（如支持）
 */
void ADC_Init(void)
{
    //-------------ADC1 单元初始化---------------//
    adc_oneshot_unit_init_cfg_t init_config1 = {                          
        .unit_id = ADC_UNIT_1, // 选择ADC1
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle)); // 创建ADC1单次采样句柄

    //-------------ADC1 通道配置---------------//
    adc_oneshot_chan_cfg_t config = {
        .atten = EXAMPLE_ADC_ATTEN,           // 衰减设置（决定最大输入电压范围）
        .bitwidth = ADC_BITWIDTH_DEFAULT,     // 位宽设置（一般为12位）
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN, &config)); // 配置ADC1通道

    //-------------ADC1 校准初始化---------------//
    do_calibration1_chan3 = example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC1_CHAN, EXAMPLE_ADC_ATTEN, &adc1_cali_chan3_handle);     
    // 校准成功后，do_calibration1_chan3为true，可进行后续电压校准
}

/**
 * @brief 电池检测初始化函数
 * 
 * 仅调用ADC初始化函数
 */
void BAT_Init(void)
{
    ADC_Init();
}

/**
 * @brief 获取当前电池电压值
 * 
 * 1. 读取ADC原始值
 * 2. 若支持校准，则将原始值转换为实际电压（单位：mV）
 * 3. 结合分压比和修正系数，计算实际电池电压（单位：V）
 * 
 * @return float 当前电池电压（单位：V）
 */
float BAT_Get_Volts(void)
{
    // 读取ADC原始数据（只用[0][0]，实际只采集一个通道一次）
    adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN, &adc_raw[0][0]);                                                     

    // 若支持校准，则将原始值转换为电压值（单位：mV）
    if (do_calibration1_chan3) {                                                                                           
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan3_handle, adc_raw[0][0], &voltage[0][0]));                    
        // 计算实际电池电压（分压比3:1，单位V，Measurement_offset为修正系数）
        BAT_analogVolts = (float)(voltage[0][0] * 3.0 / 1000.0) / Measurement_offset;
    }
    return BAT_analogVolts;
}