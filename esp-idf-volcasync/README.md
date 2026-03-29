# ESP-IDF real Ableton Link milestone

This folder is the **real Ableton Link integration path** for Volcasync.

## Goal

Make the ESP32:
- connect to Wi-Fi
- join the same Ableton Link session as djay / Ableton Live peers
- log peer count, tempo, beat and phase
- become the foundation for real Volca sync output driven by Link

## Stack

- ESP-IDF
- `docwilco/esp_abl_link`
- dual-core ESP32 target recommended

## Current status

This is the real-integration path, not a fake “done” implementation.
You should expect minor API adjustments depending on the exact `esp_abl_link` version.

## Build outline

```bash
idf.py set-target esp32
idf.py menuconfig
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

In `menuconfig`, set your Wi-Fi credentials under the example connection configuration.

## Milestone 3

The current `main.c` is now prepared for **Milestone 3**:
- connects to Wi-Fi
- joins Ableton Link
- logs peer count / tempo / beat / phase
- pulses the onboard LED on each whole Link beat
- outputs a simple sync pulse on **GPIO25** on each whole beat

### Test expectation

With djay or another Ableton Link peer active on the same Wi-Fi:
- peer count should rise above 0
- beat should advance continuously
- the LED on GPIO2 should pulse on each beat
- GPIO25 should output a short pulse once per beat

### Wiring for first hardware test

- **GPIO25** -> resistor -> **tip** of 3.5mm sync jack
- **GND** -> **sleeve** of sync jack

This is still a first honest sync milestone: simple whole-beat pulse output, not yet a refined Volca pulse scheduler with configurable PPB.
