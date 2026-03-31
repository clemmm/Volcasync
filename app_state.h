#pragma once
#include <stdint.h>

enum ClockSource {
  CLOCK_INTERNAL = 0,
  CLOCK_LINK_EXTERNAL = 1
};

struct LinkState {
  bool locked = false;
  double tempoBpm = 120.0;
  double beatPhase = 0.0;
  uint64_t lastSyncUs = 0;
};

struct AppState {
  ClockSource clockSource = CLOCK_INTERNAL;
  float bpm = 120.0f;
  int pulsesPerBeat = 2;
  int pulseWidthMs = 15;
  int syncPin = 25;
  bool clockRunning = false;
  bool ledEnabled = true;

  double internalBeatPhase = 0.0;
  uint64_t internalBeatRefUs = 0;

  volatile bool pulseHigh = false;
  double nextPulseBeat = 0.0;

  LinkState linkState;
};

extern AppState state;
