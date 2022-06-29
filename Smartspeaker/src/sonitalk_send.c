#include "sonitalk_send.h"

#include "atom_echo.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>

#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char logPrefix[] = "SoniTalkSend";

int16_t *LUT = NULL;      
int16_t *soniTalk_buffer;

#define FADE_OUT_TIME 0.04 // s
#define FADE_OUT_LENGTH ((int)(SAMPLE_RATE_SPEAKER * FADE_OUT_TIME))  // Nr of samples to fade out

bool init_soniTalk_player(){
    // Init LUT
    if(LUT==NULL){
        LUT = (int16_t*)malloc(sizeof(int16_t) * LUT_SIZE);
    }
    if(LUT==NULL){
        ESP_LOGE(logPrefix, "Failed to allocate %d Bytes for Sin-LUT\n", sizeof(int16_t) * LUT_SIZE);
        return false;
    }

    // Fill LUT
    for (int i = 0; i < LUT_SIZE; ++i)
    {
        LUT[i] = (int16_t)roundf(MAX_VOLUME * sinf(2.0f * M_PI * (float)i / LUT_SIZE));
    }

    // Allocate play-buffer
    if(soniTalk_buffer==NULL){
        soniTalk_buffer = (int16_t*)malloc(sizeof(int16_t) * BUFF_SIZE);
        if(soniTalk_buffer==NULL){
            ESP_LOGE(logPrefix, "Failed to allocate %d Bytes for soniTalk_buffer\n", sizeof(int16_t) * BUFF_SIZE);
            return false;
        }
    }
    
    // Fill st_start/st_end 
    init_start_end_blocks();

    InitI2S(MODE_SPK, SAMPLE_RATE_SPEAKER);

    ESP_LOGI(logPrefix, "Initialized Player\n");

    return true;
}

void uninit_soniTalk_player(){
    if(soniTalk_buffer!=NULL){
        free(soniTalk_buffer);
        soniTalk_buffer = NULL;
    }
    if(LUT!=NULL){
        free(LUT);
        LUT = NULL;
    }

    ESP_LOGI(logPrefix, "Uninitialized Player\n");
}

void generate_frequency_buffer(int nr_frequencies, int16_t frequencies[])
{
    float delta_phis[nr_frequencies];
    for (int f = 0; f < nr_frequencies; f++)
    {
        delta_phis[f] = (float) frequencies[f] / LUT_BASE_FREQUENCY;
    }
    // generate buffer of output
    int16_t value;
    int phase_i;
    for (int i = 0; i < BUFF_SIZE; ++i)
    {
        value = 0;
        if(nr_frequencies <= 0){
            soniTalk_buffer[i] = 0;
        }else{
            for (int f = 0; f < nr_frequencies; ++f)
            {
                phase_i = (int)(delta_phis[f] * i);        
                value += LUT[phase_i % LUT_SIZE];
            }
            soniTalk_buffer[i] = (int16_t)(value / nr_frequencies);
        }        
    }

    // apply fade out
    uint32_t pos;
    for (int i = 0; i < FADE_OUT_LENGTH; i++)
    {
        pos = BUFF_SIZE - FADE_OUT_LENGTH + i;
        soniTalk_buffer[pos] = (int16_t)((float)soniTalk_buffer[pos] * pow((1.0 - ((float)i)/ ((float)FADE_OUT_LENGTH)), 2.0));
    }
    
}

void bits_to_frequency_buffer(const bool *bits){
    // Count how many Frequencies shall be stacked
    uint8_t active_frequencies = 0;
    for (uint8_t i = 0; i < NR_OF_FREQUENCIES; ++i)
    {
        if(bits[i]){
            active_frequencies++;
        }
    }
    
    // 
    int16_t *frequencies = (int16_t*)malloc(sizeof(int16_t)*active_frequencies);
    uint8_t j = 0;
    for (uint8_t i = 0; i < NR_OF_FREQUENCIES; ++i)
    {
        if (bits[i])
        {
            frequencies[j] = BASE_FREQUENCY + i* FREQUENCY_SEPARATION;
            j++;
        }        
    }

    generate_frequency_buffer(active_frequencies, frequencies);

    free(frequencies);
}

bool* bytes_to_bits(const uint8_t* byteArr){
    bool* bitArr = (bool*)malloc(NR_OF_MESSAGES*NR_OF_FREQUENCIES*sizeof(bool));

    uint pos;
    uint8_t tmp;
    for(int i=0; i<NR_OF_MESSAGES; i++){
        for(int j=0; j<NR_OF_FREQUENCIES; j++){
            pos = i*NR_OF_FREQUENCIES + j;
            tmp = byteArr[pos/8];
            tmp = tmp >> (7-pos%8);
            bitArr[pos] = tmp&1;
        }
    }
    return bitArr;
}

void send_bits(bool *bits_raw){
    // Translate flat array into 2d
    bool bits[NR_OF_MESSAGES][NR_OF_FREQUENCIES];
    for(int i=0; i<NR_OF_MESSAGES; i++){
        for(int j=0; j<NR_OF_FREQUENCIES; j++){
            bits[i][j] = bits_raw[i*NR_OF_FREQUENCIES + j];
        }
    }    
    // play the data transmission
    // send start block
    bits_to_frequency_buffer(st_start);
    play_buffer(soniTalk_buffer, BUFF_SIZE);
    rtos_delay_ms(PAUSE_LENGTH);
    for (uint8_t i = 0; i < NR_OF_MESSAGES; ++i)
    {   
        // send block     
        bits_to_frequency_buffer(bits[i]);
        play_buffer(soniTalk_buffer, BUFF_SIZE);
        rtos_delay_ms(PAUSE_LENGTH);
    }
}

void sonitalk_send(uint8_t *bytes){
    init_soniTalk_player();

    send_bits(bytes_to_bits(bytes));

    uninit_soniTalk_receiver();

    
    for(int i=0; i<4; i++){
        ESP_LOGW("soniPlay", "Sent %d: 0x%02X (%d)\n", i, bytes[i], bytes[i]);
    }
}