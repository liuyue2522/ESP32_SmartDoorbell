#include "xiaozhi_encoder.h"

// 编码器句柄
esp_audio_enc_handle_t encoder = NULL;

// 每一次编码PCM音频字节个数、   每一次编码opus音频字节个数
int pcm_size = 0,              opus_size = 0;
// 编码器PCM与OPUS音频数据缓冲区
uint8_t *pcm_buffer = NULL, *opus_buffer = NULL;

static char *TAG = "ENCODER";


// 编码器:编码任务
void encoder_task(void *params);


void xiaozhi_encoder_init(void)
{

    // 1.初始化OPUS编码器参数
    esp_opus_enc_config_t opus_cfg = {
        .sample_rate = ESP_AUDIO_SAMPLE_RATE_16K,            // 编码原始数据PCM采样16KHZ
        .channel = ESP_AUDIO_MONO,                           // 音频都是单声道
        .bits_per_sample = ESP_AUDIO_BIT16,                  // 原始PCM数据的位深16
        .bitrate = 36000,                                    // 编码器编码输出比特率设置
        .frame_duration = ESP_OPUS_ENC_FRAME_DURATION_60_MS, // 编码PCM数据帧长度60ms
        .application_mode = ESP_OPUS_ENC_APPLICATION_AUDIO,  // 编码器使用场景
        .complexity = 5,                                     // 编码器进行编码复杂程度【0-10】,数值越大越吃算力!
        .enable_fec = false,                                 // 编码器编码音频数据时候,是否备份!网络确实差可以开启
        .enable_dtx = false,                                 // 不管当前是有人说话、还是安静得掉根针都能听见，编码器都老老实实每 60ms 发一个正常的 Opus 包，一个都不少。
        .enable_vbr = false,                                 // 可变比特率不需要:36000
    };

    // 2.开启OPUS编码器
    esp_opus_enc_open(&opus_cfg, sizeof(esp_opus_enc_config_t), &encoder);

    // 3.根据顶部参数:咱们可以获取到需要 PCM 帧 原始音频数据字节个数、编码完成opus音频字节个数
    esp_opus_enc_get_frame_size(encoder, &pcm_size, &opus_size);
    ESP_LOGE(TAG, "PCM原始音频数据字节个数:%d, 编码完成opus音频字节个数:%d", pcm_size, opus_size);


    // 4.编码器需要创建 PCM 与 OPUS 音频数据缓冲区
    pcm_buffer = heap_caps_malloc(pcm_size, MALLOC_CAP_SPIRAM);
    opus_buffer = heap_caps_malloc(opus_size, MALLOC_CAP_SPIRAM);

    // 5.创建编码任务,将PCM原始音频数据编码为opus音频数据
    // 1.ESP32S3双核:WIFI、BLE、LVGL、HTTP等默认任务,默认都是放在core0
    // 2.处理音频数据【60ms->1920字节】
    xTaskCreatePinnedToCoreWithCaps(encoder_task, "encoder", 32 * 1024, NULL, 3, NULL, 1, MALLOC_CAP_SPIRAM);
}


void encoder_task(void *params)
{
    // 输入数据帧信息:PCM原始音频数据
    // 每一次:都是一个60msPCM数据帧进行编码
    esp_audio_enc_in_frame_t pcm_frame = {
        .buffer = pcm_buffer, // buffer: 存储 1 个数据帧【1920 字节】,每一次找到要进行编码数据帧首个字节地址
        .len = pcm_size,      // 每一次处理PCM音频数据字节个数
    };

    // 输出的数据帧信息:OPUS编码后的数据
    esp_audio_enc_out_frame_t opus_frame = {
        .buffer = opus_buffer, // 指向编码缓冲区首地址
        .len = opus_size,      // 编码缓冲区的大小, encoded_bytes成员是实际编码出来音频数据字节个数
    };
    while (1)
    {
        // 服务器需要帧长:60ms,1920字节,每一次需要从环形缓冲区提取 PCM 1920字节,编码出 opus 370字节压缩音频数据
        size_t hope_size = pcm_size; // 期望从缓冲区提取出来的字节个数1920
        // 存储 PCM 原始音频数据缓冲区空间
        uint8_t *encoder_ptr = pcm_buffer;

        while (hope_size > 0)
        {
            size_t receive_len = 0; // 实际从缓冲区提取出来的字节个数
            uint8_t *data = xRingbufferReceiveUpTo(xiaozhi_data.sr_to_encoder_handle, &receive_len, portMAX_DELAY, hope_size);   // 最多只读剩余需要的字节数，绝不越界
            if (data == NULL) {
                ESP_LOGE(TAG, "环形缓冲区接收失败");
                break;
            }

            memcpy(encoder_ptr, data, receive_len);
            // 更新指针地址
            encoder_ptr += receive_len;
            // 更新期望获取字节个数
            hope_size -= receive_len;
            // 每一次提取数据,需要通知缓冲区进行释放
            vRingbufferReturnItem(xiaozhi_data.sr_to_encoder_handle, data);
        }

        // 进行PCM数据转OPUS进行编码
        esp_opus_enc_process(encoder, &pcm_frame, &opus_frame);

       // 将编码器压缩opus音频数据添加到缓冲区当中
       // 1.缓冲区 2.向缓冲区添加数据 3.添加数据长度 4.阻塞时间
       // opus_frame:len,表示缓冲区大小  .encoded_bytes编码器实际编码出来opus字节个数
       xRingbufferSend(xiaozhi_data.encoder_to_ws_handle, opus_frame.buffer, opus_frame.encoded_bytes, portMAX_DELAY);

        // 打印当前任务堆栈使用情况
        // ESP_LOGI(TAG, "encoder stack high water mark: %u", uxTaskGetStackHighWaterMark(NULL));

        // 编码完成后，主动让出 CPU，避免独占
        vTaskDelay(1);  // 至少延时 1 个 tick，避免 CPU 占用率过高
    }
}
