#ifndef FACE_DETECT_H
#define FACE_DETECT_H

#include <stdint.h>
#include <stdbool.h>

/* Face detected by the cloud. Coordinates are 0-1000 (permille). */
typedef struct {
    int x;       /* center X, permille */
    int y;       /* center Y, permille */
    int w;       /* width, permille */
    int h;       /* height, permille */
    int confidence; /* 0-1000 */
} face_t;

/* State for tracking face stability across frames. */
typedef struct {
    bool face_present;
    uint32_t stable_start_ms; /* tick count when face first became stable */
} face_detect_state_t;

/*
 * Initialise face detection state.
 * On-device ESP-WHO is not included (heavy deps). Face detection
 * happens in the cloud via Rekognition; this module tracks stability
 * of the cloud-reported face across consecutive frames.
 */
void face_detect_init(face_detect_state_t *state);

/*
 * Update state with the latest face reported by the cloud.
 * Returns true if a face has been stable for >= threshold_ms.
 */
bool face_detect_update(face_detect_state_t *state, const face_t *face,
                        uint32_t threshold_ms);

/* Reset stability counter (e.g. after uploading). */
void face_detect_reset(face_detect_state_t *state);

#endif // FACE_DETECT_H
