#ifndef __XIAOZHI_WIFI_STA_H__
#define __XIAOZHI_WIFI_STA_H__
// 字符串操作，如 strlen、memcpy。
#include <string.h>

// FreeRTOS 基础，用于任务、延时。如 vTaskDelay。
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 事件标志组，用于 xEventGroupWaitBits() 等待 Wi-Fi 连接结果。如 wifi_sta_connected。
#include "freertos/event_groups.h"

// esp_restart() 等系统函数。如 esp_restart。
#include "esp_system.h"

// Wi-Fi 驱动 API。
#include "esp_wifi.h"

// 事件循环，注册 Wi-Fi / IP 事件。
#include "esp_event.h"

// 日志输出。
#include "esp_log.h"

// NVS 初始化，保存 Wi-Fi 配置。
#include "nvs_flash.h"

// LwIP 协议栈错误码和系统接口
#include "lwip/err.h"
#include "lwip/sys.h"


//*新增蓝牙配网头文件
// 配网管理器
#include <wifi_provisioning/manager.h>
// BLE 配网方案
#include <wifi_provisioning/scheme_ble.h>

//*生成二维码
#include "qrcode.h"


// lvgl 相关
#include "xiaozhi_lvgl.h"



// 传输方式为 BLE。
#define PROV_TRANSPORT_BLE "ble"
// 二维码协议版本。
#define PROV_QR_VERSION "v1"

// 默认 Wi-Fi 名称（现在主要配网用，硬编码时用）。
#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
// 默认密码。
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
// 连接失败最大重试次数，在 event_handler 里使用。
#define EXAMPLE_ESP_MAXIMUM_RETRY CONFIG_ESP_MAXIMUM_RETRY

// 用于 WPA3 连接
#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif

// 认证模式阈值：扫描 AP 时，低于该认证模式的热点会被忽略。
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

// 事件标志位
// 连接成功并获取 IP 后置位。
#define WIFI_CONNECTED_BIT BIT0
// 重试次数超过上限后置位。
#define WIFI_FAIL_BIT BIT1

// 1.初始化WIFI_STA模式[MCU去链接AP热点]
esp_err_t xiaozhi_wifi_sta_init(void);


//2.可以擦除flash已有的WIFI账号与密码
void xiaozhi_wifi_sta_erase(void);

#endif /* __XIAOZHI_WIFI_STA_H__ */