#include "xiaozhi_lvgl.h"

// 1.LCD的IO句柄
extern esp_lcd_panel_io_handle_t io_handle;
// 2.面板句柄
extern esp_lcd_panel_handle_t panel_handle;

// 3.LVGL句柄
lv_display_t *lvgl_disp;

// 4.字体句柄
/* 
字体变量	        字体家族	        字号	位深 (bpp)
font_puhui_14_1	    阿里巴巴普惠体	    14px	1 (点阵/二值)
font_puhui_16_4	    阿里巴巴普惠体	    16px	4 (16级灰度)
font_puhui_20_4	    阿里巴巴普惠体	    20px	4 (16级灰度)
font_puhui_30_4	    阿里巴巴普惠体	    30px	4 (16级灰度)
*/
// 引入中文字体 14
extern lv_font_t font_puhui_14_1;
extern lv_font_t font_puhui_16_4;
extern lv_font_t font_puhui_20_4;
extern lv_font_t font_puhui_30_4;

// 5.屏幕/根 组件
lv_obj_t *screen = NULL;
// 6.二维码组件
lv_obj_t *qrcode = NULL;
// 7.标题组件
lv_obj_t *title = NULL;
// 8.表情组件
lv_obj_t *emoji = NULL;
// 9.对话组件
lv_obj_t *dialog = NULL;

// 10.二维码颜色
#define QR_DARK lv_color_hex(0xffffff)
// 11.二维码背景颜色
#define QR_LIGHT lv_color_hex(0x000000)

// 1.LVGL初始化并且与LCD进行关联
void xiaozhi_lvgl_init(void) {

    // 1.LCD液晶显示屏初始化
    xiaozhi_lcd_init();

    // 2.LVGL初始化创建任务  + LVGL与LCD关联
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,       // LVGL底层任务刷新数据任务优先级
        .task_stack = 4096 * 3,   // lvgl任务的栈的大小
        .task_affinity = 0,       // 绑定到 CPU0（0 表示 CPU0，-1 表示不绑定）
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


// LCD布局
void xiaozhi_lvgl_layout(void)
{
    // 进入临界区
    lvgl_port_lock(0);

    // 1.根组件
    screen = lv_screen_active();
    // 设置屏幕背景颜色
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x003a57), LV_PART_MAIN);

    // 2.标题部分
    title = lv_label_create(screen);
    // 标题内容
    lv_label_set_text(title, "");
    // 更新标题颜色
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    // 设置标题的位置
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);
    // 修改标题的字体   默认:Montserrat 阿根廷不支持汉字     选择:小智【虾哥】提供字体库
    lv_obj_set_style_text_font(title, &font_puhui_16_4, 0);

    // 3.中间的表情部分
    emoji = lv_label_create(screen);
    // 表情内容
    lv_label_set_text(emoji, "");
    lv_obj_set_style_text_color(emoji, lv_color_hex(0xffffff), 0);
    // 位置
    lv_obj_set_align(emoji, LV_ALIGN_CENTER);
    //设置表情包字体
    lv_obj_set_style_text_font(emoji,font_emoji_64_init() , 0);

    // 4.底部的对话内容
    dialog = lv_label_create(screen);
    // 对话内容
    lv_label_set_text(dialog, "");
    lv_obj_set_style_text_color(dialog, lv_color_hex(0xffffff), 0);
    // 位置
    lv_obj_set_align(dialog, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_style_text_font(dialog, &font_puhui_14_1, 0);

    // 退出临界区
    lvgl_port_unlock();
}
// 6.更新标题的内容
void xiaozhi_lvgl_update_title(const char *newTitle)
{
    // 临界区
    lvgl_port_lock(0);
    // 更新标题内容
    lv_label_set_text(title, newTitle);
    // 退出临界区
    lvgl_port_unlock();
}
// 7.更新表情内容
void xiaozhi_lvgl_update_emoji(const char *newEmoji)
{
    // 临界区
    lvgl_port_lock(0);
    // 更新标题内容
    for (uint8_t i = 0; i < 21; i++)
    {
        if (strcmp(newEmoji, emoji_array[i].emotion) == 0)
        {
            // 更新label展示内容
            lv_label_set_text(emoji, emoji_array[i].text);
            break;
        }
    }
    // 退出临界区
    lvgl_port_unlock();
}
// 8.更新对话内容
void xiaozhi_lvgl_update_dialogue(const char *newDialogue)
{
    // 临界区
    lvgl_port_lock(0);
    // 更新标题内容
    lv_label_set_text(dialog, newDialogue);
    // 退出临界区
    lvgl_port_unlock();
}

// 展示二维码
void xiaozhi_lvgl_show_qrcode(const char *info)
{

    // 上锁:进入临界区
    lvgl_port_lock(0);

    // 2.屏幕下二维码组件
    qrcode = lv_qrcode_create(screen);
    // 尺寸大小
    lv_qrcode_set_size(qrcode, 150);
    // 二维码颜色设置
    lv_qrcode_set_dark_color(qrcode, QR_DARK);
    lv_qrcode_set_light_color(qrcode, QR_LIGHT);
    // 二维码展示内容
    lv_qrcode_set_data(qrcode, info);
    // 这个静区就是二维码四周的一圈空白边缘，它不编码任何数据，但对扫码识别至关重要
    lv_qrcode_set_quiet_zone(qrcode, true);
    // 二维码边框的颜色
    lv_obj_set_style_border_color(qrcode, QR_DARK, 0);
    // 二维码边框大小
    lv_obj_set_style_border_width(qrcode, 4, 0);

    // 居中显示
    lv_obj_center(qrcode);

    // 退出临界区
    lvgl_port_unlock();
}

// 删除二维码组件
void xiaozhi_lvgl_del_qrcode(void)
{
    lvgl_port_lock(0);
    lv_obj_delete(qrcode);
    lvgl_port_unlock();
}

