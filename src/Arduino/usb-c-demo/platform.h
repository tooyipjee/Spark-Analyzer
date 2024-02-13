
#include <Arduino.h>

void platform_usleep(uint64_t us) {
  delayMicroseconds(us);
}

