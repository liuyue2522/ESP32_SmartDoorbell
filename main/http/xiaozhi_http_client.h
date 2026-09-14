#ifndef __XIAOZHI_HTTP_CLIENT_H__
#define __XIAOZHI_HTTP_CLIENT_H__
#include "esp_http_client.h" // HTTP 客户端核心头文件
#include "esp_crt_bundle.h" // HTTPS 证书头文件
#include "esp_log.h" // 日志头文件
#include "esp_wifi.h" // WiFi 头文件
#include <stdint.h>  // 标准整数头文件
#include <stdio.h> // 标准输入输出头文件
#include "esp_random.h" // 随机数头文件
#include "esp_heap_caps.h" // 堆内存头文件
#include "string.h" // 字符串头文件
#include "cJSON.h" // JSON 头文件
#include "xiaozhi_lvgl.h" // LVGL 头文件
#include "xiaozhi_data.h" // 数据头文件

// 1.初始化HTTP客户端
void xiaozhi_http_client_init(void);

#endif /* __XIAOZHI_HTTP_CLIENT_H__ */