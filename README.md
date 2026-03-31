# Volcasync

ESP32 Korg Volca sync box experiments, now reorganized as a modular Arduino IDE project.

## Current state

This repository now contains a refactored **ESP32 Arduino** implementation split into small modules:

- `VolcaSyncBox.ino` — Arduino entry point
- `clock_engine.*` — timing engine and pulse scheduling
- `api_server.*` — web UI and HTTP API
- `network.*` — Wi-Fi and mDNS setup
- `storage.*` — persistent settings with Preferences
- `app_state.*` — shared runtime state
- `config.h` / `secrets.h` — constants and local Wi-Fi credentials

## Features

- internal BPM clock mode
- external Link-style injected timing mode
- beat prediction between updates
- precise pulse scheduling with `esp_timer`
- web UI placeholder
- HTTP API
- persistent settings
- mDNS
- Volca sync pulse output on configurable GPIO

## Notes

- This remains an Arduino-first ESP32 project.
- `esp_timer` is intentionally kept for timing precision.
- `secrets.h` is currently committed as a template and should be edited locally with your Wi-Fi credentials.
- The previous monolithic sketch is replaced by a modular codebase intended as the new project baseline.
