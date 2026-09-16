#include <stdio.h>
#include "esp_log.h"
#include "xiaozhi_button.h"
#include "xiaozhi_wifi_sta.h"
#include "xiaozhi_lvgl.h"
#include "xiaozhi_http_client.h"
#include "xiaozhi_sr.h"
#include "xiaozhi_data.h"
#include "xiaozhi_encoder.h"

static char *TAG = "xiaozhi_main";

// 按键回调
void button_callBack(void *button_handle, void *usr_data);

// SR的声学前端检测到唤醒词回调函数
void wakeup_callback(void);
// VAD状态变化回调函数
void vad_state_callback(void);

// 创建项目中需要使用的环形缓冲区
static void xiaozhi_ringbuf_init(void);

// -------------------------------------------------------------

void app_main(void)
{
    // 项目中任务通信需要使用到的消息队列【环形缓冲区】
    xiaozhi_ringbuf_init();

    xiaozhi_data.wakeup_callback = wakeup_callback;
    xiaozhi_data.vad_state_callback = vad_state_callback;

    // 0.初始化LVGL
    xiaozhi_lvgl_init();
    // 1. lvgl屏幕布局
    xiaozhi_lvgl_layout();

    // 更新标题
    xiaozhi_lvgl_update_title("AI 小智");
    // 更新表情
    xiaozhi_lvgl_update_emoji("kissy");
    // 更新对话内容
    xiaozhi_lvgl_update_dialogue_stream("正在连接WiFi，请稍后...");
    

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
    esp_err_t err =  xiaozhi_wifi_sta_init();


    // 4. HTTP之POST请求想获取虾哥智能体激活码,webscoket通信服务器地址.....
    if (err == ESP_OK)
    {
        xiaozhi_http_client_init();
    }

    // 5.语音识别模块SR初始化
    xiaozhi_sr_init();

    // 6.初始化opus编码器
    xiaozhi_encoder_init();
}

//-----------------------------------------------------------------------------------------

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

// 检测到唤醒词执行一次
void wakeup_callback(void)
{
    ESP_LOGE(TAG, "MAIN wakeup_callback");
}
// 语音状态检测变化回调
void vad_state_callback(void)
{
    ESP_LOGE(TAG, "MAIN vad_state_callback");

    // 清除唤醒标志
    xiaozhi_data.wakeup_flag = 0;
}

// 任务间通信使用缓冲区
static void xiaozhi_ringbuf_init(void)
{

    // 创建唤醒缓冲区:不可分割、可分割、字节流
    // 环形缓冲区大小:根据用户能接收到的语音延迟时间设计! 100-200ms
    // 1920->60ms: 200ms->4数据帧 缓冲缓冲区:(1920 * 4)/1024 = 7.5K
    // xiaozhi_data.sr_to_encoder_handle = xRingbufferCreateWithCaps(8 * 1024, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);
    xiaozhi_data.sr_to_encoder_handle = xRingbufferCreateWithCaps(8 * 1024, RINGBUF_TYPE_BYTEBUF, MALLOC_CAP_SPIRAM);
}
