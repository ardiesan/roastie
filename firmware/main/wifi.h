#ifndef WIFI_H
#define WIFI_H

esp_err_t wifi_init(void);
bool wifi_is_connected(void);
void wifi_reconnect(void);

#endif // WIFI_H
