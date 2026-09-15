#ifndef __XIAOZHI_AUDIO_H__
#define __XIAOZHI_AUDIO_H__
// I2C驱动
#include "driver/i2c_master.h"
// I2S驱动
#include "driver/i2s_std.h"
// 编解码器驱动
#include "esp_codec_dev.h"
// 编解码器默认参数
#include "esp_codec_dev_defaults.h"

// 1.编解码器初始化
void xiaozhi_audio_init(void);

// 播放声音
void xiaozhi_audio_play(void *buf, int len);

// 录制声音
void xiaozhi_audio_record(void *buf, int len);

#endif /* __XIAOZHI_AUDIO_H__ */