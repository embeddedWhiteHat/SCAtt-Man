

#include "wifi.h"
#include "wifi_connection.h"
#include "wifi_access_point.h"


static int wifi_mode_current = MODE_WIFI_NONE;

void wifi_set_mode(int mode){
    if(mode == wifi_mode_current){
        return;
    }
    else
    {
        // First uninitialize current mode
        if(wifi_mode_current == MODE_WIFI_STA)
        {
            wifi_uninit_sta();
        }
        else if(wifi_mode_current == MODE_WIFI_AP)
        {
            wifi_uninit_ap();
        }
        else if(wifi_mode_current == MODE_WIFI_NONE){
            // First Init: Init basics
            ESP_ERROR_CHECK(esp_netif_init());
            esp_netif_create_default_wifi_sta();
            esp_netif_create_default_wifi_ap();
        }

        // initialize requested mode
        if(mode == MODE_WIFI_STA)
        {
            wifi_init_sta();
        }
        else if(mode == MODE_WIFI_AP)
        {
            wifi_init_ap();
        }

        wifi_mode_current = mode;
    }
}

