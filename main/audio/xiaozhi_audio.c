#include "xiaozhi_audio.h"

// I2C句柄
static i2c_master_bus_handle_t i2c_bus_handle;
// ES8311编解码器句柄
esp_codec_dev_handle_t codec_dev = NULL;

// I2S收发数据句柄
i2s_chan_handle_t tx_handle;
i2s_chan_handle_t rx_handle;

// -----------------------------------------------------------------------

static void xiaozhi_audio_i2c_init(void)
{
    i2c_master_bus_config_t i2c_bus_config = {0};
    // I2C时钟源
    i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    // I2C_0,ESP32S3拥有两个I2C
    i2c_bus_config.i2c_port = I2C_NUM_0;
    // I2C的时钟线对应IO引脚
    i2c_bus_config.scl_io_num = GPIO_NUM_1;
    // I2C的数据线对应IO引脚
    i2c_bus_config.sda_io_num = GPIO_NUM_0;
    // I2C一般开漏,需要上拉电阻
    i2c_bus_config.flags.enable_internal_pullup = true;
    // glitch_ignore_cnt 用于配置 I2C 总线硬件滤波器，过滤掉总线上持续时间短于设定值的尖峰脉冲干扰，防止毛刺被误识别为正常的时钟或数据信号。
    i2c_bus_config.glitch_ignore_cnt = 7;
    // 初始化I2C
    i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle);
}

// 初始化I2S:传输音频数据
static void xiaozhi_audio_i2s_init(void)
{
    // I2S协议通道相关配置
    // I2S0,通信作为主设备
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    // I2S通信需要用到的GPIO引脚
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),                            // I2S音频采样频率16KHZ
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(16, I2S_SLOT_MODE_MONO), // 采样位深16bit,单声道
        .gpio_cfg = {
            .mclk = GPIO_NUM_3,
            .bclk = GPIO_NUM_2,
            .ws = GPIO_NUM_5,
            .dout = GPIO_NUM_6,
            .din = GPIO_NUM_4,
        },
    };
    // I2S收发配置
    i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle);
    // I2S按照:五个引脚,采样率16KZ,位深16,收发数据
    i2s_channel_init_std_mode(tx_handle, &std_cfg);
    i2s_channel_init_std_mode(rx_handle, &std_cfg);
    // I2S收发使能
    i2s_channel_enable(tx_handle);
    i2s_channel_enable(rx_handle);
}


//------------------------------------------------------------------------

// 1.编解码器初始化
void xiaozhi_audio_init(void)
{
    // 1.初始化I2C与I2S
    xiaozhi_audio_i2c_init();
    xiaozhi_audio_i2s_init();


    // 2.为编解码器设备实现 控制接口，数据接口和 GPIO接口 (使用默认提供的接口实现)
    // 编解码器控制通信使用I2C
    audio_codec_i2c_cfg_t i2c_cfg = {
        .addr = ES8311_CODEC_DEFAULT_ADDR, // 从机地址：根据 ES8311 数据手册，芯片的 7 位 I2C 地址格式固定为 0001 100x，其中最后一位 x 直接对应 CE 引脚的电平：0 表示 0x18，1 表示 0x19。
        .bus_handle = i2c_bus_handle,      // I2C句柄,ESP_IDF版本大于5.3都需要指定!
    };
    const audio_codec_ctrl_if_t *out_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    // GPIO接口
    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();

    // 3.基于控制接口和 ES8311 特有的配置实现
    es8311_codec_cfg_t es8311_cfg = {
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH, // ADC与DAC都用,   录制与播放声音
        .ctrl_if = out_ctrl_if,
        .gpio_if = gpio_if,
        .pa_pin = GPIO_NUM_7, // 主控通过P7控制外放使能
        .use_mclk = true,
    };
    const audio_codec_if_t *out_codec_if = es8311_codec_new(&es8311_cfg);

    // I2S与编解码器数据通信使用I2S
    audio_codec_i2s_cfg_t i2s_cfg = {
        .rx_handle = rx_handle,
        .tx_handle = tx_handle};
    const audio_codec_data_if_t *data_if = audio_codec_new_i2s_data(&i2s_cfg);

    // 4.其他初始化配置
    // 获取ES8311编解码器句柄
    esp_codec_dev_cfg_t dev_cfg = {
        .codec_if = out_codec_if,              // es8311_codec_new 获取到的接口实现
        .data_if = data_if,                    // audio_codec_new_i2s_data 获取到的数据接口实现
        .dev_type = ESP_CODEC_DEV_TYPE_IN_OUT, // 设备同时支持录制和播放
    };
    // 编码器句柄:录制声音、播放声音需要
    codec_dev = esp_codec_dev_new(&dev_cfg);

    // 播放音频相关参数配置
    esp_codec_dev_set_out_vol(codec_dev, 30.0); // 播放音量大小: 音量百分比（0~100），不是 dB
    esp_codec_dev_sample_info_t fs = {
        // 播放音频数据参数要求
        .sample_rate = 16000,  // 采样率16KHZ
        .channel = 1,          // 单声道
        .bits_per_sample = 16, // 位深16
    };
    esp_codec_dev_open(codec_dev, &fs);

    // 这行代码的作用是设置 ES8311 的输入增益（录音增益），也就是调整麦克风信号在进入 ADC 之前的放大倍数。
    /* 30.0 输入增益值，单位是 dB（分贝）。这里表示将输入信号放大 30 dB。 */
    esp_codec_dev_set_in_gain(codec_dev, 30.0); // 30 dB 输入增益
}

// 播放音频
void xiaozhi_audio_play(void *buf, int len)
{
    // 播放音频
    esp_codec_dev_write(codec_dev, buf, len);
}

// 录制声音
void xiaozhi_audio_record(void *buf, int len)
{
    esp_codec_dev_read(codec_dev, buf, len);
}

//------------------------------------------------------------------------

