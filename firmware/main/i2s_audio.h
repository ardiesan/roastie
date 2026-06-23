#ifndef I2S_AUDIO_H
#define I2S_AUDIO_H

#include <stdint.h>
#include <stdbool.h>

/* MAX98357A I2S pins for generic ESP32 */
#define I2S_PIN_BCLK  18
#define I2S_PIN_LRC   23
#define I2S_PIN_DIN   19

/* Initialise I2S in master mode, 16-bit, 16 kHz, mono. */
esp_err_t i2s_audio_init(void);

/* Write PCM samples to I2S. Blocks until all samples are queued. */
esp_err_t i2s_audio_play(const uint8_t *data, size_t len);

/* Stop playback and flush the I2S buffer. */
esp_err_t i2s_audio_stop(void);

/* Deinitialise I2S. */
esp_err_t i2s_audio_deinit(void);

#endif // I2S_AUDIO_H
