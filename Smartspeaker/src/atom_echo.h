#ifndef __ATOM_ECHO_H

#define __ATOM_ECHO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define CONFIG_I2S_BCK_PIN 19
#define CONFIG_I2S_LRCK_PIN 33
#define CONFIG_I2S_DATA_PIN 22
#define CONFIG_I2S_DATA_IN_PIN 23

// Sampling Parameters
#define SAMPLE_RATE_SPEAKER 44100 // Hz
#define SAMPLE_RATE_MIC 16000 // Hz

#define SPEAK_I2S_NUMBER I2S_NUM_0

#define MODE_MIC 0
#define MODE_SPK 1

void init_button();
bool isButtonDown(void);

bool InitI2S(int mode, uint32_t sample_rate);
void InitI2SSpeakerOrMic(int mode);
void play_buffer(int16_t* buffer, int buff_size);

void readMic(int16_t* dst, size_t byteCnt);

double getTimeRTOS();
void rtos_delay_ms(uint16_t duration);

void init_led();
void ledSet(uint32_t r, uint32_t g, uint32_t b);

#define COL_WAIT_BUTTON 64, 64, 64
#define COL_WAIT_APCON  255, 0  ,   0
#define COL_SONI_LISTEN   0, 255,   0
#define COL_SONI_PLAY   255,   0, 255

#define COL_STT_LISTEN    0,   0, 255
#define COL_TTS_PLAY    255, 255,   0

#endif