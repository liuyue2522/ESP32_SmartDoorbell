#include "xiaozhi_lvgl.h"
// 3.LCD的IO句柄
extern esp_lcd_panel_io_handle_t io_handle;
// 面板句柄
extern esp_lcd_panel_handle_t panel_handle;

// LVGL句柄
lv_display_t *lvgl_disp;
// 1.LVGL初始化并且与LCD进行关联
void xiaozhi_lvgl_init(void)
{

    // 1.LCD液晶显示屏初始化
    xiaozhi_lcd_init();

    // 2.LVGL初始化创建任务  + LVGL与LCD关联
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,       // LVGL底层任务刷新数据任务优先级
        .task_stack = 4096 * 3,   // lvgl任务的栈的大小
        .task_affinity = 0,       // 指定哪一个内核执行任务-1,没有指定内核!
        .task_max_sleep_ms = 500, // 任务阻塞时间最长500ms
        .timer_period_ms = 5      // 5ms刷新一次数据,1000ms->刷新200次,200HZ
    };
    // lvgl初始化:创建任务
    lvgl_port_init(&lvgl_cfg);

    // 2.lvgl与LCD液晶显示屏进行关联
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,                                               // LCD液晶显示屏IO句柄
        .panel_handle = panel_handle,                                         // LCD面板句柄
        .buffer_size = EXAMPLE_LCD_H_RES * PARALLEL_LINES * sizeof(uint16_t), // 指定缓冲区大小[LVGL底层代码:s_lines[2]
        .double_buffer = false,                                               // 双缓冲区
        .hres = EXAMPLE_LCD_H_RES,                                            // 屏幕尺寸大小
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = false,                    // 屏幕上一个像素点是否用一位数据表示
        .color_format = LV_COLOR_FORMAT_RGB565, // LCD屏幕上像素点需要数据格式:565
        .rotation = {
            .swap_xy = true, // 镜像的设置
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,   // 使用DMA
            .swap_bytes = true, // 大端小端

        }};
    // lvgl与LCD进行关联
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);
}

/* void xiaozhi_lvgl_test(void)
{

    // 1.先获取到屏幕【根组件】
    lv_obj_t *screen = lv_screen_active();
    // 设置屏幕背景颜色:0x003a57
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x003a57), 0);
    // 设置屏幕文字颜色
    lv_obj_set_style_text_color(screen, lv_color_hex(0xffffff), 0);

    // 2想在屏幕上添加一个文本
    lv_obj_t *label1 = lv_label_create(screen);
    // 设置文本内容
    lv_label_set_text(label1, "Hello World!");
    lv_obj_set_align(label1, LV_ALIGN_CENTER);

    lv_obj_t *btn = lv_button_create(screen);
    lv_obj_set_pos(btn, 10, 10);
    lv_obj_set_size(btn, 120, 50);

    lv_obj_t *btn_title = lv_label_create(btn);
    lv_label_set_text(btn_title, "123");
} */
void xiaozhi_lvgl_test(void)
{
    lv_obj_t * screen = lv_screen_active();
    /* 💡 Tap a day to select it; use the header arrows to jump between months. */
    lv_obj_t * calendar = lv_calendar_create(screen);
    lv_obj_set_size(calendar, 300, 230);
    lv_obj_set_align(calendar, LV_ALIGN_CENTER);
    lv_calendar_set_today_year(calendar, 2026);
    lv_calendar_set_today_month(calendar, 5);
    lv_calendar_set_today_day(calendar, 15);
    lv_calendar_set_shown_year(calendar, 2026);
    lv_calendar_set_shown_month(calendar, 5);
    lv_calendar_add_header_arrow(calendar);
}
