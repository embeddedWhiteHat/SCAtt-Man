/* Implentation of the speech to text functions */

#ifndef STT
#define STT

#include <stdio.h>
#include "nvslib.h"
#include "http_server.h"

char* speech_to_text();

/*  Speech-to-text API-call that calls Baidu-Rest and posts the I2S
    microphone-data */
void stt_post_request(uint8_t* post_data, uint32_t pcm_lan, char *response);

/* Synchronizes SNTP time */

#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_CUSTOM
void sntp_sync_time(struct timeval *tv);
#endif

/* Notifies on time synchronization event */

void time_sync_notification_cb(struct timeval *tv);

/* Initializes the I2S Speaker of Microphone 
    Necessary because the data returned by TTS Service
    is 16kHz instead of */
void InitI2SSpeakerOrMic_tts(int mode);

#endif