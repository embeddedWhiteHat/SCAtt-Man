#include <stdint.h>
#include <wifi_access_point.h>

enum MODE_WIFI_T {
    MODE_WIFI_NONE = 0,
    MODE_WIFI_STA,
    MODE_WIFI_AP,
};

void wifi_set_mode(int mode);

