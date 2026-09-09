#include "xiaozhi_button.h"
// 按键句柄
button_handle_t adc_btn2 = NULL;
button_handle_t adc_btn3 = NULL;
// 1.ADC按键初始化
void xiaozhi_button_init(void)
{
    // ADC2按键初始化
    //  GPIO按键配置参数
    button_config_t gpio2_config = {0};
    // ADC按键配置参数
    button_adc_config_t adc2_config = {
        .unit_id = ADC_UNIT_1, // ADC选择:S3拥有两个ADC
        .adc_channel = 7,      // ADC1的通道7
        .button_index = 2,     // 按键索引
        .min = 0,              // 电压最小数值MV
        .max = 300,            // 电压最大数组MV
    };
    iot_button_new_adc_device(&gpio2_config, &adc2_config, &adc_btn2);

    // ADC3按键初始化
    //  GPIO按键配置参数
    button_config_t gpio3_config = {0};
    // ADC按键配置参数
    button_adc_config_t adc3_config = {
        .unit_id = ADC_UNIT_1, // ADC选择:S3拥有两个ADC
        .adc_channel = 7,      // ADC1的通道7
        .button_index = 3,     // 按键索引
        .min = 1300,           // 电压最小数值MV
        .max = 1800,           // 电压最大数组MV
    };
    iot_button_new_adc_device(&gpio3_config, &adc3_config, &adc_btn3);
}

// 2.按键2注册事件
void xiaozhi_button2_registerCallBack(button_event_t event, button_event_args_t *eventParams, button_cb_t cb, void *user_data)
{
    iot_button_register_cb(adc_btn2, event, eventParams, cb, user_data);
}

// 3.按键3注册事件
void xiaozhi_button3_registerCallBack(button_event_t event, button_event_args_t *eventParams, button_cb_t cb, void *user_data)
{
    iot_button_register_cb(adc_btn3, event, eventParams, cb, user_data);
}
