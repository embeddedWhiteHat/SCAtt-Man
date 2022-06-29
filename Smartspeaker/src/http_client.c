/* Implementation of the REST functions */

#include "http_client.h"

ParseWave_T *pWave = NULL;
static const char *HTTP_TAG = "http client";

/* HTTP event handler for the Speech-to-text API-call */
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;
    static int output_len;
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(HTTP_TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            if (evt->user_data)
            {
                memcpy(evt->user_data + output_len, evt->data, evt->data_len);
            }
            else
            {
                if (output_buffer == NULL)
                {
                    output_buffer = (char *)malloc(esp_http_client_get_content_length(evt->client));
                    output_len = 0;
                    if (output_buffer == NULL)
                    {
                        ESP_LOGE(HTTP_TAG, "Failed to allocate memory for output buffer");
                        return ESP_FAIL;
                    }
                }
                memcpy(output_buffer + output_len, evt->data, evt->data_len);
            }
            output_len += evt->data_len;
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ON_FINISH");
        if (output_buffer != NULL)
        {
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(HTTP_TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
        if (err != 0)
        {
            if (output_buffer != NULL)
            {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            ESP_LOGI(HTTP_TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(HTTP_TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        break;
    }
    return ESP_OK;
}

/* HTTP event handler for the Text-to-speech API-call. Plays the response directly on the speaker */
esp_err_t _http_event_handler_tts(esp_http_client_event_t *evt)
{
    static char *output_buffer; // Buffer to store response of http request from event handler
    static int output_len;      // Stores number of bytes read
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGI(HTTP_TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(HTTP_TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(HTTP_TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(HTTP_TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        parseWaveChunk((uint8_t *)evt->data, evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(HTTP_TAG, "HTTP_EVENT_ON_FINISH");
        if (output_buffer != NULL)
        {
            // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
            ESP_LOG_BUFFER_HEX(HTTP_TAG, output_buffer, output_len);
            ESP_LOGI(HTTP_TAG, "output_buffer: %s", output_buffer);
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(HTTP_TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
        if (err != 0)
        {
            if (output_buffer != NULL)
            {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            ESP_LOGI(HTTP_TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(HTTP_TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        break;
    }
    return ESP_OK;
}

void parseWaveInit()
{
    pWave = (ParseWave_T *)calloc(1, sizeof(ParseWave_T));
    pWave->state = WAIT_FOR_HEADER;

    ESP_LOGE(HTTP_TAG, "parseWaveInit fin\n");
}

void parseWaveChunk(const uint8_t *data, int len)
{

    if (pWave->state == WAIT_FOR_HEADER)
    {
        if (len < sizeof(wav_header))
        {
            pWave->state = WAVE_ERROR;
            ESP_LOGE(HTTP_TAG, "First chunk does not contain enough data to fill wav_header. (Only %d instead of %d). Aborting\n", len, sizeof(wav_header));
            return;
        }

        memcpy(&pWave->header, data, sizeof(wav_header));

        data += sizeof(wav_header);
        len -= sizeof(wav_header);
        pWave->state = WAIT_FOR_DATA;

        ESP_LOGE(HTTP_TAG, "wav_size: %u, audio_format: %d, num_channels: %d\n", pWave->header.wav_size, pWave->header.audio_format, pWave->header.num_channels);

        ESP_LOGE(HTTP_TAG, "sample_rate: %u, byte_rate: %u, sample_alignment: %u\n", pWave->header.sample_rate, pWave->header.byte_rate, pWave->header.sample_alignment);

        ESP_LOGE(HTTP_TAG, "bit_depth: %u\n", pWave->header.bit_depth);
    }

    while (len > 0 && pWave->state == WAIT_FOR_DATA)
    {
        const data_header *dHeadPtr = (data_header *)data;
        ESP_LOGE(HTTP_TAG, "Chunk of type '%4s' , size: %u, len left: %d\n", dHeadPtr->data_header, dHeadPtr->data_bytes, len);
        if (0 == memcmp(dHeadPtr->data_header, "data", 4))
        {
            // Note: Watson does not fill data_bytes with a meaningful value, just 0xFFFFFFFF. Just skip magic-str and length gield
            data += 8;
            len -= 8;
            vTaskDelay(pdMS_TO_TICKS(1500));
            pWave->state = RECEIVING_SAMPLES;
        }
        else
        {
            // data_bytes does not contain magic-str (4bytes) and length field (+4bytes)
            len -= dHeadPtr->data_bytes + 8;
            data += dHeadPtr->data_bytes + 8;
        }
    }

    if (len > 0 && pWave->state == RECEIVING_SAMPLES)
    {
        size_t bytes_written;
        i2s_write(0, data, len, &bytes_written, portMAX_DELAY);
    }

    pWave->total_received += len;
}

void parseWaveCleanup()
{
    ESP_LOGE(HTTP_TAG, "parseWaveCleanup: %u Bytes received\n", pWave->total_received);
    free(pWave);
}