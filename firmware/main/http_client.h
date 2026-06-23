#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Send a JPEG image to the roast Lambda and receive PCM audio.
 *
 * Returns true if audio was received (audio_buf contains valid PCM),
 * false on any error (network, auth, server error).
 */
bool http_client_roast(const uint8_t *jpeg_buf, size_t jpeg_len,
                       uint8_t *audio_buf, size_t audio_buf_len,
                       size_t *audio_out_len);

#endif // HTTP_CLIENT_H
