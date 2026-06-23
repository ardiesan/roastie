#include "face_detect.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "face_detect";

void face_detect_init(face_detect_state_t *state)
{
    state->face_present = false;
    state->stable_start_ms = 0;
}

bool face_detect_update(face_detect_state_t *state, const face_t *face,
                        uint32_t threshold_ms)
{
    int64_t now_ms = esp_timer_get_time() / 1000;

    if (!face || face->confidence < 500) {
        /* No face or low confidence — reset stability */
        if (state->face_present) {
            ESP_LOGI(TAG, "Face lost");
        }
        state->face_present = false;
        state->stable_start_ms = 0;
        return false;
    }

    if (!state->face_present) {
        /* Face just appeared */
        state->stable_start_ms = (uint32_t)now_ms;
        state->face_present = true;
        ESP_LOGI(TAG, "Face detected (conf=%d)", face->confidence);
    }

    /* Check if face has been stable long enough */
    uint32_t elapsed = (uint32_t)now_ms - state->stable_start_ms;
    if (elapsed >= threshold_ms) {
        return true;
    }

    return false;
}

void face_detect_reset(face_detect_state_t *state)
{
    state->face_present = false;
    state->stable_start_ms = 0;
}
