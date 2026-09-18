#include "xiaozhi_websocket.h"

// websocket客户端
esp_websocket_client_handle_t client;

// 请求携带MAC地址与UUID
extern char mac_str[18];
// 获取uuid->128为随机标识符
extern char uuid[37];
static const char *TAG = "xiaozhi_websocket";

// 封装一个函数,用于客户端发送请求携带请求头
static void xiaozhi_websocket_send_header(void);

// websocket客户端向服务器发送Hello消息
void xiaozhi_websocket_send_hello(void);
// websocket客户端向服务器发送唤醒词消息
void xiaozhi_websocket_send_wakeup(void);

// websocket事件处理函数
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id)
    {
        // 准备开始连接服务器
    case WEBSOCKET_EVENT_BEGIN:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_BEGIN");
        break;
    // 客户端发送请求连上服务器
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        // 事件标志组
        xEventGroupSetBits(xiaozhi_data.event_group_handle, SERVER_CONNECTED_BIT);
        break;
    // 客户端与服务器端断开连接
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        break;
        // 客户端接收到服务端数据:文本 + 语音
    case WEBSOCKET_EVENT_DATA:
        // 服务器返回信息判断
        // 返回文本内容
        if (data->op_code == 0x01)
        {
            ESP_LOGE(TAG, "%.*s", data->data_len, data->data_ptr);
            // 注册回调
            if (xiaozhi_data.ws_text_callback != NULL)
            {
                xiaozhi_data.ws_text_callback(data->data_ptr, data->data_len);
            }
        }

        // 返回音频内容
        if (data->op_code == 0x02)
        {
            if (xiaozhi_data.ws_audio_callback != NULL)
            {
                xiaozhi_data.ws_audio_callback(data->data_ptr, data->data_len);
            }
        }
        break;
        // 通信出现错误
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
        break;
        // 通信结束
    case WEBSOCKET_EVENT_FINISH:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_FINISH");
        break;
    }
}
// 1.初始化websocket客户端
void xiaozhi_websocket_init(void)
{
    // 1.websocket客户端相关配置
    esp_websocket_client_config_t socket_config = {
        .uri = xiaozhi_data.websocket_url,              // 请求地址
        .transport = WEBSOCKET_TRANSPORT_OVER_SSL, // 请求方式加密+配合证书
        .crt_bundle_attach = esp_crt_bundle_attach,
        .network_timeout_ms = 10000,  // 网络超时时间
        .reconnect_timeout_ms = 3000, // 重连超时时间
    };

    // 2.初始化websocket客户端
    client = esp_websocket_client_init(&socket_config);

    // 3.注册事件回调
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);

    // 4.携带请求头
    xiaozhi_websocket_send_header();
}

// 连接服务器添加请求头
static void xiaozhi_websocket_send_header(void)
{
    // 携带请求头
    esp_websocket_client_append_header(client, "Protocol-Version", "1");
    esp_websocket_client_append_header(client, "Device-Id", mac_str);
    esp_websocket_client_append_header(client, "Client-Id", uuid);

    // 拼接token
    uint32_t len = strlen(xiaozhi_data.token) + strlen("Bearer ") + 1;
    char *params = heap_caps_malloc(len, MALLOC_CAP_SPIRAM);
    sprintf(params, "Bearer %s", xiaozhi_data.token);
    ESP_LOGE(TAG, "%s", params);
    esp_websocket_client_append_header(client, "Authorization", params);
    heap_caps_free(params);
    params = NULL;
}

/*****************************************************************************/
// 2.webscoket客户端用于向服务器端发送文本消息方法
void xiaozhi_websocket_send_text(const char *text, int text_len)
{
    esp_websocket_client_send_text(client, text, text_len, portMAX_DELAY);
}

// 3.websocket客户端向服务器端发送音频数据方法
void xiaozhi_websocket_send_audio(char *audio_data, int audio_data_len)
{
    esp_websocket_client_send_bin(client, audio_data, audio_data_len, portMAX_DELAY);
}

// 务必等待客户端连接成功以后,客户端再发送Hello消息
void xiaozhi_websocket_send_hello(void)
{
    char *Hello = "{\"type\":\"hello\",\"version\":1,\"transport\":\"websocket\",\"features\":{\"mcp\":true},\"audio_params\":{\"format\":\"opus\",\"sample_rate\":16000,\"channels\":1,\"frame_duration\":60}}";
    xiaozhi_websocket_send_text(Hello, strlen(Hello));
}

// 客户端发送唤醒词:务必在客户端与服务端正式建立连接以后
void xiaozhi_websocket_send_wakeup(void)
{

    // char *wakeup = "{\"type\":\"listen\",\"state\":\"detect\",\"text\":\"你好,小智\"}";
    char *wakeup = "{\"type\":\"listen\",\"state\":\"detect\",\"text\":\"今天天气怎么样\"}";
    // 发送唤醒词->你好小智,给服务器
    xiaozhi_websocket_send_text(wakeup, strlen(wakeup));
}

// 当SR语音识别检测唤醒词:连接服务器->发送Hello->发送唤醒词
void xiaozhi_websocket_start(void)
{
    // 6.发起webscoket请求,准备开连服务器
    // websocket客户端正常来说,不应该初始化就准备连服务器,应该SR检测到唤醒词,客户端在连服务器,准备建立连接!
    // 目前先书写在这里!!!!!
    esp_websocket_client_start(client);
    // 能保证客户端连上服务器
    xEventGroupWaitBits(xiaozhi_data.event_group_handle, SERVER_CONNECTED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
    // 发送Hello消息
    xiaozhi_websocket_send_hello();

    // 等待服务器回复Hello消息,确认建立连接
    xEventGroupWaitBits(xiaozhi_data.event_group_handle, CLINET_SERVER_CONNECTED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

    // 发送唤醒词消息
    xiaozhi_websocket_send_wakeup();
}

// 当对话结束后,客户端断开连接
void xiaozhi_websocket_stop(void)
{
    // 获取客户端和服务器的连接状态
    if (esp_websocket_client_is_connected(client))
    {
        char *stop = "{\"type\":\"abort\",\"reason\":\"wake_word_detected\"}";
        xiaozhi_websocket_send_text(stop, strlen(stop));

        esp_websocket_client_stop(client);
    }
    
}
