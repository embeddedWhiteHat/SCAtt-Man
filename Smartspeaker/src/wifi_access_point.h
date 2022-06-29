#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define WIFI_SOFTAP_SSID "AtomEcho"
#define WIFI_SOFTAP_PASS "qwer1234"
#define WIFI_SOFTAP_CHANNEL 1
#define WIFI_SOFTAP_MAX_CONN 1

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

// Defined in wifi_accesspoint.c
extern int AP_ConnectedCnt;

void wifi_init_ap(void);
void wifi_uninit_ap(void);