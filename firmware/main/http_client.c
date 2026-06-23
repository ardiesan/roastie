#include "http_client.h"
#include "secrets.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_tls.h"

#include <string.h>

static const char *TAG = "http_client";

/* Maximum roast audio size: ~5 s of 16 kHz mono 16-bit PCM = 160 KB */
#define MAX_AUDIO_SIZE (160 * 1024)

bool http_client_roast(const uint8_t *jpeg_buf, size_t jpeg_len,
                       uint8_t *audio_buf, size_t audio_buf_len,
                       size_t *audio_out_len)
{
    if (!audio_buf || !audio_out_len) {
        ESP_LOGE(TAG, "Invalid output buffer");
        return false;
    }
    *audio_out_len = 0;

    esp_http_client_config_t config = {0};
    config.url = SECRET_ROAST_ENDPOINT;
    config.timeout_ms = 10000; /* 10 s total timeout */
    config.event_handler = NULL;
    /* We send raw bytes, not form data */
    config.method = HTTP_METHOD_POST;
    config.disable_auto_redirect = false;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "HTTP client init failed");
        return false;
    }

    /* Set headers */
    esp_http_client_set_header(client, "Content-Type", "application/octet-stream");
    esp_http_client_set_header(client, "X-API-Key", SECRET_ROAST_API_KEY);
    esp_http_client_set_header(client, "Content-Length", NULL);
    /* Set content length — esp_http_client will handle chunked encoding */
    char len_str[16];
    snprintf(len_str, sizeof(len_str), "%zu", jpeg_len);
    esp_http_client_set_header(client, "Content-Length", len_str);

    esp_err_t err = esp_http_client_open(client, (int)jpeg_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP open failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }

    /* Write JPEG payload */
    size_t written = 0;
    while (written < jpeg_len) {
        size_t to_write = jpeg_len - written;
        int w = esp_http_client_write(client, (const char *)(jpeg_buf + written), to_write);
        if (w < 0) {
            ESP_LOGE(TAG, "HTTP write failed");
            esp_http_client_cleanup(client);
            return false;
        }
        written += w;
    }

    /* Read response */
    int status_code = esp_http_client_fetch_headers(client);
    if (status_code < 200 || status_code >= 300) {
        char err_msg[64];
        snprintf(err_msg, sizeof(err_msg), "HTTP %d", status_code);
        ESP_LOGW(TAG, "%s", err_msg);
        esp_http_client_cleanup(client);
        return false;
    }

    int content_length = esp_http_client_get_content_length();
    if (content_length <= 0 || content_length > (int)audio_buf_len) {
        content_length = (int)audio_buf_len;
    }

    int total_read = 0;
    while (total_read < content_length) {
        int read_len = esp_http_client_read(client,
                                            (char *)audio_buf + total_read,
                                            content_length - total_read);
        if (read_len <= 0) {
            break;
        }
        total_read += read_len;
    }

    *audio_out_len = (size_t)total_read;
    esp_http_client_cleanup(client);

    if (total_read == 0) {
        ESP_LOGW(TAG, "No audio data received");
        return false;
    }

    ESP_LOGI(TAG, "Received %d bytes of audio (HTTP %d)", total_read, status_code);
    return true;
}
