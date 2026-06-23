#include "i2s_audio.h"
#include "esp_log.h"
#include "driver/i2s_std.h"

#include <string.h>

static const char *TAG = "i2s_audio";
static i2s_chan_handle_t s_tx_handle = NULL;

esp_err_t i2s_audio_init(void)
{
    i2s_chan_handle_t tx_handle = NULL;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    esp_err_t ret = i2s_new_channel(&chan_cfg, NULL, &tx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S tx channel create failed");
        return ret;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)I2S_PIN_BCLK,
            .ws   = (gpio_num_t)I2S_PIN_LRC,
            .din  = (gpio_num_t)I2S_PIN_DIN,
            .dout = I2S_GPIO_UNUSED,
        },
    };

    ret = i2s_channel_init_std_mode(tx_handle, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S std mode init failed");
        return ret;
    }

    ret = i2s_channel_enable(tx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S enable failed");
        return ret;
    }

    s_tx_handle = tx_handle;
    ESP_LOGI(TAG, "I2S init OK: 16 kHz, 16-bit, mono (BCLK=%d, LRC=%d, DIN=%d)",
             I2S_PIN_BCLK, I2S_PIN_LRC, I2S_PIN_DIN);
    return ESP_OK;
}

esp_err_t i2s_audio_play(const uint8_t *data, size_t len)
{
    if (!s_tx_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    size_t bytes_written = 0;
    esp_err_t ret = i2s_write(s_tx_handle, data, len, &bytes_written, portMAX_DELAY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S write failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t i2s_audio_stop(void)
{
    if (!s_tx_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Flush the buffer by writing silence */
    uint8_t zeros[512] = {0};
    size_t dummy = 0;
    return i2s_write(s_tx_handle, zeros, sizeof(zeros), &dummy, portMAX_DELAY);
}

esp_err_t i2s_audio_deinit(void)
{
    if (s_tx_handle) {
        i2s_channel_disable(s_tx_handle);
        i2s_del_channel(s_tx_handle);
        s_tx_handle = NULL;
    }
    return ESP_OK;
}
