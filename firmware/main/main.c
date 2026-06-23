#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "wifi.h"
#include "camera.h"
#include "http_client.h"
#include "i2s_audio.h"
#include "status.h"
#include "secrets.h"

static const char *TAG = "ai_roast";

/* Timing constants */
#define WIFI_STABLE_MS      2000
#define ROAST_INTERVAL_MS   3000  /* Minimum time between roasts */
#define MAX_AUDIO_BUF       (160 * 1024)  /* 160 KB max PCM */

static uint8_t s_audio_buf[MAX_AUDIO_BUF];

static void _delay_ms(uint32_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

static bool _run_roast(void)
{
    image_t img = {0};
    size_t audio_len = 0;

    /* Capture frame */
    status_set(STATUS_UPLOADING);
    if (!camera_capture(&img)) {
        ESP_LOGW(TAG, "Capture failed");
        status_blink(STATUS_ERROR, 3, 300);
        return false;
    }
    ESP_LOGI(TAG, "Captured JPEG: %zu bytes", img.len);

    /* Send to cloud */
    bool success = http_client_roast(img.data, img.len,
                                     s_audio_buf, sizeof(s_audio_buf),
                                     &audio_len);
    if (!success || audio_len == 0) {
        ESP_LOGW(TAG, "Roast upload failed");
        status_blink(STATUS_ERROR, 3, 300);
        return false;
    }

    /* Play audio */
    status_set(STATUS_PLAYING);
    esp_err_t ret = i2s_audio_play(s_audio_buf, audio_len);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "I2S play failed");
        status_blink(STATUS_ERROR, 5, 200);
        return false;
    }

    ESP_LOGI(TAG, "Roast played: %zu bytes PCM", audio_len);
    return true;
}

void app_main(void)
{
    ESP_LOGI(TAG, "AI Roast starting...");

    /* 1. Status LED */
    status_init();
    status_set(STATUS_IDLE);

    /* 2. Wi-Fi */
    ESP_LOGI(TAG, "Initialising Wi-Fi...");
    if (wifi_init() != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi init failed — will retry");
    }

    /* 3. Camera */
    ESP_LOGI(TAG, "Initialising camera...");
    camera_init();

    /* 4. I2S audio */
    ESP_LOGI(TAG, "Initialising I2S...");
    i2s_audio_init();

    ESP_LOGI(TAG, "All subsystems initialised");

    uint32_t last_roast_tick = 0;

    while (1) {
        uint32_t now = xTaskGetTickCount();

        /* Check Wi-Fi */
        if (!wifi_is_connected()) {
            status_set(STATUS_ERROR);
            status_blink(STATUS_ERROR, 2, 500);
            _delay_ms(1000);
            continue;
        }

        /* Cooldown between roasts */
        if ((now - last_roast_tick) * portTICK_PERIOD_MS < ROAST_INTERVAL_MS) {
            status_set(STATUS_IDLE);
            _delay_ms(500);
            continue;
        }

        /* Capture, upload, play */
        if (_run_roast()) {
            last_roast_tick = now;
        }

        _delay_ms(200);
    }
}
