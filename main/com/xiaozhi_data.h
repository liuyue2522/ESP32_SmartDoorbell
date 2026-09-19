#ifndef __XIAOZHI_DATA_H__
#define __XIAOZHI_DATA_H__

#include "stdbool.h" // 布尔类型
#include "esp_vad.h" // 语音激活检测
#include "freertos/ringbuf.h" // 环形缓冲区
#include "freertos/event_groups.h" // 事件标志组

// 定义结构体:表示表情
typedef struct
{
    char *emotion; // 表情名称
    char *text;    // 表情文本

} EMOJI_T;


typedef enum
{
    SERVER_STATE_IDLE, // 小智空闲状态
    SERVER_STATE_SPEAKING, // 小智正在说话
    SERVER_STATE_LISTENING, // 小智正在监听
} SERVER_STATE_T;


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

    // SR组件任务 与 编码器任务 的通信缓冲区句柄
    RingbufHandle_t sr_to_encoder_handle;
    // encoder 与 ws 的通信缓冲区句柄
    RingbufHandle_t encoder_to_ws_handle;
    // ws 与 解码器 的通信缓冲区句柄
    RingbufHandle_t ws_to_decoder_handle;

    //事件标志组句柄
    EventGroupHandle_t event_group_handle;

    //注册处理服务器返回文本信息回调函数
    void (*ws_text_callback)(char *text,int text_len);
    //注册处理服务器返回语音数据回调
    void (*ws_audio_callback)(char *audio,int audio_len);

    // 服务器状态
    SERVER_STATE_T server_state;

} XIAOZHI_DATA_T;


//对外暴露21表情包
extern EMOJI_T emoji_array[21];

extern XIAOZHI_DATA_T xiaozhi_data;

//--------------------------------------------------------


#endif /* __XIAOZHI_DATA_H__ */