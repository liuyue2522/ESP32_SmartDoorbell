#ifndef __XIAOZHI_ENCODER_H__
#define __XIAOZHI_ENCODER_H__

#include "esp_opus_enc.h" // 编码器
#include "stdio.h" // 输入输出
#include "esp_log.h" // 日志
#include "esp_heap_caps.h" // 内存
#include "freertos/idf_additions.h" // freertos


//1.初始化OPUS编码器
void xiaozhi_encoder_init(void);







/**
  1. 以后工作时候,处理音频数据,给服务器送过去之前,进行编码压缩!!!延迟小一点
  2.虾哥服务器:音频数据,16KHZ,单声道,位深16位！人家服务器需要帧长:60ms! 采样点:960 * 2 = 1920->压缩过后送过去!
**/


/* 
        PCM 是原始、无损的音频数据，而 Opus 是高效、有损的压缩格式。在 ESP32 项目中，为了节省网络带宽和存储空间，
    通常会在发送前将 PCM 编码为 Opus，在接收后解码回 PCM 进行播放。你可以通过集成 esp-opus 等组件在设备上实时转换
*/

#endif /* __XIAOZHI_ENCODER_H__ */
