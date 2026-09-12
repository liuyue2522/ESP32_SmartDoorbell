#ifndef __XIAOZHI_BUTTON_H__
#define __XIAOZHI_BUTTON_H__

// ADC 按键硬件驱动层：这是 button 组件里专门针对 ADC 按键的驱动，用来支持“多个按键共用一个 ADC 引脚”的硬件设计。
#include "button_adc.h"

//  按键事件抽象层
/* 
支持的事件类型：

BUTTON_SINGLE_CLICK（单击）

BUTTON_DOUBLE_CLICK（双击）

BUTTON_LONG_PRESS_START（长按开始）

BUTTON_LONG_PRESS_HOLD（长按保持）

BUTTON_LONG_PRESS_UP（长按释放）

BUTTON_MULTIPLE_CLICK（连击） */
#include "iot_button.h"

// 日志输出：这是 ESP-IDF 官方核心日志库，用于在串口终端打印调试信息。
/*
提供 5 个级别：

ESP_LOGE（Error，红色）

ESP_LOGW（Warning，黄色）

ESP_LOGI（Info，绿色）

ESP_LOGD（Debug）

ESP_LOGV（Verbose）
 */
#include "esp_log.h"



//1.按键初始化[ADC按键]
void xiaozhi_button_init(void);

//2.按键2注册事件
void xiaozhi_button2_registerCallBack(button_event_t event, button_event_args_t * eventParams, button_cb_t cb, void * user_data);

//3.按键3注册事件
void xiaozhi_button3_registerCallBack(button_event_t event ,button_event_args_t * eventParams, button_cb_t cb, void * user_data);
#endif /* __XIAOZHI_BUTTON_H__ */