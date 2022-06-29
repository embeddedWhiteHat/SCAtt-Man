/* Implentation of the smart speaker functions */
#include "stt.h"
#include "tts.h"
#include "esp_sntp.h"
#include <time.h>
#include <sys/time.h>

/*  Triggers STT. Generates the suitable answer for the command and triggers TTS */
void smart_speaker();

/* Fetches time from the SNTP server */
void obtain_time(void);

/* Initializes the SNTP server connection */
void initialize_sntp(void);