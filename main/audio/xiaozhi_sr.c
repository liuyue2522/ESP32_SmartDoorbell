#include "xiaozhi_sr.h"

// 唤醒词算法模型句柄
srmodel_list_t *models = NULL;
// 声学前端算法句柄
static esp_afe_sr_iface_t *afe_handle = NULL;
// 声学前端算法语音数据操作句柄
static esp_afe_sr_data_t *afe_data = NULL;

static char *TAG = "xiaozhi_sr";

//-------------------------------------------------------------
// 喂数据任务
void feed_task(void *params);
// 提取识别结果任务
void detect_task(void *params);


//-------------------------------------------------------------
void xiaozhi_sr_init(void)
{
    // 初始化ES8311利用录音功能获取麦克风录制原始音频数据
    xiaozhi_audio_init();

    // 唤醒词检测模型算法初始化
    models = esp_srmodel_init("model");

    // SR组件:AFE声学前端:去噪、检测语音状态【vad】,唤醒词检测
    //"M":表示语音识别的数据来自于麦克风通道,双麦克风"MM"
    // models:AFE声学前端,可以检测换测试,使用模型算法:wakenet9、wakenet9i
    //  AFE_TYPE_SR:场景选择,语音识别场景、 AFE_TYPE_VC 语音通话场景
    // AFE_MODE_HIGH_PERF:高性能,吃内存，吃算力,但是识别度高一些!
    afe_config_t *afe_config = afe_config_init("M", models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);
    // 创建AFE声学前端句柄
    afe_handle = esp_afe_handle_from_config(afe_config);
    // FAE声学前端:音频数据相关进行初始化
    afe_data = afe_handle->create_from_config(afe_config);
    // 添加优化配置,语音识别更准一些
    //   关闭一些硬件上本来就不支持的功能 免得出现反效果
    afe_config->aec_init = false; // 禁用回声消除
    afe_config->se_init = false;  // 禁用人声增强
    afe_config->ns_init = false;  // 禁用噪声消除
    afe_config->wakenet_init = true;
    // 提升了唤醒的灵敏度
    afe_config->wakenet_mode = DET_MODE_90;
    // 环形缓冲区大小
    afe_config->afe_ringbuf_size = 4 * 1024;
    // 更多的内存分配到外部的sram里去
    afe_config->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;
    // 语音状态检测灵敏度设置,风吹草动不能误判人说话!!!!
    afe_config->vad_mode = VAD_MODE_4;

    afe_config_free(afe_config);

    // 1.1任务,给AFE喂数据PCM音频数据
    /* 创建任务： 指定任务所用内存空间是内部内存还是外部内存，还可以指定内核。 */
    xTaskCreatePinnedToCoreWithCaps(feed_task, "feed", 32 * 1024, NULL, 5, NULL, 1, MALLOC_CAP_SPIRAM);

    // 2.提取识别结果任务
    xTaskCreatePinnedToCoreWithCaps(detect_task, "detect", 32 * 1024, NULL, 5, NULL, 1, MALLOC_CAP_SPIRAM);
}

void feed_task(void *params)
{
    // AFE声学前端算法:每一次喂数据的采样点个数:512采样点
    int feed_chunksize = afe_handle->get_feed_chunksize(afe_data);
    ESP_LOGE(TAG, "feed_chunksize:%d", feed_chunksize);
    // AFE声学前端喂数据使用几个通道
    int feed_nch = afe_handle->get_feed_channel_num(afe_data);
    ESP_LOGE(TAG, "feed_nch:%d", feed_nch);
    // 创建一个存储PCM音频数据缓冲区
    int16_t *feed_buff = heap_caps_malloc(feed_chunksize * feed_nch * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    while (1)
    {

        // ES8311获取麦克风录制PCM音频数据
        xiaozhi_audio_record(feed_buff, feed_chunksize * feed_nch * sizeof(int16_t));

        // 将原始PCM数据缓冲区里面音频数据喂给AFE
        afe_handle->feed(afe_data, feed_buff);
    }
}

void detect_task(void *params)
{
    while (1)
    {
        // 持续获取语音识别结果
        afe_fetch_result_t *result = afe_handle->fetch(afe_data);

        // 检测到唤醒词
        if (result->wakeup_state == WAKENET_DETECTED) // wakeup_state 判断是否检测到唤醒词
        {
            // 唤醒词检测成功
            ESP_LOGE(TAG, "唤醒词检测成功+++++++++++++++++++++++++++++++++++++++++++++++++++++");

            // 全局标志位变为1
            xiaozhi_data.wakeup_flag = 1; // SR的AFE声学前端确实检测到唤醒词

            // 当检测到唤醒词以后,唤醒词回调函数执行!别的组件可以得知检测到唤醒词
            if (xiaozhi_data.wakeup_callback != NULL)
            {
                xiaozhi_data.wakeup_callback();
            }
        }

        /*
           1.检测到唤醒词目的,websocket客户端与虾哥服务器建立连接
           2.后续建立连接,想客户端给虾哥服务器发送音频数据,不说你想发就发的！
           3.后续想让虾哥服务器可以接受音频数据、解析音频内容,webscoket客户端给    "开始监听指令"
           4.后续客户端静音,不在给虾哥服务器传递音频数据,webscoket客户端给        "停止监听指令"
           5.SR的声学前端可以检测语音状态变化
                1.静音0->说话1    它发送监听指令
                2.说话1->静音0    它发送停止监听指令
        */
        if (xiaozhi_data.wakeup_flag)
        {
            // 存储当前语音状态
            xiaozhi_data.current_vad_state = result->vad_state; // vad_state 判断是否检测到语音
            // 当前语音状态与上一次语音状态不一样,执行语音状态变化回调
            if (xiaozhi_data.current_vad_state != xiaozhi_data.last_vad_state)
            {
                // 执行语音状态发生变化回调
                if (xiaozhi_data.vad_state_callback != NULL)
                {
                    xiaozhi_data.vad_state_callback();
                }
            }
            // 更新上一次语音状态
            xiaozhi_data.last_vad_state = xiaozhi_data.current_vad_state;
        }
    }
}
