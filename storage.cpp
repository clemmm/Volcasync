#include <Preferences.h>
#include "storage.h"
#include "app_state.h"
#include "config.h"
#include "clock_engine.h"

Preferences prefs;

void saveSettings() {
  prefs.begin("volca-sync", false);
  prefs.putFloat("bpm", state.bpm);
  prefs.putInt("ppb", state.pulsesPerBeat);
  prefs.putInt("width", state.pulseWidthMs);
  prefs.putInt("pin", state.syncPin);
  prefs.putBool("led", state.ledEnabled);
  prefs.putBool("running", state.clockRunning);
  prefs.putInt("source", (int)state.clockSource);
  prefs.end();
}

void loadSettings() {
  prefs.begin("volca-sync", true);
  state.bpm = prefs.getFloat("bpm", DEFAULT_BPM);
  state.pulsesPerBeat = prefs.getInt("ppb", DEFAULT_PPB);
  state.pulseWidthMs = prefs.getInt("width", DEFAULT_PULSE_WIDTH_MS);
  state.syncPin = prefs.getInt("pin", DEFAULT_SYNC_PIN);
  state.ledEnabled = prefs.getBool("led", true);
  state.clockRunning = prefs.getBool("running", false);
  state.clockSource = (ClockSource)prefs.getInt("source", 0);
  prefs.end();

  if (state.bpm < 20.0 || state.bpm > 300.0) state.bpm = DEFAULT_BPM;
  if (state.pulsesPerBeat < 1 || state.pulsesPerBeat > 24) state.pulsesPerBeat = DEFAULT_PPB;
  if (state.pulseWidthMs < 1 || state.pulseWidthMs > 100) state.pulseWidthMs = DEFAULT_PULSE_WIDTH_MS;
  if (!isAllowedPin(state.syncPin)) state.syncPin = DEFAULT_SYNC_PIN;
  if (!(state.clockSource == CLOCK_INTERNAL || state.clockSource == CLOCK_LINK_EXTERNAL)) {
    state.clockSource = CLOCK_INTERNAL;
  }
}
