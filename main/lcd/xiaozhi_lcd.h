#ifndef __XIAOZHI_LCD_H__
#define __XIAOZHI_LCD_H__
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "jpeg_decoder.h"
#include "string.h"

// MCU将来初始化SPI片上外设选择:SPI2
#define LCD_HOST SPI2_HOST
// 每一次刷新数据的行数:这个数值必须能被240整除
#define PARALLEL_LINES 4
#define ROTATE_FRAME 30
// 液晶显示屏工作时钟频率:20MHZ
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)
// 高电平打开背光灯
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL 1
// 低电平关闭背光灯
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
// SPI协议的IO引脚指:MOIS编号
#define EXAMPLE_PIN_NUM_DATA0 48
// SPI协议的时钟线引脚
#define EXAMPLE_PIN_NUM_PCLK 47
// SPI的片选引脚
#define EXAMPLE_PIN_NUM_CS 21
// D/C引脚编号
#define EXAMPLE_PIN_NUM_DC 45
// 控制LCD的复位引脚
#define EXAMPLE_PIN_NUM_RST 16
// 控制背光源的IO引脚编号
#define EXAMPLE_PIN_NUM_BK_LIGHT 40
// LCD屏幕分辨率
#define EXAMPLE_LCD_H_RES 320
#define EXAMPLE_LCD_V_RES 240

// 数据或者命令八位
#define EXAMPLE_LCD_CMD_BITS 8
#define EXAMPLE_LCD_PARAM_BITS 8

// 1.LCD初始化方法
void xiaozhi_lcd_init(void);



//2.获取图像的数据【二进制】
void xiaozhi_lcd_get_image_data(void);



//3.显示图像数据
void xiaozhi_lcd_show_image(void);
#endif /* __XIAOZHI_LCD_H__ */