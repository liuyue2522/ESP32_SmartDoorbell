#ifndef __XIAOZHI_HTTP_CLIENT_H__
#define __XIAOZHI_HTTP_CLIENT_H__
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include <stdint.h>
#include <stdio.h>
#include "esp_random.h"
#include "esp_heap_caps.h"
#include "string.h"
#include "cJSON.h"
#include "xiaozhi_lvgl.h"
#include "xiaozhi_data.h"

// 1.初始化HTTP客户端
void xiaozhi_http_client_init(void);

#endif /* __XIAOZHI_HTTP_CLIENT_H__ */