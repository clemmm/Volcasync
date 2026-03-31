#include "clock_engine.h"
#include "config.h"

esp_timer_handle_t pulseTimer = nullptr;

bool isAllowedPin(int pin) {
  for (int i = 0; i < allowedPinsCount; i++) {
    if (allowedPins[i] == pin) return true;
  }
  return false;
}

double getCurrentTempoBpm() {
  if (state.clockSource == CLOCK_LINK_EXTERNAL && state.linkState.locked) {
    return state.linkState.tempoBpm;
  }
  return state.bpm;
}

double beatDurationUs(double tempo) {
  return 60000000.0 / tempo;
}

double getPredictedBeatNow() {
  uint64_t nowUs = esp_timer_get_time();

  if (state.clockSource == CLOCK_LINK_EXTERNAL && state.linkState.locked) {
    double elapsedUs = (double)(nowUs - state.linkState.lastSyncUs);
    double beatsElapsed = elapsedUs / beatDurationUs(state.linkState.tempoBpm);
    return state.linkState.beatPhase + beatsElapsed;
  }

  double elapsedUs = (double)(nowUs - state.internalBeatRefUs);
  double beatsElapsed = elapsedUs / beatDurationUs(state.bpm);
  return state.internalBeatPhase + beatsElapsed;
}

double getPulseGrid() {
  return (double)state.pulsesPerBeat;
}

double getNextPulseBoundary(double currentBeat) {
  double grid = getPulseGrid();
  return ceil(currentBeat * grid) / grid;
}

uint64_t microsUntilBeat(double currentBeat, double targetBeat, double tempoBpm) {
  double beatsToGo = targetBeat - currentBeat;
  if (beatsToGo < 0.0) beatsToGo = 0.0;
  return (uint64_t)(beatsToGo * beatDurationUs(tempoBpm));
}

void IRAM_ATTR setOutputs(bool high) {
  digitalWrite(state.syncPin, high ? HIGH : LOW);

  if (state.ledEnabled) {
    digitalWrite(LED_PIN, high ? HIGH : LOW);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

void stopClockOutputs() {
  setOutputs(false);
  state.pulseHigh = false;
}

void pulseTimerCallback(void* arg) {
  if (!state.clockRunning) {
    stopClockOutputs();
    return;
  }

  if (!state.pulseHigh) {
    setOutputs(true);
    state.pulseHigh = true;

    esp_timer_stop(pulseTimer);
    esp_timer_start_once(pulseTimer, (uint64_t)state.pulseWidthMs * 1000ULL);
  } else {
    setOutputs(false);
    state.pulseHigh = false;
    scheduleNextPulseStart();
  }
}

void scheduleNextPulseStart() {
  if (!pulseTimer || !state.clockRunning) return;

  double tempo = getCurrentTempoBpm();
  double currentBeat = getPredictedBeatNow();
  double candidate = getNextPulseBoundary(currentBeat);

  if (candidate <= currentBeat + 0.000001) {
    candidate += 1.0 / getPulseGrid();
  }

  state.nextPulseBeat = candidate;

  uint64_t waitUs = microsUntilBeat(currentBeat, state.nextPulseBeat, tempo);
  if (waitUs < 1000) waitUs = 1000;

  esp_timer_stop(pulseTimer);
  esp_timer_start_once(pulseTimer, waitUs);
}

void restartPulseTimer() {
  if (!pulseTimer) return;

  esp_timer_stop(pulseTimer);
  stopClockOutputs();

  if (state.clockRunning) {
    scheduleNextPulseStart();
  }
}

void resetInternalPhaseNow() {
  state.internalBeatPhase = 0.0;
  state.internalBeatRefUs = esp_timer_get_time();
}

void setSyncPin(int newPin) {
  if (!isAllowedPin(newPin) || newPin == state.syncPin) return;

  stopClockOutputs();
  pinMode(state.syncPin, INPUT);

  state.syncPin = newPin;

  pinMode(state.syncPin, OUTPUT);
  digitalWrite(state.syncPin, LOW);
}

void updateLinkTimeout() {
  if (state.clockSource == CLOCK_LINK_EXTERNAL && state.linkState.locked) {
    uint64_t nowUs = esp_timer_get_time();
    if (state.linkState.lastSyncUs > 0 &&
        (nowUs - state.linkState.lastSyncUs) > LINK_TIMEOUT_US) {
      state.linkState.locked = false;
    }
  }
}

void setupPulseTimer() {
  esp_timer_create_args_t timerArgs = {};
  timerArgs.callback = &pulseTimerCallback;
  timerArgs.arg = nullptr;
  timerArgs.dispatch_method = ESP_TIMER_TASK;
  timerArgs.name = "pulse-timer";
  esp_timer_create(&timerArgs, &pulseTimer);
}

void initClockEngine() {
  pinMode(state.syncPin, OUTPUT);
  digitalWrite(state.syncPin, LOW);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  resetInternalPhaseNow();
  setupPulseTimer();
}
