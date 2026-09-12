#include <stdio.h>
#include "esp_log.h"
#include "xiaozhi_button.h"
#include "xiaozhi_wifi_sta.h"
#include "xiaozhi_lvgl.h"

static char *TAG = "xiaozhi_button";

// 按键回调
void button_callBack(void *button_handle, void *usr_data);

void app_main(void)
{
    // 0.初始化LVGL
    xiaozhi_lvgl_init();
    // 1. lvgl屏幕布局
    xiaozhi_lvgl_layout();
    
    
    // 2.adc按键初始化
    xiaozhi_button_init();
    // 2.按键注册单机与双机事件
    xiaozhi_button2_registerCallBack(BUTTON_SINGLE_CLICK, NULL, button_callBack, (void *)1);
    xiaozhi_button2_registerCallBack(BUTTON_DOUBLE_CLICK, NULL, button_callBack, (void *)2);
    button_event_args_t longparams = {
        .long_press = {
            .press_time = 3000}};
    xiaozhi_button3_registerCallBack(BUTTON_LONG_PRESS_UP, &longparams, button_callBack, (void *)3);

    

    // 3.目前WIFI_STA模式,只能让咱们当前设备链接AP[JCH 12345678],不支持用户配网
    xiaozhi_wifi_sta_init();
}

void button_callBack(void *button_handle, void *usr_data)
{
    uint8_t button_id = (uint8_t)usr_data;
    switch (button_id)
    {
    case 1:
        ESP_LOGI(TAG, "button single click");
        // 清除WIFI信息
        xiaozhi_wifi_sta_erase();
        break;
    case 2:
        ESP_LOGI(TAG, "button double click");
        break;
    case 3:
        ESP_LOGI(TAG, "button long press up");
        break;

    default:
        break;
    }
}