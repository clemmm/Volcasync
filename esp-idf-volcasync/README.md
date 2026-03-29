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

This is a **Milestone 1 scaffold**.
It is the honest real-integration starting point, not a fake “done” implementation.

You should expect minor API adjustments depending on the exact `esp_abl_link` version.

## Build outline

```bash
idf.py set-target esp32
idf.py menuconfig
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

In `menuconfig`, set your Wi-Fi credentials under the example connection configuration.

## Next milestone

Once Link peer visibility works, add the Volca pulse scheduler as the next step.

## Milestone 2

The current `main.c` is now prepared for **Milestone 2**:
- connects to Wi-Fi
- joins Ableton Link
- logs peer count / tempo / beat / phase
- pulses the onboard LED once per whole Link beat

### Test expectation

With djay or another Ableton Link peer active on the same Wi-Fi:
- peer count should rise above 0
- beat should advance continuously
- the LED on GPIO2 should pulse on each beat

This is the step right before replacing the LED pulse with Volca sync pulse output.
