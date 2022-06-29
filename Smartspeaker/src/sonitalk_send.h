#ifndef __SONITALK_SEND_H

#define __SOITALK_SEND_H

#include "sonitalk.h"
#include <stdbool.h>

bool init_soniTalk_player();
void uninit_soniTalk_player();
void sonitalk_send(uint8_t *byte);

#endif