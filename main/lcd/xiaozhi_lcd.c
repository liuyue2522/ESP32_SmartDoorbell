#include "xiaozhi_lcd.h"
// 双缓冲区
/* static uint16_t *s_lines[2]; */
// 存储整张图像图像的数据
/* uint8_t *image_buffers = NULL; */
// 3.LCD的IO句柄
esp_lcd_panel_io_handle_t io_handle = NULL;
// 面板句柄
esp_lcd_panel_handle_t panel_handle = NULL;
// 获取文件的起始与结束地址
// extern const uint8_t image_jpg_start[] asm("_binary_image_jpg_start");
// extern const uint8_t image_jpg_end[] asm("_binary_image_jpg_end");
/* #define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
extern const uint8_t image_jpg_start[] asm(TOSTRING(IMAGE_START_SYMBOL));
extern const uint8_t image_jpg_end[]   asm(TOSTRING(IMAGE_END_SYMBOL)); */


// 1.LCD初始化方法
void xiaozhi_lcd_init(void)
{
    // 1.初始化GPIO->P40引脚,控制LCD的背光源
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT};
    gpio_config(&bk_gpio_config);

    // 2.初始化SPI
    spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_PIN_NUM_PCLK,  // SPI2的时钟线P47
        .mosi_io_num = EXAMPLE_PIN_NUM_DATA0, // MOSI引脚
        .miso_io_num = -1,                    // MISO引脚,根本没有使用
        .quadwp_io_num = -1,                  // 四线SPI写法
        .quadhd_io_num = -1,
        .max_transfer_sz = PARALLEL_LINES * EXAMPLE_LCD_H_RES * 2 + 8 // SPI协议每一次最多传输数据最大值(字节)
    };
    // 初始化SPI
    spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = EXAMPLE_PIN_NUM_DC,     // P45可以控制给LCD通过SPI发送过去的是数据,还是命令
        .cs_gpio_num = EXAMPLE_PIN_NUM_CS,     // P21控制片选引脚
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ, // LCD工作频率
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,  // LCD接收到数据、命令都是八位一字节
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .spi_mode = 0,           // SPI工作模式:选择模式0
        .trans_queue_depth = 10, // trans_queue_depth = SPI 事务排队缓冲区的大小，决定了在事务被真正发送出去之前，最多能积压多少个待发送请求
    };

    // SPI控制引脚:CS与DC与LCD进行关联
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle);

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,      // P16控制LCD复位
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB, // 颜色模式:RGB
        .bits_per_pixel = 16,                       // RGB采用16位表示颜色
    };

    // ST7789中景园芯片开始初始化
    esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle);
    // 背光灯关闭
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL);
    // 复位
    esp_lcd_panel_reset(panel_handle);
    // 面板初始化操作
    esp_lcd_panel_init(panel_handle);
    // 面板开启
    esp_lcd_panel_disp_on_off(panel_handle, true);
    // 是否交换X轴与Y轴
    esp_lcd_panel_swap_xy(panel_handle, true);
    esp_lcd_panel_mirror(panel_handle, true, false);
    // 背光灯开启
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);

    /* // LCD需要双缓冲区
    for (uint8_t i = 0; i < 2; i++)
    {
        //开辟空间使用外部PSRAM的
        s_lines[i] = heap_caps_malloc(EXAMPLE_LCD_H_RES * PARALLEL_LINES * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    } */
}

/* // 获取图像数据
void xiaozhi_lcd_get_image_data(void)
{

    // 先准备存储图像连续空间
    // STM32 + freeRTOS,用亚马逊pvMalloc
    // ESP32 + FreeRTOS,用ESP32的heap_caps_malloc + heap_caps_calloc + heap_caps_ralloc + heap_caps_free
    image_buffers = heap_caps_calloc(EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES, sizeof(uint16_t), MALLOC_CAP_SPIRAM);

    // 图像解码需要参数
    esp_jpeg_image_cfg_t jpeg_cfg = {
        .indata = (uint8_t *)image_jpg_start,                                    // 图片起始地址
        .indata_size = image_jpg_end - image_jpg_start,                          // 计算图像大小
        .outbuf = image_buffers,                                                 // 存储图像数据的缓冲区
        .outbuf_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * sizeof(uint16_t), // 缓冲区大小
        .out_format = JPEG_IMAGE_FORMAT_RGB565,                                  // 图像二进制数据格式RGB[565]
        .out_scale = JPEG_IMAGE_SCALE_0,                                         // 图形生成数据不需要缩放
        .flags = {
            .swap_color_bytes = 1, // RGB->565 高低位字节位置进行交换
        }};

    // 结构体变量:decode解码器解析图像完成以后,将原始图像信息保存outimg变量当中
    esp_jpeg_image_output_t outimg;
    // 将图像数据进行解码,数据放到 image_buffers缓冲区里面
    esp_jpeg_decode(&jpeg_cfg, &outimg);
}

// 显示图像
void xiaozhi_lcd_show_image(void)
{
    // 当前发送过去的需要渲染缓冲区数据
    int sending_line = 0;
    // 计算下一行需要展示图像数据
    int calc_line = 0;
    uint16_t copy_byte_num = EXAMPLE_LCD_H_RES * PARALLEL_LINES * sizeof(uint16_t);

    // 渲染60次整张图像
    uint8_t *image_buffers_offset = image_buffers; // 图像数据偏移量
    for (int y = 0; y < EXAMPLE_LCD_V_RES; y += PARALLEL_LINES)
    {
        memcpy(s_lines[calc_line], image_buffers_offset, copy_byte_num);
        // 图像下四行数据
        image_buffers_offset += copy_byte_num;
        sending_line = calc_line;
        calc_line = !calc_line;
        // 渲染图像数据
        esp_lcd_panel_draw_bitmap(panel_handle, 0, y, 0 + EXAMPLE_LCD_H_RES, y + PARALLEL_LINES, s_lines[sending_line]);
    }
} */