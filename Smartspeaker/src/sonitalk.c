#include "sonitalk.h"
#include <stdbool.h>

bool start_end_blocks_is_init = false;

bool st_start[NR_OF_FREQUENCIES];

void init_start_end_blocks()
{
    if (!start_end_blocks_is_init)
    {
        uint8_t i = 0;
        while (i < NR_OF_FREQUENCIES / 2)
        {
            st_start[i] = 0;
            ++i;
        }
        while (i < NR_OF_FREQUENCIES)
        {
            st_start[i] = 1;
            ++i;
        }
        start_end_blocks_is_init = true;
    }
}