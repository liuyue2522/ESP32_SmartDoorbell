#include "xiaozhi_http_client.h"


// ---------------------------------全局参数-------------------------------------------
static const char *TAG = "xiaozhi_http_client";
// mac地址
uint8_t mac[6];
// mac地址字符拼接
char mac_str[18] = {0};
// 获取uuid->128为随机标识符
char uuid[37] = {0};
// HTTP客户端句柄
esp_http_client_handle_t http_client;


// ---------------------------------函数声明-------------------------------------------
// HTTP客户端发送请求头的方法
void xiaozhi_http_client_setSend_header(void);
// HTTP客户端发送请求体的方法
void xiaozhi_http_client_setSend_body(void);
// JSON形式字符串解析
void xiaozhi_http_client_json_parse(char *json_str);
// 生成uuid
void uuid_v4_generate(char *out);


// ----------------------------------------------------------------------------
// HTTP事件回调
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    // 缓冲区:想要存储HTTP请求服务器返回的响应数据
    static char *output_buffer = NULL;
    // 接收响应数据的长度
    static int output_len = 0;
    switch (evt->event_id)
    {
        // 请求失败
    case HTTP_EVENT_ERROR:
        ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
        break;
        // HTTP客户端连接服务器成功
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
        // HTTP客户端每次向服务器发送一个KV[请求头]就会执行一次
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
        // HTTP客户端接收到服务器响应头的数据
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        // 当服务器响应头数据返回:Content-Length知道服务器返回数据长度,开辟缓冲区准备接受返回数据
        if (strcmp(evt->header_key, "Content-Length") == 0)
        {
            if (output_buffer != NULL)
            {
                // 释放空间
                heap_caps_free(output_buffer);
                output_buffer = NULL;
                output_len = 0;
            }
            output_buffer = heap_caps_malloc(atoi(evt->header_value) + 1, MALLOC_CAP_SPIRAM);
            if (output_buffer == NULL)
            {
                ESP_LOGI(TAG, "内存分配失败");
            }
            else
            {
                ESP_LOGI(TAG, "内存分配成功");
            }
        }
        break;
        // http客户端接收到服务器响应JSON形式数据
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "接受数据:%.*s", evt->data_len, (char *)evt->data);
        // 分批次将返回数据放到缓冲区
        if (output_buffer != NULL)
        {
            memcpy(output_buffer + output_len, evt->data, evt->data_len);
            output_len += evt->data_len;
        }
        else
        {
            ESP_LOGE(TAG, "缓冲区大小为空，无法接受数据");
        }
        break;
        // 请求结束
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
        // 请求结束缓冲区空间需要释放
        if (output_buffer)
        {
            output_buffer[output_len] = '\0';   // 关键：添加字符串结束符
            // JSON形式字符串解析:解析JSON形式字符串务必在缓冲区空间释放之前解决掉
            xiaozhi_http_client_json_parse(output_buffer);
            // 释放空间
            heap_caps_free(output_buffer);
            output_buffer = NULL;
        }
        // 接受数据长度
        output_len = 0;
        break;
        // 客户端与服务器断开链接
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}
// 初始化HTTP客户端
void xiaozhi_http_client_init(void)
{
    // 1.HTTP客户端初始化配置参数
    esp_http_client_config_t config = {
        .host = "api.tenclass.net",                 // 请求的服务器域名
        .path = "/xiaozhi/ota/",                    // 请求路径
        .transport_type = HTTP_TRANSPORT_OVER_SSL,  // HTTP_TRANSPORT_OVER_TCP:HTTP明文传输不需要证书
        .crt_bundle_attach = esp_crt_bundle_attach, // 证书
        .method = HTTP_METHOD_POST,                 // 请求方法
        .event_handler = _http_event_handler,       // 事件回调函数
    };

    // 2.更新HTTP客户端配置参数,初始化HTTP客户端
    http_client = esp_http_client_init(&config);

    // 发送POST携带参数:请求头+请求体需要携带参数
    xiaozhi_http_client_setSend_header();
    // 请求体
    xiaozhi_http_client_setSend_body();
    // 3.想服务器发起HTTP请求【POST】
    // 只要HTTP请求失败,对于用户来说,没有网络,网络有问题!
    while (esp_http_client_perform(http_client) != ESP_OK)
    {
        // 没网,网络有问题
        xiaozhi_lvgl_update_title("网络有问题");
        xiaozhi_lvgl_update_emoji("crying");
        xiaozhi_lvgl_update_dialogue_stream("亲,请检查一下网络连接");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// 请求头
void xiaozhi_http_client_setSend_header(void)
{
    // 1.携带请求头User-Agent
    esp_http_client_set_header(http_client, "User-Agent", "bread-compact-wifi-128x64/1.0.1");

    // 2.携带请求头Device-Id:携带mac【六个字节】
    // WIFI初始化STA模式
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    esp_http_client_set_header(http_client, "Device-Id", mac_str);

    // 3.uuid
    uuid_v4_generate(uuid);
    ESP_LOGE(TAG, "uuid:%s", uuid);
    esp_http_client_set_header(http_client, "Client-Id", uuid);
}

// 请求体
void xiaozhi_http_client_setSend_body(void)
{
    static char body[512];
    snprintf(body, sizeof(body),
        "{\"application\":{\"version\":\"1.0.1\",\"elf_sha256\":\"c8a8ecb6...\"},"
        "\"board\":{\"type\":\"bread-compact-wifi\",\"name\":\"bread-compact-wifi-128x64\","
        "\"ssid\":\"卧室\",\"rssi\":-55,\"channel\":1,\"ip\":\"192.168.1.11\",\"mac\":\"%s\"}}",
        mac_str);
    // 请求体携带参数
    esp_http_client_set_post_field(http_client, body, strlen(body));
}

// JSON形式字符串解析
void xiaozhi_http_client_json_parse(char *json_str)
{
    if (json_str == NULL)
    {
        // 说明当前设备已激活
        xiaozhi_lvgl_update_title("json_str 字符串为空");
        xiaozhi_lvgl_update_emoji("crying");
        xiaozhi_lvgl_update_dialogue("请联系管理员");
        ESP_LOGE(TAG, "json_str 字符串为空");
        return;
    }
    // 1.将字符串[JSON形式],转化为CJSON结构体
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        xiaozhi_lvgl_update_title("json数据解析失败");
        xiaozhi_lvgl_update_emoji("crying");
        xiaozhi_lvgl_update_dialogue("请联系管理员");
        ESP_LOGE(TAG, "JSON 解析失败");
        return;
    }
    // 提取websocket、activation
    cJSON *websocket = cJSON_GetObjectItem(root, "websocket");
    cJSON *activation = cJSON_GetObjectItem(root, "activation");
    // 提取对话websocket服务器信息
    if (websocket != NULL)
    {
        // 对话服务器地址【websocket协议】
        cJSON *url_item = cJSON_GetObjectItem(websocket, "url");
        if (url_item && cJSON_IsString(url_item)) {
            char *url = cJSON_GetObjectItem(websocket, "url")->valuestring;
            // 存储到结构体成员中，将来别的组件使用，引入 xiaozhi_data.h 头文件即可
            strncpy(xiaozhi_data.websocket_url, url, sizeof(xiaozhi_data.websocket_url) - 1);
            xiaozhi_data.websocket_url[sizeof(xiaozhi_data.websocket_url) - 1] = '\0';
            ESP_LOGI(TAG, "websocket_url: %s", xiaozhi_data.websocket_url);
        }
        else
        {
            ESP_LOGE(TAG, "websocket url not found");
        }

        cJSON *token_item = cJSON_GetObjectItem(websocket, "token");
        if (token_item && cJSON_IsString(token_item)) {
            char *token = cJSON_GetObjectItem(websocket, "token")->valuestring;
            // 存储到结构体成员中，将来别的组件使用，引入 xiaozhi_data.h 头文件即可
            strncpy(xiaozhi_data.token, token, sizeof(xiaozhi_data.token) - 1);
            xiaozhi_data.token[sizeof(xiaozhi_data.token) - 1] = '\0';
            ESP_LOGI(TAG, "token: %s", xiaozhi_data.token);
        }
        else
        {
            ESP_LOGE(TAG, "token not found");
        }
    }

    // 提取激活码字段:可能有、可能无
    if (activation != NULL)
    {
        // 说明当前设备未激活
        char *code = cJSON_GetObjectItem(activation, "code")->valuestring;
        ESP_LOGE(TAG, "激活码:%s", code);
        // 更换LCD标题、表情、对话内容
        xiaozhi_lvgl_update_title("请您前往https://xiaozhi.me/激活设备");
        xiaozhi_lvgl_update_emoji("crying");
        char info[50] = {0};
        sprintf(info, "激活码:%s,激活后请重启设备", code);
        xiaozhi_lvgl_update_dialogue_stream(info);
    }
    else
    {
        // 说明当前设备已激活
        // 更新标题
        xiaozhi_lvgl_update_title("AI 小智");
        // 更新表情
        xiaozhi_lvgl_update_emoji("kissy");
        // 更新对话内容
        xiaozhi_lvgl_update_dialogue_stream("欢迎使用 AI·小智，请问有什么可以帮助您的？");
    }

    //释放json对象内存：  递归释放整棵树，包括所有子节点、键名、字符串值等。单独释放子节点反而会导致双重释放（double free）和程序崩溃。
    cJSON_Delete(root);
}

// uuid
void uuid_v4_generate(char *out)
{
    uint8_t uuid[16];

    // 1. 填充 16 字节随机数
    esp_fill_random(uuid, sizeof(uuid));

    // 2. 设置版本号：第 7 字节高 4 位 = 0100 (v4)
    uuid[6] = (uuid[6] & 0x0F) | 0x40;

    // 3. 设置变体：第 9 字节高 2 位 = 10 (RFC 4122)
    uuid[8] = (uuid[8] & 0x3F) | 0x80;

    // 4. 格式化成 8-4-4-4-12
    sprintf(out,
            "%02x%02x%02x%02x-"
            "%02x%02x-"
            "%02x%02x-"
            "%02x%02x-"
            "%02x%02x%02x%02x%02x%02x",
            uuid[0], uuid[1], uuid[2], uuid[3],
            uuid[4], uuid[5],
            uuid[6], uuid[7],
            uuid[8], uuid[9],
            uuid[10], uuid[11], uuid[12],
            uuid[13], uuid[14], uuid[15]);
}
