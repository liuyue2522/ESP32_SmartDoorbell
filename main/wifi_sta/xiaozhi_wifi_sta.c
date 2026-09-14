#include "xiaozhi_wifi_sta.h"

// 日志标签
static const char *TAG = "wifi station";

// 记录 Wi-Fi 连接失败重试次数，超过上限后置位失败标志。
static int s_retry_num = 0;

// 事件标志组句柄：FreeRTOS 事件组，用于在"连接成功"和"连接失败"之间同步主任务。
static EventGroupHandle_t s_wifi_event_group;

// 引用外部变量：  xiaozhi_lvgl.c 中定义的标题栏对象
extern lv_obj_t *title;

// WIFI回调
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    // 1.Wi-Fi 事件
    // Wi-Fi 启动后立刻发起连接
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    // Wi-Fi 连接失败重试
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) // 最多重试 5 次
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT); // 超过重试上限，置位失败标志

            // 更新标题
            xiaozhi_lvgl_update_title("wifi 连接超过重试上限");
            // wifi连接失败：标题栏闪烁
            xiaozhi_lvgl_start_blink(title);
            // 更新表情
            xiaozhi_lvgl_update_emoji("crying");
            // 更新对话内容
            xiaozhi_lvgl_update_dialogue_stream("请重新进行配网");
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }

    // 2.IP事件
    // 获得 IP 地址，说明 Wi-Fi 真正连接成功。
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        // 置位成功标志。
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        // wifi连接成功：标题栏停止闪烁
        xiaozhi_lvgl_stop_blink(title);
    }

    // 3. 配网事件（WIFI_PROV_EVENT）
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
            // 配网成功，删除二维码
            xiaozhi_lvgl_del_qrcode();
            break;
        case WIFI_PROV_END:
            // 蓝牙配网结束:相应配网资源全部关闭,蓝牙资源!!!!!
            wifi_prov_mgr_deinit();
            break;
        default:
            break;
        }
    }

    // 4.ESP32_低功耗蓝牙传输事件: 手机是否连上ESP32,手机蓝牙是否断开链接
    // 用于监控手机 App 与 ESP32 的 BLE 连接状态。
    if (event_base == PROTOCOMM_TRANSPORT_BLE_EVENT)
    {
        switch (event_id)
        {
            // 手机连上蓝牙
        case PROTOCOMM_TRANSPORT_BLE_CONNECTED:
            ESP_LOGI(TAG, "BLE transport: Connected!");

            // 更新标题
            xiaozhi_lvgl_update_title("蓝牙连接成功");
            // 更新表情
            xiaozhi_lvgl_update_emoji("kissy");
            // 更新对话内容
            xiaozhi_lvgl_update_dialogue_stream("请选择 WIFI 进行连接");

            break;
            // 手机断开蓝牙
        case PROTOCOMM_TRANSPORT_BLE_DISCONNECTED:
            break;
        default:
            break;
        }
    }

    // 安全会话事件: BLE传输数据加密相关打印!
    if (event_base == PROTOCOMM_SECURITY_SESSION_EVENT)
    {
        switch (event_id)
        {
            // 加密会话建立成功
        case PROTOCOMM_SECURITY_SESSION_SETUP_OK:
            ESP_LOGI(TAG, "Secured session established!");
            break;
            // 参数错误
        case PROTOCOMM_SECURITY_SESSION_INVALID_SECURITY_PARAMS:
            ESP_LOGE(TAG, "Received invalid security parameters for establishing secure session!");
            break;
            // POP 或用户名不匹配
        case PROTOCOMM_SECURITY_SESSION_CREDENTIALS_MISMATCH:
            ESP_LOGE(TAG, "Received incorrect username and/or PoP for establishing secure session!");
            break;
        default:
            break;
        }
    }
}

//*获取配网当前蓝牙名字
/* 
读取 Wi-Fi STA 的 MAC 地址。
用 MAC 后 3 字节拼成唯一蓝牙名，如 liuyue_9E9E00。
手机 App 搜到的就是这个蓝牙名。
*/
static void get_device_service_name(char *service_name, size_t max)
{
    // 拼凑ESP32蓝牙设备名字
    const char *ssid_prefix = "liuyue_"; // 前缀
    uint8_t eth_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);

    snprintf(service_name, max, "%s%02X%02X%02X",
             ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}

