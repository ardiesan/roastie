#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>

typedef struct {
    uint8_t *data;
    size_t len;
} image_t;

esp_err_t camera_init(void);
bool camera_capture(image_t *img);
void camera_deinit(void);

#endif // CAMERA_H
