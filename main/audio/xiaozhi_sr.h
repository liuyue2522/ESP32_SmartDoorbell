#ifndef __XIAOZHI_SR_H__
#define __XIAOZHI_SR_H__

/* ESP-SR 音频前端（AFE） 的核心接口文件，定义了操作 AFE 算法的所有标准方法
集成回声消除（AEC）、噪声抑制（NS）、语音活动检测（VAD）和唤醒词识别（WakeNet） 等语音处理功能。 */
#include "esp_afe_sr_iface.h" 
/* 管理 ESP-SR 所需的各种语音模型（如唤醒词模型、语音识别模型）
提供 srmodel_list_t 等结构体，用于初始化和检索模型列表。 */
#include "esp_afe_sr_models.h"
/* 这是 ESP-IDF 对标准 FreeRTOS 的功能扩展头文件。 */
#include "freertos/idf_additions.h"
/* 日志输出 */
#include "esp_log.h"
/* 内存能力分配 */
#include "esp_heap_caps.h"

/* 自定义声音采集 */
#include "xiaozhi_audio.h"

// 1.语音识别AFE声学前端初始化
void xiaozhi_sr_init(void);

#endif /* __XIAOZHI_SR_H__ */
