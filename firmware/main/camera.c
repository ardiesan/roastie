#include "camera.h"
#include "esp_camera.h"
#include "esp_log.h"

#define TAG "camera"

/* OV2640 pinout for generic ESP32 (AI-Thinker style) */
#define CAM_PIN_XCLK    4
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27
#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      19
#define CAM_PIN_D2      18
#define CAM_PIN_D1       5
#define CAM_PIN_D0      34
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

static esp_err_t camera_init_internal(void)
{
    camera_config_t config = {0};

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = CAM_PIN_D0;
    config.pin_d1 = CAM_PIN_D1;
    config.pin_d2 = CAM_PIN_D2;
    config.pin_d3 = CAM_PIN_D3;
    config.pin_d4 = CAM_PIN_D4;
    config.pin_d5 = CAM_PIN_D5;
    config.pin_d6 = CAM_PIN_D6;
    config.pin_d7 = CAM_PIN_D7;
    config.pin_xclk = CAM_PIN_XCLK;
    config.pin_pclk = CAM_PIN_PCLK;
    config.pin_vsync = CAM_PIN_VSYNC;
    config.pin_href = CAM_PIN_HREF;
    config.pin_sccb_sda = CAM_PIN_SIOD;
    config.pin_sccb_scl = CAM_PIN_SIOC;
    config.pin_pwdn = -1;
    config.pin_reset = -1;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.dcic_mode = 0;

    /* Try high resolution first, fall back to QVGA if memory is tight */
    esp_err_t ret = esp_camera_init(&config);
    if (ret == ESP_ERR_NOT_SUPPORTED && config.frame_size == FRAMESIZE_VGA) {
        ESP_LOGW(TAG, "VGA not supported, falling back to QVGA");
        config.frame_size = FRAMESIZE_QVGA;
        ret = esp_camera_init(&config);
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "camera init failed: 0x%x", ret);
        return ret;
    }

    /* Ensure JPEG output */
    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        s->set_pixformat(s, PIXFORMAT_JPEG);
    }

    ESP_LOGI(TAG, "Camera init OK (frame_size=%d)", config.frame_size);
    return ESP_OK;
}

esp_err_t camera_init(void)
{
    return camera_init_internal();
}

bool camera_capture(image_t *img)
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGW(TAG, "snapshot failed");
        return false;
    }

    img->data = (uint8_t *)fb->buf;
    img->len = fb->len;

    /* Return buffer to pool so next capture works */
    esp_camera_fb_return(fb);
    return true;
}

void camera_deinit(void)
{
    esp_camera_deinit();
}
