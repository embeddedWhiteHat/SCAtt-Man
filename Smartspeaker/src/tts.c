/* Implementation of the text to speech functions */
#include "tts.h"

static const char logPrefix[] = "TTS";

/* Building of the GET Request for the Watson API*/
void text_to_speech(const char *txt)
{
    char *query_str = urlencode(txt);

    ESP_LOGE(logPrefix, "HTTP GET request starting....");

    esp_http_client_config_t config = {
        .host = "api.eu-de.text-to-speech.watson.cloud.ibm.com",
        .path = "/v1/synthesize",
        .event_handler = _http_event_handler_tts,
        .user_data = local_response_buffer,
        .disable_auto_redirect = false,
        .query = query_str,
        .auth_type = HTTP_AUTH_TYPE_BASIC,
        .username = "apikey",
        .password = "please.use.your.own.token",
    };
    esp_http_client_handle_t client2 = esp_http_client_init(&config);
    esp_http_client_set_header(client2, "Accept", "audio/wav");

    parseWaveInit();

    esp_err_t err = esp_http_client_perform(client2);
    if (err == ESP_OK)
    {
        ESP_LOGI(logPrefix, "HTTP GET Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client2),
                 esp_http_client_get_content_length(client2));
    }
    else
    {
        ESP_LOGE(logPrefix, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client2);

    parseWaveCleanup();
    free(query_str);
}

char *urlencode(const char *originalText)
{
    // allocate memory for the worst possible case (all characters need to be encoded)
    char *encodedText_WithGetField = (char *)malloc(sizeof(char) * strlen(originalText) * 3 + 1 + 6);

    sprintf(encodedText_WithGetField, "text=");
    char *encodedText = encodedText_WithGetField + 5;

    const char *hex = "0123456789abcdef";

    int pos = 0;
    for (int i = 0; i < strlen(originalText); i++)
    {
        if (('a' <= originalText[i] && originalText[i] <= 'z') || ('A' <= originalText[i] && originalText[i] <= 'Z') || ('0' <= originalText[i] && originalText[i] <= '9'))
        {
            encodedText[pos++] = originalText[i];
        }
        else
        {
            encodedText[pos++] = '%';
            encodedText[pos++] = hex[originalText[i] >> 4];
            encodedText[pos++] = hex[originalText[i] & 15];
        }
    }
    encodedText[pos] = '\0';
    return encodedText_WithGetField;
}
