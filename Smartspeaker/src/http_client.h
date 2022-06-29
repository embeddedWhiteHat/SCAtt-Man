/* Defines the REST functions */

#ifndef HTTP_CLIENT
#define HTTP_CLIENT

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include <driver/i2s.h>
#include <driver/gpio.h>
#include "esp_http_client.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

char local_response_buffer[MAX_HTTP_RECV_BUFFER];

/* HTTP event handler for the Speech-to-text API-call */
esp_err_t _http_event_handler(esp_http_client_event_t *evt);

typedef struct __attribute__((packed)) {
    // RIFF Header
    char riff_header[4]; // Contains "RIFF"
    uint32_t wav_size; // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
    char wave_header[4]; // Contains "WAVE"
    
    // Format Header
    char fmt_header[4]; // Contains "fmt " (includes trailing space)
    uint32_t fmt_chunk_size; // Should be 16 for PCM
    uint16_t audio_format; // Should be 1 for PCM. 3 for IEEE Float
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate; // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
    uint16_t sample_alignment; // num_channels * Bytes Per Sample
    uint16_t bit_depth; // Number of bits per sample
    
} wav_header;

typedef struct __attribute__((packed)) {
    // There seem to be multiple possible "data-chunks", eg the chunk following wav_header watson gives us is "LIST", whatever that is
    // Later on we just search for "DATA", which contains the sample-data
    char data_header[4]; // Contains "data"
    uint32_t data_bytes; // Number of bytes in data. Number of samples * num_channels * sample byte size
} data_header;

typedef struct {
    enum  { WAIT_FOR_HEADER, WAIT_FOR_DATA, RECEIVING_SAMPLES, WAVE_ERROR} state;
    size_t total_received;
    wav_header header;
} ParseWave_T;

/* HTTP event handler for the Text-to-speech API-call. Plays the response directly on the speaker */
esp_err_t _http_event_handler_tts(esp_http_client_event_t *evt);

void parseWaveInit();

void parseWaveChunk(const uint8_t* data, int len);

void parseWaveCleanup();

#endif