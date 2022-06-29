/* Implentation of the speech to text funtions functions */

#include <driver/i2s.h>
#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "http_client.h"

#include "stt.h"
#include "atom_echo.h"

static const char logPrefix[] = "STT";

static int data_offset = 0;
static uint8_t *microphonedata;
static char *stt_response;
#define DATA_SIZE 1024

char *speech_to_text()
{
    data_offset = 0;
    InitI2SSpeakerOrMic(MODE_MIC);
    size_t byte_read;

    microphonedata = (uint8_t *)calloc(1024 * 40, sizeof(uint8_t));
    stt_response = (char *)calloc(2048, sizeof(char));

    while (1)
    {
        i2s_read(SPEAK_I2S_NUMBER, (char *)(microphonedata + data_offset), DATA_SIZE, &byte_read, (100 / portTICK_RATE_MS));
        data_offset += 1024;
        if (data_offset == 1024 * 40)
        {
            break;
        }
    }
    ESP_LOGD(logPrefix, "Post request following\n");
    stt_post_request(microphonedata, data_offset, stt_response);
    ESP_LOGD(logPrefix, "%s\n", stt_response);

    char *result = NULL;
    char *command = (char *)calloc(2048, sizeof(char));
    strtok(stt_response, "[");
    char *secondHalf = strtok(NULL, "["); // Get pointer to first [
    if (secondHalf != NULL)
    {                                            // Assure there even is a [
        command = strtok(secondHalf, "]"); // Get ptr for following ]
        // result = calloc(1, (command - secondHalf + 2) * sizeof(char *));
        // memcpy(result, secondHalf, secondHalf - command);
        // ESP_LOGD(logPrefix, "Command: %s\n", command);
    }

    free(microphonedata);
    free(stt_response);

    ESP_LOGI(logPrefix, "================================\n%s", command);

    // return result;
    return command;
}

/*  Speech-to-text API-call that calls Baidu-Rest and posts the I2S
    microphone-data */
void stt_post_request(uint8_t *post_data, uint32_t pcm_lan, char *response)
{
    char output_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    int content_length = 0;

    char strbuff[128];
    sprintf(strbuff, "lan=%d", pcm_lan);

    char postString[1000];
    sprintf(postString, "http://flow.m5stack.com:5003/pcm?cuid=1A57&token=deadbeaf%s", strbuff);

    esp_http_client_config_t config = {
                .url = postString,
                .event_handler = _http_event_handler,
                .user_data = output_buffer,
                .disable_auto_redirect = true,
                .timeout_ms = 15000,
    };
    char new_url[100];

    if (nvslib_begin()) {
        if (nvslib_get_blob("http_conf", (uint8_t *) new_url, sizeof(new_url))) {
            config.url = new_url;
        }
        nvslib_close();
    }
    ESP_LOGI(logPrefix, "URL: %s", config.url);
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "audio/pcm;rate=16000");
    esp_err_t err = esp_http_client_open(client, pcm_lan);
    if (err != ESP_OK)
    {
        ESP_LOGE(logPrefix, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    }
    else
    {
        int wlen = esp_http_client_write(client, (char *)post_data, pcm_lan);
        if (wlen < 0)
        {
            ESP_LOGE(logPrefix, "Write failed");
        }
        content_length = esp_http_client_fetch_headers(client);
        if (content_length < 0)
        {
            ESP_LOGE(logPrefix, "HTTP client fetch headers failed");
        }
        else
        {
            int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_OUTPUT_BUFFER);
            if (data_read >= 0)
            {
                ESP_LOGI(logPrefix, "HTTP POST Status = %d, content_length = %d",
                         esp_http_client_get_status_code(client),
                         esp_http_client_get_content_length(client));
            }
            else
            {
                ESP_LOGE(logPrefix, "Failed to read response");
            }
        }
    }
    esp_http_client_cleanup(client);
    memcpy(response, output_buffer, 2048);
}

/* Synchronizes SNTP time */

#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_CUSTOM
void sntp_sync_time(struct timeval *tv)
{
    settimeofday(tv, NULL);
    ESP_LOGI(STT_TAG, "Time is synchronized from custom code");
    sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
}
#endif

/* Notifies on time synchronization event */

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(logPrefix, "Notification of a time synchronization event");
}