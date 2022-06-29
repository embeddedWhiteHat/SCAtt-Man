/* Defines the Wifi connection setup functions */

#ifndef WIFI_CONNECTION
#define WIFI_CONNECTION

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include <esp_wifi.h>
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"


#define WIFI_CLIENT_SSID "YourWifi"
#define WIFI_CLIENT_PASS "yourWiFiPW"
#define WIFI_CLIENT_MAXIMUM_RETRY  3

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/* Initilializes the Wifi and connects to Access Point */

void wifi_init_sta(void);
void wifi_uninit_sta(void);

#endif