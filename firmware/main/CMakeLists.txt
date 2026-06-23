idf_component_register(
    SRCS "main.c"
           "wifi.c"
           "camera.c"
           "face_detect.c"
           "http_client.c"
           "i2s_audio.c"
           "status.c"
    INCLUDE_DIRS "."
    REQUIRES esp_http_client esp_wifi nvs_flash esp_camera i2s esp_timer driver
)
