#pragma once

#include <Arduino.h>
#include <esp_timer.h>
#include "app_state.h"

extern esp_timer_handle_t pulseTimer;

bool isAllowedPin(int pin);

double getCurrentTempoBpm();
double beatDurationUs(double tempo);
double getPredictedBeatNow();
double getPulseGrid();
double getNextPulseBoundary(double currentBeat);
uint64_t microsUntilBeat(double currentBeat, double targetBeat, double tempoBpm);

void setOutputs(bool high);
void stopClockOutputs();

void initClockEngine();
void setupPulseTimer();
void scheduleNextPulseStart();
void restartPulseTimer();
void resetInternalPhaseNow();

void setSyncPin(int newPin);
void updateLinkTimeout();
