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

// 引用外部变量：  xiaozhi_lvgl.c 中定义的标题栏对象
extern lv_obj_t *title;

static char *TAG = "xiaozhi_main";

// 按键回调
void button_callBack(void *button_handle, void *usr_data);

// SR的声学前端检测到唤醒词回调函数
void wakeup_callback(void);
// 语音状态检测变化回调, 结束对话
void vad_state_callback(void);

// 处理服务器返回文本信息
void ws_text_callback(char *text, int len);
// 处理服务器音频信息
void ws_audio_callback(char *audio, int len);

// 创建项目中需要使用的环形缓冲区
static void xiaozhi_ringbuf_init(void);

// 提取 encoder_to_ws 缓冲区数据， 发送给websocket客户端
void ws_upload_task(void *pvParameters);

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
    xiaozhi_lvgl_update_title("小爱童鞋");
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

    

    // 3.BLE配网 + WIFI_STA模式连接上AP热点
    esp_err_t err =  xiaozhi_wifi_sta_init();


    if (err == ESP_OK)
    {
        // 4. HTTP之POST请求想获取虾哥智能体激活码,webscoket通信服务器地址.....
        xiaozhi_http_client_init();

        // 标记小智服务器状态
        xiaozhi_data.server_state = SERVER_STATE_IDLE;

        // 4.1 webscoket客户端初始化
        xiaozhi_websocket_init();

        // 5.语音识别模块SR初始化
        xiaozhi_sr_init();

        // 6.初始化opus编码器
        xiaozhi_encoder_init();

        // 7.解码器初始化
        xiaozhi_decoder_init();

        // 提取 encoder_to_ws 缓冲区数据， 发送给websocket客户端
        xTaskCreatePinnedToCoreWithCaps(ws_upload_task, "ws_upload_task", 32 * 1024, NULL, 5, NULL, 1, MALLOC_CAP_SPIRAM);
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

//-----------------------------------------------------------------
// 新增：WebSocket 连接任务
static void websocket_connect_task(void *params)
{
    xiaozhi_websocket_start();  // 阻塞式连接，放在独立任务里
    vTaskDelete(NULL);
}

// 检测到唤醒词执行一次
void wakeup_callback(void)
{
    ESP_LOGE(TAG, "MAIN wakeup_callback");
    // 检测到唤醒时,与小智服务器建立连接
    if (xiaozhi_data.server_state == SERVER_STATE_IDLE)
    {
        // 先设置为"连接中"，阻止 vad_state_callback 提前改状态
        xiaozhi_data.server_state = SERVER_STATE_CONNECTING;
        // 不要直接调用 xiaozhi_websocket_start()，改为创建任务
        xTaskCreatePinnedToCoreWithCaps(websocket_connect_task, "ws_connect", 8 * 1024, NULL, 5, NULL, 0, MALLOC_CAP_SPIRAM);
    }
    else if (xiaozhi_data.server_state == SERVER_STATE_SPEAKING)
    {
        // 小智正在说话的时候,让它终止
        xiaozhi_websocket_stop();
        // 再次换新新的聊天
        xiaozhi_websocket_send_wakeup();
    }
}
//--------------------------------------------------------------

// 语音状态检测变化回调, 结束对话
void vad_state_callback(void)
{
    if (xiaozhi_data.server_state == SERVER_STATE_CONNECTING)
    {
        return;  // 连接中，不处理
    }

    // 1.SR语音识别,人【不是小智】如果说话,需要让小智服务器处于监听状态
    // SR检测到有声音:有可能喇叭播放小智声音、人的声音!
    if (xiaozhi_data.current_vad_state == VAD_SPEECH)
    {
        // 能保证喇叭,小智绝对没有在说话
        if (xiaozhi_data.server_state == SERVER_STATE_IDLE)
        {

            // 在发送人声音二进制数据之前,下发监听命令,小智服务器处于监听状态
            xiaozhi_websocket_send_start_listen();

            // 小智服务器接收到这个命令,它的状态发生变化
            xiaozhi_data.server_state = SERVER_STATE_LISTENING;

            xiaozhi_lvgl_update_title("聆听中.....");
            // 标题开始闪烁
            xiaozhi_lvgl_start_blink(title);
        }
    }

    // 2.SR语音识别检测到静音,可以让小智服务器停止监听!!!!
    if (xiaozhi_data.current_vad_state == VAD_SILENCE)
    {
        // 小智服务器没有说话,一定检测不到声音 【没声、小智处于空闲、监听】
        if (xiaozhi_data.server_state == SERVER_STATE_IDLE)
        {
            // 真的下达停止监听命令
            xiaozhi_websocket_send_stop_listen();
            // 更新小智服务器状态空闲
            xiaozhi_data.server_state = SERVER_STATE_IDLE;
            xiaozhi_lvgl_update_title("小爱童鞋");
            // 标题栏停止闪烁
            xiaozhi_lvgl_stop_blink(title);
        }
    }
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

void ws_upload_task(void *pvParameters)
{
    while (1)
    {
        // 将encoder_to_ws缓冲区内部编码音频数据提取出来
        size_t len = 0;
        char *opus_data = xRingbufferReceive(xiaozhi_data.encoder_to_ws_handle, &len, portMAX_DELAY);

        // 提取 encoder_to_ws 缓冲区数据， 发送给websocket客户端
        // 小智服务器务必处于监听状态,把人的声音音频数据在上传给服务器
        if (xiaozhi_data.server_state == SERVER_STATE_LISTENING)
        {
            xiaozhi_websocket_send_audio(opus_data, len);
        }

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
        
        // 连接建立完成，恢复空闲状态，允许 vad_state_callback 处理
        if (xiaozhi_data.server_state == SERVER_STATE_CONNECTING)
        {
            xiaozhi_data.server_state = SERVER_STATE_IDLE;
        }
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
        
        // 查看小智[服务器]状态,修改
        if (strcmp(state, "start") == 0)
        {
            // 说明小智服务器开始返回文本消息、语音消息.开始说话了
            xiaozhi_data.server_state = SERVER_STATE_SPEAKING;
            // 更新标题,表示小智兄弟正在说话
            xiaozhi_lvgl_update_title("正在说话中......");
            // 标题开始闪烁
            xiaozhi_lvgl_start_blink(title);
        }

        if (strcmp(state, "stop") == 0)
        {
            // 小智服务器给咱们返回数据结束,它不在说话了
            xiaozhi_data.server_state = SERVER_STATE_IDLE;
            xiaozhi_lvgl_update_title("小爱童鞋");
            // 标题栏停止闪烁
            xiaozhi_lvgl_stop_blink(title);
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

