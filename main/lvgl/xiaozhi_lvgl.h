#ifndef __XIAOZHI_LVGL_H__
#define __XIAOZHI_LVGL_H__

#include "xiaozhi_lcd.h"

//  LVGL 移植层组件
#include "esp_lvgl_port.h"

// 表情字体
#include "font_emoji.h"

// 表情名称和文本对应关系
#include "xiaozhi_data.h"


// 1.LVGL初始化方法
void xiaozhi_lvgl_init(void);


// 2.测试语法
void xiaozhi_lvgl_test(void);


// 3.layout布局
void xiaozhi_lvgl_layout(void);

// 4.更新标题
void xiaozhi_lvgl_update_title(const char *title);
// 5.更新表情
void xiaozhi_lvgl_update_emoji(const char *face);
// 6.更新对话
void xiaozhi_lvgl_update_dialogue(const char *dialog); // 直接显示


// 7.展示二维码
void xiaozhi_lvgl_show_qrcode(const char *qrcode);


// 8.删除二维码
void xiaozhi_lvgl_del_qrcode(void);


// ----------------------------------------------

// 对话框流式显示
void xiaozhi_lvgl_update_dialogue_stream(const char *text);

// 启动对象闪烁动画
void xiaozhi_lvgl_start_blink(lv_obj_t *obj);

// 停止对象闪烁并恢复完全不透明
void xiaozhi_lvgl_stop_blink(lv_obj_t *obj);

// -----------------------------------------------



#endif /* __XIAOZHI_LVGL_H__ */