// 生成配网二维码
/* 
生成一个 JSON 字符串，包含协议版本、蓝牙名、PoP 密码、传输方式。
手机 App 扫描这个二维码后，会自动通过 BLE 连接设备，并发送 Wi-Fi 账号密码。
esp_qrcode_generate 是 qrcode 组件提供的，会把二维码以 ASCII 形式打印到串口终端。
*/
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

    // 生成二维码图像
    esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
    // qrcode 组件提供的，会把二维码以 ASCII 形式打印到串口终端。
    esp_qrcode_generate(&cfg, payload);

    // 设置屏幕标题
    xiaozhi_lvgl_update_title("请扫描二维码");
    // 更新表情
    xiaozhi_lvgl_update_emoji("cool");
    // 更新对话内容
    xiaozhi_lvgl_update_dialogue_stream("等待连接蓝牙进行配网");

    // LCD显示二维码
    xiaozhi_lvgl_show_qrcode(payload);
}

// 核心初始化流程
esp_err_t xiaozhi_wifi_init_sta(void)
{
    // 1.事件标志组
    s_wifi_event_group = xEventGroupCreate();
    // 2.初始化网络层TCP/IP协议栈
    esp_netif_init();
    // 3.创建事件循环:底层本质,创建任务(内核0,优先级20)
    esp_event_loop_create_default();
    // 4.创建WIFI_STA模式
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    // ★ 设置设备主机名（路由器上显示的名称）
    esp_netif_set_hostname(sta_netif, "liuyue"); 

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


    // 初始化配网管理器, 配置配网模式为BLE配网
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
        wifi_prov_security_t security = WIFI_PROV_SECURITY_1; // 使用 PoP 加密。    安全系数较高,但是产品售卖的时候算法2【强烈推荐的!!!!!!!!】
        const char *pop = "abcd1234"; // PoP 密码
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


     // 连接成功
    if (bits & WIFI_CONNECTED_BIT)
    {
        wifi_config_t cfg = {0};
        esp_wifi_get_config(WIFI_IF_STA, &cfg);
        ESP_LOGE(TAG, "connected to ap SSID:%s password:%s",
                cfg.sta.ssid, cfg.sta.password);
        // 更新标题
        xiaozhi_lvgl_update_title("WIFI连接成功");
        // wifi连接成功：标题栏停止闪烁
        xiaozhi_lvgl_stop_blink(title);
        // 更新表情
        xiaozhi_lvgl_update_emoji("kissy");
        // 更新对话内容
        xiaozhi_lvgl_update_dialogue_stream("等待连接服务器...");

        return ESP_OK;
    }
    else if (bits & WIFI_FAIL_BIT) // 链接失败
    {
        wifi_config_t cfg = {0};
        esp_wifi_get_config(WIFI_IF_STA, &cfg);
        ESP_LOGE(TAG, "Failed to connect to SSID:%s, password:%s",
                cfg.sta.ssid, cfg.sta.password);
        // 更新标题
        xiaozhi_lvgl_update_title("WIFI连接失败");
        // wifi连接失败：标题栏闪烁
        xiaozhi_lvgl_start_blink(title);
        // 更新表情
        xiaozhi_lvgl_update_emoji("crying");
        // 更新对话内容
        xiaozhi_lvgl_update_dialogue_stream("请重试");
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    return ESP_FAIL;
}




// WIFI_STA模式,让MCU可以去链接热点(路由器本事)
esp_err_t xiaozhi_wifi_sta_init(void)
{
    // 1.初始化FLASH： 初始化 NVS（保存 Wi-Fi 配置）
    esp_err_t ret = nvs_flash_init();

    // 2.如果 NVS 分区满或版本不匹配，先擦除再初始化
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // 3.初始化WIFI_STAM模式
    return xiaozhi_wifi_init_sta();
}

// 擦除配网信息
void xiaozhi_wifi_sta_erase(void)
{

    // 擦出掉FLASH的WFIFI配置信息
    esp_wifi_restore();

    // 软件复位
    esp_restart();
}