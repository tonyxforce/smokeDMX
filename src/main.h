#define CONFIG_RDM_DEVICE_UID_MAN_ID (0x05e0)
#define RDM_UID_MANUFACTURER_ID (CONFIG_RDM_DEVICE_UID_MAN_ID)
#define DMX_INTR_FLAGS_DEFAULT (1 << 10)

#include <Arduino.h>
#define DEBUG 1
#define ISBETWEEN(lowEnd, test, highEnd) (lowEnd <= test && test <= highEnd)