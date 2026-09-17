#include "xiaozhi_decoder.h"

// opus解码器句柄
esp_audio_dec_handle_t decoder;

// 解码器任务回调
void decoder_task(void *params);

// 解码器初始化
void xiaozhi_decoder_init(void)
{
    // 1.解码器配置参数
    esp_opus_dec_cfg_t opuc_cfg = {
        .sample_rate = ESP_AUDIO_SAMPLE_RATE_16K,            // 解码器解码数据采样率
        .channel = ESP_AUDIO_MONO,                           // 解码音频数据单声道
        .frame_duration = ESP_OPUS_DEC_FRAME_DURATION_60_MS, // 解码音频数据帧长
        .self_delimited = false,                             // 数据包是否自带长度信息
    };
    //     用大白话说就是“这包数据有没有自带尺子”。
    // • false：数据包不带长度信息，解码器靠你传入的 len 来判断包大小（最常用，配合 RTP/Ogg 等协议）。
    // • true：数据包自带长度前缀，解码器可自行从连续流中切出完整包。

    // 2.开启OPUS解码器
    esp_opus_dec_open(&opuc_cfg, sizeof(opuc_cfg), &decoder);

    // 3.开启解码任务
    xTaskCreatePinnedToCoreWithCaps(decoder_task, "decoder", 32 * 1024, NULL, 5, NULL, 1, MALLOC_CAP_SPIRAM);
}

void decoder_task(void *params)
{
    // 解码器需要存储:要进行解码音频数据信息
    esp_audio_dec_in_raw_t opus_frame = {
        .buffer = NULL, // 解码器需要解码数据地址
        .len = 0,       // 解码器需要待解码数据长度个数
    };

    // 解码器解码完成输出音频数据信息
    esp_audio_dec_out_frame_t out_frame = {
        .buffer = heap_caps_malloc(8 * 1024, MALLOC_CAP_SPIRAM), // 用于存储解码完成数据PCM
        .len = 8 * 1024                                          // 存储解码完成这个缓冲区长度
    };

    // 存储解码器给解码信息
    esp_audio_dec_info_t aud_info;

    while (1)
    {
        // 从缓冲区提取需要解码数据
        /* xRingbufferReceive 一次只能取一个数据项，而数据项的大小由发送时决定 */
        uint8_t *decoder_data = xRingbufferReceive(xiaozhi_data.ws_to_decoder_handle, &opus_frame.len, portMAX_DELAY);
        // 解码数据给解码器
        opus_frame.buffer = decoder_data;

        // 解码数据:解码这个动作不是一次全部解码完成
        while (opus_frame.len > 0)
        {
            // 每执行一次解码部分数据:  解码器每次调用只消耗一个完整的 Opus 包，并返回一个解码后的 PCM 帧数据
            esp_opus_dec_decode(decoder, &opus_frame, &out_frame, &aud_info);

            // 更新还剩下多少数据还未解码
            opus_frame.len -= opus_frame.consumed; // consumed:已经解码数据长度个数
            // 更新解码数据地址
            opus_frame.buffer += opus_frame.consumed;

            //解码完成用喇叭播放出来
            xiaozhi_audio_play(out_frame.buffer, out_frame.decoded_size);
        }
        // 缓冲区用完以后释放数据
        vRingbufferReturnItem(xiaozhi_data.ws_to_decoder_handle, decoder_data);
    }
}