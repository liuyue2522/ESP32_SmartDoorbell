#include <stdio.h>
#include "esp_log.h"
#include "xiaozhi_button.h"
#include "xiaozhi_wifi_sta.h"
#include "xiaozhi_lvgl.h"
#include "xiaozhi_http_client.h"
#include "xiaozhi_sr.h"
#include "xiaozhi_data.h"
#include "xiaozhi_encoder.h"
#include "xiaozhi_decoder.h"
#include "xiaozhi_websocket.h"
#include "freertos/event_groups.h"

static char *TAG = "xiaozhi_main";

// 按键回调
void button_callBack(void *button_handle, void *usr_data);

// SR的声学前端检测到唤醒词回调函数
void wakeup_callback(void);
// VAD状态变化回调函数
void vad_state_callback(void);

// 处理服务器返回文本信息
void ws_text_callback(char *text, int len);
// 处理服务器音频信息
void ws_audio_callback(char *audio, int len);

// 创建项目中需要使用的环形缓冲区
static void xiaozhi_ringbuf_init(void);

// 短接使用测试任务
void test_task(void *pvParameters);

// -------------------------------------------------------------

void app_main(void)
{
    // 创建事件标志组:用于websocket组件
    xiaozhi_data.event_group_handle = xEventGroupCreate();

    // 项目中任务通信需要使用到的消息队列【环形缓冲区】
    xiaozhi_ringbuf_init();

    xiaozhi_data.wakeup_callback = wakeup_callback;
    xiaozhi_data.vad_state_callback = vad_state_callback;

    // 处理服务器返回文本信息
    xiaozhi_data.ws_text_callback = ws_text_callback;
    // 处理服务器返回语音信息
    xiaozhi_data.ws_audio_callback = ws_audio_callback;

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


    if (err == ESP_OK)
    {
        // 4. HTTP之POST请求想获取虾哥智能体激活码,webscoket通信服务器地址.....
        xiaozhi_http_client_init();

        // 4.1 webscoket客户端初始化
        xiaozhi_websocket_init();

        // 5.语音识别模块SR初始化
        xiaozhi_sr_init();

        // 6.初始化opus编码器
        xiaozhi_encoder_init();

        // 7.解码器初始化
        xiaozhi_decoder_init();

        // 测试任务
        xTaskCreatePinnedToCoreWithCaps(test_task, "test_task", 32 * 1024, NULL, 5, NULL, 1, MALLOC_CAP_SPIRAM);
    }

    
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
    xiaozhi_websocket_start();
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
    // 1.创建唤醒缓冲区:字节流
    // 环形缓冲区大小:根据用户能接收到的语音延迟时间设计! 100-200ms
    // 1920->60ms: 200ms->4数据帧 缓冲缓冲区:(1920 * 4)/1024 = 7.5K
    xiaozhi_data.sr_to_encoder_handle = xRingbufferCreateWithCaps(8 * 1024, RINGBUF_TYPE_BYTEBUF, MALLOC_CAP_SPIRAM);

    // 2.这个环形缓冲区:编码器组件内部任务与webscoket客户端组件内部任务通信使用
    // 不可分割缓冲区
    xiaozhi_data.encoder_to_ws_handle = xRingbufferCreateWithCaps(8 * 1024, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);

    // 3.webscoket组件内部任务与解码器组件内部任务通信缓冲区
    xiaozhi_data.ws_to_decoder_handle = xRingbufferCreateWithCaps(8 * 1024, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);
}

void test_task(void *pvParameters)
{
    while (1)
    {
        // 将encoder_to_ws缓冲区内部编码音频数据提取出来
        size_t len = 0;
        uint8_t *opus_data = xRingbufferReceive(xiaozhi_data.encoder_to_ws_handle, &len, portMAX_DELAY);
        // 提取数据数据 发送给websocket客户端
        // ----------------------------------------------

        // 用完的数据一定要释放
        vRingbufferReturnItem(xiaozhi_data.encoder_to_ws_handle, opus_data);
    }
}



/******************************************处理虾哥服务器返回文本、音频数据------------------------------------- */

// 处理服务器返回文本信息
void ws_text_callback(char *text, int len)
{
    if (text == NULL || len <= 0)
    {
        // 说明当前设备已激活
        xiaozhi_lvgl_update_title("text 字符串为空");
        xiaozhi_lvgl_update_emoji("crying");
        xiaozhi_lvgl_update_dialogue("请联系管理员");
        ESP_LOGE(TAG, "text 字符串为空");
        return;
    }

    // 解析服务器返回文本信息
    cJSON *root = cJSON_Parse(text);
    if (root == NULL) {
        xiaozhi_lvgl_update_title("json数据解析失败");
        xiaozhi_lvgl_update_emoji("crying");
        xiaozhi_lvgl_update_dialogue("请联系管理员");
        ESP_LOGE(TAG, "JSON 解析失败");
        return;
    }

    // 提取type:Hello,表示客户端与服务器正式建立连接
    char *type = cJSON_GetObjectItem(root, "type")->valuestring;
    if (strcmp(type, "hello") == 0)
    {
        // 客户端收到服务端返回Hello消息,表明与客户端正式建立连接
        xEventGroupSetBits(xiaozhi_data.event_group_handle, CLINET_SERVER_CONNECTED_BIT);
    }

    // 更新LCD屏幕表情
    if (strcmp(type, "llm") == 0)
    {
        // 提取表情
        char *emotion = cJSON_GetObjectItem(root, "emotion")->valuestring;
        // 更新表情
        xiaozhi_lvgl_update_emoji(emotion);
    }

    // 更新LCD底部对话内容
    if (strcmp(type, "tts") == 0)
    {
        char *state = cJSON_GetObjectItem(root, "state")->valuestring;
        // 并不是JSONtype=TTS都是对话内容,必须要保证state  = sentence_start
        if (strcmp(state, "sentence_start") == 0)
        {
            char *text = cJSON_GetObjectItem(root, "text")->valuestring;
            // 更新对话内容
            xiaozhi_lvgl_update_dialogue_stream(text);
        }
    }

    // 释放内存
    cJSON_Delete(root);
}
// 处理服务器音频信息
void ws_audio_callback(char *audio, int len)
{
    // 处理服务器返回音频数据进行播放
    xRingbufferSend(xiaozhi_data.ws_to_decoder_handle, audio, len, portMAX_DELAY);
}

