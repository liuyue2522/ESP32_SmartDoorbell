#ifndef __XIAOZHI_DATA_H__
#define __XIAOZHI_DATA_H__

#include "stdbool.h" // 布尔类型
#include "esp_vad.h" // 语音激活检测
#include "freertos/ringbuf.h" // 环形缓冲区

// 定义结构体:表示表情
typedef struct
{
    char *emotion; // 表情名称
    char *text;    // 表情文本

} EMOJI_T;

//对外暴露21表情包
extern EMOJI_T emoji_array[21];


// 存储项目多个组件共用的数据
typedef struct
{
    // 存储 websocker 服务器相关信息
    char websocket_url[256];
    char token[128];

    // 语音识别模块对全局暴露成员
    bool wakeup_flag; // SR,声学前端是否检测到唤醒词->你好小智
    // 注册唤醒词的回调函数
    void (*wakeup_callback)(void);

    // 成员,用来SR检测到语音当前状态
    vad_state_t current_vad_state;
    // 成员,用来存储SR语音状态上次状态
    vad_state_t last_vad_state;
    // 注册语音状态发生变化的回调
    void (*vad_state_callback)(void);

    // SR组件的任务 与 编码器的任务 通信缓冲区句柄
    RingbufHandle_t sr_to_encoder_handle;
    //encoder与ws通信使用缓冲区
    RingbufHandle_t encoder_to_ws_handle;
    //ws与解码器使用缓冲区
    RingbufHandle_t ws_to_decoder_handle;


} XIAOZHI_DATA_T;

extern XIAOZHI_DATA_T xiaozhi_data;

#endif /* __XIAOZHI_DATA_H__ */