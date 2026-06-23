#ifndef STATUS_H
#define STATUS_H

#include <stdint.h>

typedef enum {
    STATUS_IDLE,
    STATUS_DETECTING,
    STATUS_UPLOADING,
    STATUS_PLAYING,
    STATUS_ERROR,
} status_t;

/* Initialise status LED (GPIO 2 — built-in on most boards). */
void status_init(void);

/* Set the current status. */
void status_set(status_t state);

/* Blink the LED `blinks` times with `interval_ms` between blinks. */
void status_blink(status_t state, uint8_t blinks, uint16_t interval_ms);

#endif // STATUS_H
