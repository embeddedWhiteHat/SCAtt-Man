#ifndef __SONITALK_RECEIVE_H

#define __SONITALK_RECEIVE_H

#include <stdbool.h>
#include <stdint.h>

bool init_soniTalk_receiver();
void uninit_soniTalk_receiver();
uint8_t *sonitalk_receive();

/* 
Set sonitalkReceive_stop to true to signalize the sonitalk-task to deinitialize.
Then wait until sonitalkReceive_isRunning is set to false to leave time for the deinit procedure.
*/
extern bool sonitalkReceive_stop;
extern bool sonitalkReceive_isRunning;

#endif