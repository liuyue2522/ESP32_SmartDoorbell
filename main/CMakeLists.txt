# 这里指定你要嵌入的图片文件名
set(IMAGE_FILE "1.jpg")

# 根据文件名生成符号名：把 . 和 / 替换成 _
string(REPLACE "." "_" IMAGE_SYMBOL ${IMAGE_FILE})
string(REPLACE "/" "_" IMAGE_SYMBOL ${IMAGE_SYMBOL})



idf_component_register(
    SRCS
        "main.c"
        "./button/xiaozhi_button.c"
        "./wifi_sta/xiaozhi_wifi_sta.c"
        "./lcd/xiaozhi_lcd.c"
        "./lvgl/xiaozhi_lvgl.c"
        "./com/xiaozhi_data.c"
        "./http/xiaozhi_http_client.c"
        "./audio/xiaozhi_audio.c"
        "./audio/xiaozhi_sr.c"
        "./audio/xiaozhi_encoder.c"
        "./audio/xiaozhi_decoder.c"
        "./websocket/xiaozhi_websocket.c"
    INCLUDE_DIRS
        "."
        "./button"
        "./wifi_sta"
        "./lcd"
        "./lvgl"
        "./com"
        "./http"
        "./audio"
        "./websocket"
        EMBED_FILES ${IMAGE_FILE}
)



# 把生成的符号名作为宏传给源文件
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    IMAGE_START_SYMBOL=_binary_${IMAGE_SYMBOL}_start
    IMAGE_END_SYMBOL=_binary_${IMAGE_SYMBOL}_end
)
