#ifndef __XIAOZHI_DECODER_H__
#define __XIAOZHI_DECODER_H__

#include "esp_opus_dec.h" // 1.解码器头文件
#include "freertos/idf_additions.h" // 2.任务头文件
#include "xiaozhi_data.h" // 3.数据头文件
#include "xiaozhi_audio.h" // 4.音频头文件

// 1.解码器初始化
void xiaozhi_decoder_init(void);

#endif /* __XIAOZHI_DECODER_H__ */

/**
  1.解码器用途:将服务器返回语音数据【压缩opus音频数据】,转换为PCM音频数据保真播放!
 **/