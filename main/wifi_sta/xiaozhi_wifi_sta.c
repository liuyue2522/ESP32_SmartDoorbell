#include "xiaozhi_wifi_sta.h"
static const char *TAG = "wifi station";
// WIFI重连次数
static int s_retry_num = 0;
// 事件标志组句柄
static EventGroupHandle_t s_wifi_event_group;

// WIFI回调
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }

    //*配网相关新增事件
    if (event_base == WIFI_PROV_EVENT)
    {
        switch (event_id)
        {
            // 蓝牙配网开始:执行
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            break;
            // 蓝牙配网ESP32接受配网信息
        case WIFI_PROV_CRED_RECV:
        {
            wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG, "Received Wi-Fi credentials"
                          "\n\tSSID     : %s\n\tPassword : %s",
                     (const char *)wifi_sta_cfg->ssid,
                     (const char *)wifi_sta_cfg->password);
            break;
        }
            // 蓝牙配网接受信息失败
        case WIFI_PROV_CRED_FAIL:
        {
            wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                          "\n\tPlease reset to factory and retry provisioning",
                     (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
            break;
        }
        // 蓝牙配网成功
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "Provisioning successful");
            break;
        case WIFI_PROV_END:
            // 蓝牙配网结束:相应配网资源全部关闭,蓝牙资源!!!!!
            wifi_prov_mgr_deinit();
            break;
        default:
            break;
        }
    }

    // ESP32_低功耗蓝牙事件:手机是否脸上ESP32,手机蓝牙是否断开链接
    if (event_base == PROTOCOMM_TRANSPORT_BLE_EVENT)
    {
        switch (event_id)
        {
        case PROTOCOMM_TRANSPORT_BLE_CONNECTED:
            ESP_LOGI(TAG, "BLE transport: Connected!");
            break;
        case PROTOCOMM_TRANSPORT_BLE_DISCONNECTED:
            ESP_LOGI(TAG, "BLE transport: Disconnected!");
            break;
        default:
            break;
        }
    }

    // 蓝牙配网:BLE传输数据加密相关打印!
    if (event_base == PROTOCOMM_SECURITY_SESSION_EVENT)
    {
        switch (event_id)
        {
        case PROTOCOMM_SECURITY_SESSION_SETUP_OK:
            ESP_LOGI(TAG, "Secured session established!");
            break;
        case PROTOCOMM_SECURITY_SESSION_INVALID_SECURITY_PARAMS:
            ESP_LOGE(TAG, "Received invalid security parameters for establishing secure session!");
            break;
        case PROTOCOMM_SECURITY_SESSION_CREDENTIALS_MISMATCH:
            ESP_LOGE(TAG, "Received incorrect username and/or PoP for establishing secure session!");
            break;
        default:
            break;
        }
    }
}

//*获取配网当前蓝牙名字
static void get_device_service_name(char *service_name, size_t max)
{
    // 拼凑ESP32蓝牙设备名字
    const char *ssid_prefix = "PROV_";
    uint8_t eth_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);

    snprintf(service_name, max, "%s%02X%02X%02X",
             ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}

//*生成二维码图像
// 生成二维码图像
static void wifi_prov_print_qr(const char *name, const char *username, const char *pop, const char *transport)
{

    // 需要蓝牙名字
    if (!name || !transport)
    {
        ESP_LOGW(TAG, "Cannot generate QR code payload. Data missing.");
        return;
    }

    // 生成二维码数据-本质就是字符串
    char payload[150] = {0};
    if (pop)
    {

        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\""
                                           ",\"pop\":\"%s\",\"transport\":\"%s\"}",
                 PROV_QR_VERSION, name, pop, transport);
    }
    else
    {
        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\""
                                           ",\"transport\":\"%s\"}",
                 PROV_QR_VERSION, name, transport);
    }

    // 生成二维码图像,vsocde调试控制键打印出来!!!!!
    esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
    esp_qrcode_generate(&cfg, payload);
}

void xiaozhi_wifi_init_sta(void)
{
    // 1.事件标志组
    s_wifi_event_group = xEventGroupCreate();
    // 2.初始化网络层TCP/IP协议栈
    esp_netif_init();
    // 3.创建事件循环:底层本质,创建任务(内核0,优先级20)
    esp_event_loop_create_default();
    // 4.创建WIFI_STA模式
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // 5.WIFI_STA模式注册两个事件:WIFI_EVENT,IP_EVENT
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &event_handler,
                                        NULL,
                                        &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &event_handler,
                                        NULL,
                                        &instance_got_ip);

    //*BLE配网新增事件
    esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    esp_event_handler_register(PROTOCOMM_SECURITY_SESSION_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    esp_event_handler_register(PROTOCOMM_TRANSPORT_BLE_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);

    //***蓝牙配网
    wifi_prov_mgr_config_t config = {

        .scheme = wifi_prov_scheme_ble,                                      // WIFI配网模式选择:BLE、AP配网
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM // BLE配网完成,蓝牙用到内存资源释放掉!!!!!
    };

    // WIFI的蓝牙配网初始化
    wifi_prov_mgr_init(config);
    bool provisioned = false;
    // 检测当前设备是否配过网:曾经配网过
    wifi_prov_mgr_is_provisioned(&provisioned);

    // 判断:设备没有配网,应该进行配网
    if (!provisioned)
    {

        // 获取ESP32蓝牙设备的名字
        char service_name[12];
        get_device_service_name(service_name, sizeof(service_name));

        // 蓝牙传输数据加密算法1
        wifi_prov_security_t security = WIFI_PROV_SECURITY_1; // 安全系数较高,但是产品售卖的时候算法2【强烈推荐的!!!!!!!!】
        const char *pop = "abcd1234";
        wifi_prov_security1_params_t *sec_params = pop;
        const char *username = NULL;
        const char *service_key = NULL;

        // 当前设备身份唯一标识
        uint8_t custom_service_uuid[] = {
            0xb4,
            0xdf,
            0x5a,
            0x1c,
            0x3f,
            0x6b,
            0xf4,
            0xbf,
            0xea,
            0x4a,
            0x82,
            0x03,
            0x04,
            0x90,
            0x1a,
            0x02,
        };
        // 当前设备唯一标识设置:128位uuid
        wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);
        // 开始配网
        wifi_prov_mgr_start_provisioning(security, (const void *)sec_params, service_name, service_key);

        // 生成配网二维码
        wifi_prov_print_qr(service_name, username, pop, PROV_TRANSPORT_BLE);
    }
    else
    {

        // 已经配过网:配网资源释放掉
        wifi_prov_mgr_deinit();
        // WIFI处于STA模式,直接连接热点去了
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start();
    }

    // 等待联网结果
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    // 链接成功
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGE(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        // 链接失败
        ESP_LOGE(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

// WIFI_STA模式,让MCU可以去链接热点(路由器本事)
void xiaozhi_wifi_sta_init(void)
{
    // 1.初始化FLASH
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // 初始化WIFI_STAM模式
    xiaozhi_wifi_init_sta();
}

void xiaozhi_wifi_sta_erase(void)
{

    // 擦出掉FLASH的WFIFI配置信息
    esp_wifi_restore();

    // 软件复位
    esp_restart();
}