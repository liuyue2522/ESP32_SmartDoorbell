#ifndef __XIAOZHI_BUTTON_H__
#define __XIAOZHI_BUTTON_H__
#include "button_adc.h"
#include "iot_button.h"
#include "esp_log.h"
//1.按键初始化[ADC按键]
void xiaozhi_button_init(void);

//2.按键2注册事件
void xiaozhi_button2_registerCallBack(button_event_t event, button_event_args_t * eventParams, button_cb_t cb, void * user_data);

//3.按键3注册事件
void xiaozhi_button3_registerCallBack(button_event_t event ,button_event_args_t * eventParams, button_cb_t cb, void * user_data);
#endif /* __XIAOZHI_BUTTON_H__ */