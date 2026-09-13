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
    screen = lv_screen_active(); // 获取当前活跃的屏幕      lv_obj_create(NULL);  // 如果父对象为 NULL → 本质是在创建屏幕
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





// -----------------------------------流式文字（打字机效果）-----------------------------------------------
// 缓冲区大小
#define TYPEWRITER_BUF_SIZE 256

// 打字机：保存完整文本
static char dialog_full_text[TYPEWRITER_BUF_SIZE] = {0};

// 1.打字机：执行回调函数。   每一帧的回调：只显示前 N 个字符
static void typewriter_exec_cb(void *var, int32_t value)
{
    lv_obj_t *label = (lv_obj_t *)var;
    int len = strlen(dialog_full_text);

    if (value < 0)   value = 0;
    if (value > len) value = len;

    // 用前 value 个字符拼出子串
    char buf[TYPEWRITER_BUF_SIZE];
    memcpy(buf, dialog_full_text, value);
    buf[value] = '\0'; // 在末尾结束符

    lv_label_set_text(label, buf);
}

// 2.启动动画的接口
void xiaozhi_lvgl_update_dialogue_stream(const char *text)
{
    lvgl_port_lock(0);

    // 保存完整文本
    strncpy(dialog_full_text, text, sizeof(dialog_full_text) - 1);
    dialog_full_text[sizeof(dialog_full_text) - 1] = '\0'; // 在末尾结束符

    int len = strlen(dialog_full_text);

    // 先删掉上一次可能还在跑的动画，避免叠加
    lv_anim_delete(dialog, typewriter_exec_cb); // 精确删除该对象上由这个回调驱动的动画，不影响对象上的其他动画。

    if (len == 0) {
        lv_label_set_text(dialog, "");
        lvgl_port_unlock();
        return;
    }

    // 配置动画：从 0 到 len，每个字 50ms
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, dialog); // 动画作用对象
    lv_anim_set_values(&a, 0, len); // 起止值
    lv_anim_set_duration(&a, len * 50); // 时长
    lv_anim_set_exec_cb(&a, typewriter_exec_cb); // 每一帧回调，把当前值写到对象上
    lv_anim_set_path_cb(&a, lv_anim_path_linear);  // 匀速，不要缓动
    lv_anim_start(&a); // 启动动画

    lvgl_port_unlock();
}


// --------------------------------------闪烁效果----------------------------------------------

/**
 * @brief 闪烁动画的每一帧回调函数
 *
 * 该函数由 LVGL 动画系统在每一帧调用，用于根据当前动画值设置对象的透明度。
 *
 * @param var   动画作用的对象指针（实际传入的是 lv_obj_t *）
 * @param value 当前动画值，取值范围由 lv_anim_set_values 决定，
 *              这里表示透明度（0~255，LV_OPA_COVER 为完全不透明）
 */
static void blink_exec_cb(void *var, int32_t value)
{
    // 设置对象的不透明度
    // 第三个参数 0 表示默认状态（LV_STATE_DEFAULT）
    lv_obj_set_style_opa((lv_obj_t *)var, value, 0);
}


/**
 * @brief 启动对象闪烁动画
 *
 * 让指定对象在完全不透明和半透明之间循环切换，常用于提示连接失败、警告等场景。
 * 动画为无限循环，使用缓入缓出曲线，视觉效果更柔和。
 *
 * @param obj 需要闪烁的 LVGL 对象（如 label、button 等）
 *
 * @note 该函数内部使用 lvgl_port_lock 加锁，可在非 LVGL 任务中安全调用。
 *       若同一对象已有相同回调的动画，会先删除再重新创建，避免动画叠加。
 */
void xiaozhi_lvgl_start_blink(lv_obj_t *obj)
{
    // 进入 LVGL 临界区，保证线程安全
    lvgl_port_lock(0);

    // 先清掉可能已有的闪烁动画，防止重复启动导致多个动画同时修改透明度
    lv_anim_delete(obj, blink_exec_cb);

    // 初始化动画结构体
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj); // 设置动画作用对象
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_20); // 设置动画起止值：从完全不透明 (255) 到 20% 不透明 (51)
    lv_anim_set_duration(&a, 300); // 正向播放时长 300ms
    lv_anim_set_playback_duration(&a, 300); // 反向回弹时长 300ms（即从 20% 再回到 100%）
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE); // 无限循环播放（32位最大值，并不是真正意义上的无限）
    lv_anim_set_exec_cb(&a, blink_exec_cb); // 设置每一帧的回调函数
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out); // 设置动画路径为缓入缓出，使闪烁更自然
    lv_anim_start(&a); // 启动动画

    // 退出 LVGL 临界区
    lvgl_port_unlock();
}

/**
 * @brief 停止对象闪烁并恢复完全不透明
 *
 * 删除由 blink_exec_cb 驱动的动画，并将对象透明度恢复为完全不透明。
 * 通常在连接成功、警告解除等场景下调用。
 *
 * @param obj 需要停止闪烁的 LVGL 对象
 *
 * @note 该函数内部使用 lvgl_port_lock 加锁，可在非 LVGL 任务中安全调用。
 */
void xiaozhi_lvgl_stop_blink(lv_obj_t *obj)
{
    // 进入 LVGL 临界区
    lvgl_port_lock(0);

    // 删除该对象上由 blink_exec_cb 驱动的动画
    lv_anim_delete(obj, blink_exec_cb);

    lv_obj_set_style_opa(obj, LV_OPA_COVER, 0);   // 恢复完全不透明

    // 退出 LVGL 临界区
    lvgl_port_unlock();
}

// ------------------------------------------------------------------------------------

