#include "wifi.h"
#include "secrets.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include <string.h>

#define MAX_SSID_LEN      32
#define MAX_PASS_LEN      64
#define CONNECT_TIMEOUT_S 30
#define STABILITY_MS      2000

static const char *TAG = "wifi";
static bool s_connected = false;
static uint32_t s_connect_start = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);

static bool _wifi_connect(const char *ssid, const char *pass)
{
    if (!ssid || !pass || !strlen(ssid)) {
        ESP_LOGW(TAG, "No credentials provided");
        return false;
    }

    wifi_config_t wifi_cfg = {0};
    memcpy(wifi_cfg.sta.ssid, ssid, strlen(ssid));
    if (strlen(pass)) {
        memcpy(wifi_cfg.sta.password, pass, strlen(pass));
    }

    esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
    esp_wifi_connect();
    return true;
}

static bool _wifi_load_nvs(const char *key, char *out, size_t max_len)
{
    nvs_handle h;
    if (nvs_open("wifi", NVS_READONLY, &h) != ESP_OK)
        return false;
    size_t len = max_len;
    esp_err_t ret = nvs_get_str(h, key, out, &len);
    nvs_close(h);
    return ret == ESP_OK && len > 1;
}

static void _wifi_save_nvs(const char *key, const char *val)
{
    nvs_handle h;
    nvs_open("wifi", NVS_READWRITE, &h);
    nvs_set_str(h, key, val);
    nvs_commit(h);
    nvs_close(h);
}

esp_err_t wifi_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ret = esp_event_handler_instance_register(WIFI_EVENT,
                                              ESP_EVENT_ANY_ID,
                                              &wifi_event_handler,
                                              NULL,
                                              &instance_any_id);
    ESP_ERROR_CHECK(ret);

    ret = esp_event_handler_instance_register(IP_EVENT,
                                              IP_EVENT_STA_GOT_IP,
                                              &wifi_event_handler,
                                              NULL,
                                              &instance_got_ip);
    ESP_ERROR_CHECK(ret);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    ESP_ERROR_CHECK(ret);

    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    ESP_ERROR_CHECK(ret);

    ret = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    ESP_ERROR_CHECK(ret);

    /* Try secrets.h first, then NVS */
    char ssid_buf[MAX_SSID_LEN] = {0};
    char pass_buf[MAX_PASS_LEN] = {0};

    if (strlen(SECRET_WIFI_SSID)) {
        strncpy(ssid_buf, SECRET_WIFI_SSID, MAX_SSID_LEN - 1);
        strncpy(pass_buf, SECRET_WIFI_PASSWORD, MAX_PASS_LEN - 1);
        _wifi_save_nvs("ssid", ssid_buf);
        _wifi_save_nvs("pass", pass_buf);
    } else if (_wifi_load_nvs("ssid", ssid_buf, sizeof(ssid_buf)) &&
               _wifi_load_nvs("pass", pass_buf, sizeof(pass_buf))) {
        ESP_LOGI(TAG, "Loaded credentials from NVS");
    } else {
        ESP_LOGE(TAG, "No Wi-Fi credentials available");
        return ESP_FAIL;
    }

    _wifi_connect(ssid_buf, pass_buf);

    return ESP_OK;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_CONNECTED:
            s_connected = false;
            s_connect_start = xTaskGetTickCount();
            ESP_LOGI(TAG, "Connected to AP");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            s_connected = false;
            ESP_LOGW(TAG, "Disconnected. Reconnecting...");
            esp_wifi_connect();
            break;
        default:
            break;
        }
    } else if (event_base == IP_EVENT &&
               event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)event_data;
        char ip_str[16];
        snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d",
                 IP2STR(&ev->ip_info.ip));
        ESP_LOGI(TAG, "Got IP: %s", ip_str);
        s_connected = true;
        s_connect_start = xTaskGetTickCount();
    }
}

bool wifi_is_connected(void)
{
    if (!s_connected)
        return false;
    /* Require stable connection for STABILITY_MS */
    return (xTaskGetTickCount() - s_connect_start) * portTICK_PERIOD_MS >=
           STABILITY_MS;
}

void wifi_reconnect(void)
{
    s_connected = false;
    esp_wifi_disconnect();
    esp_wifi_connect();
}
