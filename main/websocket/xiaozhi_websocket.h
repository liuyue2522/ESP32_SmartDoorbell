#ifndef __XIAOZHI_WEBSOCKET_H__
#define __XIAOZHI_WEBSOCKET_H__

#include "esp_websocket_client.h" // websocket客户端
#include "xiaozhi_data.h" // 小智数据
#include "esp_crt_bundle.h" // 证书
#include "esp_log.h" // 日志
#include "esp_heap_caps.h" // 内存管理

// websocket连接服务器状态位
#define SERVER_CONNECTED_BIT (1<<0)
// 等待服务器回复Hello消息,确认建立连接状态位
#define CLINET_SERVER_CONNECTED_BIT (1<<1)


// 1.初始化websocket客户端
void xiaozhi_websocket_init(void);

// 2.webscoket客户端用于向服务器端发送文本消息方法
void xiaozhi_websocket_send_text(const char *text, int text_len);

// 3.websocket客户端向服务器端发送音频数据方法
void xiaozhi_websocket_send_audio(char *audio_data, int audio_data_len);

//4.检测唤醒词进行建立连接与发送唤醒词
void xiaozhi_websocket_start(void);

// 当对话结束后,客户端断开连接
void xiaozhi_websocket_stop(void);

#endif /* __XIAOZHI_WEBSOCKET_H__ */