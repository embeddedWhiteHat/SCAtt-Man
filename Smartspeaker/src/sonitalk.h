#ifndef __SONITALK_H

#define __SONITALK_H

#include <stdint.h>
#include "atom_echo.h"
#include "sonitalk_send.h"
#include "sonitalk_receive.h"

// SoniTalk Parameters
#define BLOCK_LENGTH 240         // ms
#define PAUSE_LENGTH 0           // ms
#define FREQUENCY_SEPARATION 200 // Hz
#define MAX_MESSAGE_SIZE 4       // Byte
#define NR_OF_FREQUENCIES 4      // Should be even
#define BASE_FREQUENCY 1010      //Hz
#define BITS_PER_BYTE 8

#define NR_OF_MESSAGES ((int)(MAX_MESSAGE_SIZE * BITS_PER_BYTE / NR_OF_FREQUENCIES))

extern bool st_start[NR_OF_FREQUENCIES];
extern bool st_end[NR_OF_FREQUENCIES];

// Sampling Parameters
#define LUT_BASE_FREQUENCY 10 // Hz
#define LUT_SIZE (SAMPLE_RATE_SPEAKER / LUT_BASE_FREQUENCY)
#define SOUND_LENGTH ((double)BLOCK_LENGTH / 1000)          // s
#define BUFF_SIZE (int)(SAMPLE_RATE_SPEAKER * SOUND_LENGTH) // size of output buffer (samples)
#define MAX_VOLUME ((int)(SHRT_MAX * 0.25))
#define VOLUME_RAISE 2

void init_start_end_blocks();

// sonitalk main task
void sonitalk();

#endif