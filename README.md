# Volcasync

ESP32 Korg Volca sync box experiments, including a Link-ready firmware architecture for standalone Wi-Fi sync.

## Current state

This repository currently contains the **v3A Link-ready Arduino sketch** discussed on 2026-03-29:

- internal BPM clock mode
- external Link-style injected timing mode
- web UI
- persistent settings
- pulse output for Volca sync
- architecture prepared for later real Ableton Link integration

## Notes

This is **not yet a full native Ableton Link ESP32 implementation**.
It is the clean product shell and timing engine for that future step.
