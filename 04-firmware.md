# Phase 4: Firmware (ESP32-S3)

## Goal

Build the ESP-IDF firmware that detects faces, captures images, uploads to Lambda, and plays back roast audio.

## Tasks

### 4.1 ESP-IDF project setup

```
firmware/
├── CMakeLists.txt          # Project-level CMake
├── sdkconfig.defaults      # Default ESP-IDF config
├── partitions.csv          # Flash partition table
├── main/
│   ├── CMakeLists.txt
│   ├── main.c              # Entry point, app task loop
│   ├── wifi.c/h            # WiFi connection + captive portal
│   ├── camera.c/h          # OV2640 capture
│   ├── face_detect.c/h     # ESP-WHO face detection
│   ├── http_client.c/h     # HTTPS POST to Lambda
│   ├── i2s_audio.c/h       # I2S playback to MAX98357A
│   ├── status.c/h          # LED status indicator
│   ├── secrets.h           # (gitignored) WiFi creds, endpoint, API key
│   └── secrets.example.h   # Template (committed)
```

### 4.2 `sdkconfig.defaults` — Key config

```
CONFIG_ESP_TASK_WDT=y
CONFIG_ESP_TASK_WDT_TIMEOUT_S=30
CONFIG_SPIRAM_USE_MALLOC=y
CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=4096
CONFIG_FREERTIC_HZ=1000
CONFIG_ESP_MAIN_TASK_STACK_SIZE=6144
# Camera
CONFIG_OV2640_SUPPORT=y
CONFIG_GC032A_SUPPORT=n
# I2S
CONFIG_I2S_ISR_IRAM_SAFE=y
# Wi-Fi
CONFIG_ESP_WIFI_SOFTMAX_CONN=1
```

### 4.3 `partitions.csv` — Flash layout

```csv
# Name,   Type, SubType, Offset,  Size,   Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 2M,
```

### 4.4 `main/wifi.c/h` — Wi-Fi + captive portal

- Connect to stored WiFi credentials (from `secrets.h`)
- If connection fails after 3 attempts → start AP mode with captive portal (simple HTML form for SSID/password entry)
- Store credentials in NVS flash (esp_wifi_store) so no re-entry needed
- Status LED: blink fast = scanning, solid = connected

### 4.5 `main/camera.c/h` — OV2640 capture

- Initialize camera with PSRAM frame buffer
- Resolution: VGA (640x480) — sufficient for face detection
- Format: JPEG
- Capture function: `camera_capture_jpeg(uint8_t** buf, size_t* len)`

### 4.6 `main/face_detect.c/h` — ESP-WHO integration

- Initialize ESP-WHO model (ESP-DSP + face recognition library)
- Periodic detection loop: take frame → detect faces → track stability
- Face is "stable" when detected continuously for ~2 seconds
- Track face bounding box across frames for consistency check
- Return: `face_detected(bool)`, `face_count(int)`

### 4.7 `main/http_client.c/h` — HTTPS POST

- Connect to Lambda Function URL over HTTPS
- Set headers: `X-API-Key`, `Content-Type: application/octet-stream`
- POST raw JPEG bytes
- Read response: audio PCM bytes
- Handle errors: timeout, connection refused, non-200 status
- Retry with exponential backoff (max 3 retries, base 1s)

### 4.8 `main/i2s_audio.c/h` — I2S playback

- Configure I2S in TX mode:
  - BCLK: GPIO 17 (or board-specific)
  - LRC: GPIO 16 (or board-specific)
  - DIN: GPIO 15 (or board-specific)
- Sample rate: 16000 Hz, 16-bit, mono
- DMA double-buffering: two 4KB buffers, alternate on fill/drain
- Play function: `i2s_play_audio(const uint8_t* data, size_t len)`
- Non-blocking: queue audio, return immediately

### 4.9 `main/status.c/h` — Status LED

- GPIO 2 (or board-specific LED)
- States:
  - Off: idle (no face detected)
  - Slow blink (1 Hz): face detected, waiting to capture
  - Fast blink (5 Hz): uploading to cloud
  - Solid: playing roast audio
  - Double blink (2 Hz): error / offline

### 4.10 `main/main.c` — App entry point

```c
void app_main(void) {
    wifi_init();              // Connect or start captive portal
    camera_init();            // Initialize OV2640
    face_detect_init();       // Load ESP-WHO model
    i2s_audio_init();         // Configure I2S TX
    status_led_init();

    while (1) {
        status_update();      // Check face detection state

        if (face_stable_for_2s()) {
            status_set(UPLOADING);
            uint8_t* img; size_t len;
            camera_capture(&img, &len);
            uint8_t* audio; size_t audio_len;
            if (http_post_roast(img, len, &audio, &audio_len) == ESP_OK) {
                status_set(PLAYING);
                i2s_play_audio(audio, audio_len);
                // Wait for playback to finish
                while (i2s_is_playing()) { vTaskDelay(10 / portTICK_PERIOD_MS); }
            }
            free(img); free(audio);
        }

        vTaskDelay(100 / portTICK_PERIOD_MS); // ~10 FPS detection loop
    }
}
```

### 4.11 Offline fallback mode

- Pre-load 3-5 roast audio clips into flash (SPIFFS partition)
- If HTTP POST fails after all retries → play one random local clip
- Status LED: double blink = offline mode

### 4.12 Warm-up ping

- On boot, after WiFi connects, send a test POST to Lambda
- Discard the response — this warms up the Lambda for the first real roast

## Pin Mapping (ESP32-S3 Freenode Board)

| Signal | GPIO | Notes |
|--------|------|-------|
| Camera PWDN | GPIO 5 | OV2640 |
| Camera RESET | GPIO 15 | OV2640 |
| Camera XCLK | GPIO 7 | OV2640 |
| Camera SDA | GPIO 4 | OV2640 |
| Camera SCL | GPIO 5 | OV2640 |
| Camera D7-D0 | GPIO 8-15 | OV2640 data bus |
| Camera VSYNC | GPIO 6 | OV2640 |
| Camera HREF | GPIO 21 | OV2640 |
| Camera PCLK | GPIO 13 | OV2640 |
| I2S BCLK | GPIO 17 | To MAX98357A BCLK |
| I2S LRC | GPIO 16 | To MAX98357A LRC (BCLK) |
| I2S DIN | GPIO 15 | To MAX98357A DIN (SDOUT) |
| Status LED | GPIO 2 | Built-in LED |

> **Note:** Pin numbers are board-specific. Adjust based on actual hardware. The XIAO ESP32S3 Sense has different pinout.

## Verification

- `idf.py build` succeeds with no warnings
- Flash to board: `idf.py -p /dev/ttyUSB0 flash`
- Serial monitor shows: WiFi connected → face detected → upload → play
- LED status transitions through all states
- Offline mode works (disconnect WiFi → local clip plays)

## Dependencies

- ESP-IDF v5.x (installed via `export IDF_PATH=...`)
- ESP-WHO component (included as ESP-IDF component)
- Hardware: ESP32-S3 board, OV2640 camera, MAX98357A DAC, speaker

## Estimated Effort

~6-8 hours (firmware development + hardware testing).
