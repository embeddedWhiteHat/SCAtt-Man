/* Implentation of the smart speaker functions */

#include "smart_speaker.h"
#include "atom_echo.h"

static const char logPrefix[] = "STT";

char strftime_buf[64];

/*  Triggers STT. Generates the suitable answer for the command and triggers TTS */
void smart_speaker()
{
    ledSet(255, 0, 0);
    char *command = speech_to_text();
    ledSet(255, 0, 255);

    InitI2S(MODE_SPK, 16000);
    if (command == NULL)
    {
        text_to_speech("I Could not understand you");
        ESP_LOGI(logPrefix, "STT failed to recognize anything.");
    }
    else if (strcmp(command, "\"clock\"") == 0)
    {
        time_t now;
        struct tm timeinfo;
        setenv("TZ", "GMT-1", 1);
        tzset();
        time(&now);
        localtime_r(&now, &timeinfo);
        // Is time set? If not, tm_year will be (1970 - 1900).
        if (timeinfo.tm_year < (2016 - 1900))
        {
            ESP_LOGI(logPrefix, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
            obtain_time();
            time(&now);
            localtime_r(&now, &timeinfo);
        }
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        ESP_LOGV(logPrefix, "%s\n", strftime_buf);
        int hours = timeinfo.tm_hour;
        int minutes = timeinfo.tm_min;
        char timestring[64];
        sprintf(timestring, "It is %d %d\n", hours, minutes);
        ESP_LOGV(logPrefix, "%s", timestring);
        ESP_LOGI(logPrefix, "================================\n%s", timestring);

        text_to_speech(timestring);
    }
    else if (strcmp(command, "\"hello\"") == 0)
    {
        text_to_speech("Hello. My name is Scattman. What can I do for you?");
    }
    else
    {
        text_to_speech("Sorry I could not find the Command ");
        text_to_speech(command);
    }
}

/* Fetches time from the SNTP server */
void obtain_time(void)
{

    /**
     * NTP server address could be aquired via DHCP,
     * see LWIP_DHCP_GET_NTP_SRV menuconfig option
     */
#ifdef LWIP_DHCP_GET_NTP_SRV
    sntp_servermode_dhcp(1);
#endif

    initialize_sntp();

    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count)
    {
        ESP_LOGI(logPrefix, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
}

/* Initializes the SNTP server connection */
void initialize_sntp(void)
{
    ESP_LOGI(logPrefix, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    sntp_init();
}