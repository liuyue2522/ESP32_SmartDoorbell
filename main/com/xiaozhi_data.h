#ifndef __XIAOZHI_DATA_H__
#define __XIAOZHI_DATA_H__

// 定义结构体:表示表情
typedef struct
{
    char *emotion; // 表情名称
    char *text;    // 表情文本

} EMOJI_T;


//对外暴露21表情包
extern EMOJI_T emoji_array[21];
#endif /* __XIAOZHI_DATA_H__ */