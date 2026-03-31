#pragma once

#include "secrets.h"

#define SERIAL_BAUD 115200
#define LED_PIN 2
#define DEFAULT_SYNC_PIN 25
#define DEFAULT_BPM 120.0f
#define DEFAULT_PPB 2
#define DEFAULT_PULSE_WIDTH_MS 15
#define LINK_TIMEOUT_US 3000000ULL

const int allowedPins[] = {25, 26, 27, 14, 12, 13, 32, 33};
const int allowedPinsCount = sizeof(allowedPins) / sizeof(allowedPins[0]);
