#include "status.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include <string.h>

static const char *TAG = "status";

#define STATUS_LED_GPIO 2
#define BLINK_ON_MS     200
#define BLINK_OFF_MS    200

/* Timer callback for blink */
static void _blink_timer_callback(void *arg)
{
    uint32_t *remaining = (uint32_t *)arg;
    if (*remaining == 0) {
        gpio_set_level(STATUS_LED_GPIO, 0); /* Turn off */
        return;
    }
    /* Toggle LED */
    uint32_t level;
    gpio_get_level(STATUS_LED_GPIO, &level);
    gpio_set_level(STATUS_LED_GPIO, !level);
    *remaining -= 1;
}

void status_init(void)
{
    gpio_config_t io_conf = {0};
    io_conf.pin_bit_mask = (1ULL << STATUS_LED_GPIO);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);
    gpio_set_level(STATUS_LED_GPIO, 0);
    ESP_LOGI(TAG, "Status LED on GPIO %d", STATUS_LED_GPIO);
}

void status_set(status_t state)
{
    /* Solid on for active states, off for idle/error */
    bool on = (state != STATUS_IDLE && state != STATUS_ERROR);
    gpio_set_level(STATUS_LED_GPIO, on ? 1 : 0);
}

void status_blink(status_t state, uint8_t blinks, uint16_t interval_ms)
{
    (void)state;

    /* Use a one-shot timer to blink */
    esp_timer_handle_t timer = NULL;
    esp_timer_create_args_t args = {
        .callback = _blink_timer_callback,
        .arg = &blinks,
        .name = "status_blink",
    };
    esp_timer_create(&args, &timer);

    for (uint8_t i = 0; i < blinks; i++) {
        gpio_set_level(STATUS_LED_GPIO, 1);
        esp_timer_start_once(timer, BLINK_ON_MS * 1000);
        vTaskDelay(BLINK_ON_MS / portTICK_PERIOD_MS);
        gpio_set_level(STATUS_LED_GPIO, 0);
        if (i < blinks - 1) {
            vTaskDelay(interval_ms / portTICK_PERIOD_MS);
        }
    }

    esp_timer_delete(timer);
}
