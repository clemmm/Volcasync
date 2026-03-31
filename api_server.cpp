#include <WiFi.h>
#include <WebServer.h>
#include <math.h>
#include <esp_timer.h>

#include "api_server.h"
#include "app_state.h"
#include "clock_engine.h"
#include "storage.h"

WebServer server(80);

String statusJson() {
  String json = "{";
  json += "\"running\":" + String(state.clockRunning ? "true" : "false") + ",";
  json += "\"clockSource\":\"" + String(state.clockSource == CLOCK_INTERNAL ? "internal" : "link") + "\",";
  json += "\"bpm\":" + String(state.bpm, 1) + ",";
  json += "\"activeTempo\":" + String(getCurrentTempoBpm(), 3) + ",";
  json += "\"pulsesPerBeat\":" + String(state.pulsesPerBeat) + ",";
  json += "\"pulseWidthMs\":" + String(state.pulseWidthMs) + ",";
  json += "\"syncPin\":" + String(state.syncPin) + ",";
  json += "\"ledEnabled\":" + String(state.ledEnabled ? "true" : "false") + ",";
  json += "\"predictedBeat\":" + String(getPredictedBeatNow(), 6) + ",";
  json += "\"nextPulseBeat\":" + String(state.nextPulseBeat, 6) + ",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
  json += "}";
  return json;
}

String linkStatusJson() {
  uint64_t nowUs = esp_timer_get_time();
  uint64_t ageUs = state.linkState.lastSyncUs == 0 ? 0 : (nowUs - state.linkState.lastSyncUs);

  String json = "{";
  json += "\"locked\":" + String(state.linkState.locked ? "true" : "false") + ",";
  json += "\"tempoBpm\":" + String(state.linkState.tempoBpm, 3) + ",";
  json += "\"beatPhase\":" + String(state.linkState.beatPhase, 6) + ",";
  json += "\"lastSyncUs\":" + String((double)state.linkState.lastSyncUs, 0) + ",";
  json += "\"ageUs\":" + String((double)ageUs, 0);
  json += "}";
  return json;
}

String htmlPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Volca Sync Box</title>
</head>
<body>
<h1>Volca Sync Box</h1>
<p>Link-ready test build.</p>
</body>
</html>
)rawliteral";
}

void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleStatus() {
  server.send(200, "application/json", statusJson());
}

void handleLinkStatus() {
  server.send(200, "application/json", linkStatusJson());
}

void handleApiSet() {
  if (server.hasArg("bpm")) {
    float newBpm = server.arg("bpm").toFloat();
    if (newBpm >= 20.0 && newBpm <= 300.0) state.bpm = newBpm;
  }

  if (server.hasArg("ppb")) {
    int newPpb = server.arg("ppb").toInt();
    if (newPpb >= 1 && newPpb <= 24) state.pulsesPerBeat = newPpb;
  }

  if (server.hasArg("width")) {
    int newWidth = server.arg("width").toInt();
    if (newWidth >= 1 && newWidth <= 100) state.pulseWidthMs = newWidth;
  }

  if (server.hasArg("led")) {
    state.ledEnabled = server.arg("led").toInt() == 1;
  }

  if (server.hasArg("pin")) {
    int newPin = server.arg("pin").toInt();
    setSyncPin(newPin);
  }

  if (server.hasArg("source")) {
    String src = server.arg("source");
    if (src == "internal") state.clockSource = CLOCK_INTERNAL;
    if (src == "link") state.clockSource = CLOCK_LINK_EXTERNAL;
  }

  saveSettings();
  restartPulseTimer();
  server.send(200, "application/json", statusJson());
}

void handleClockSource() {
  if (server.hasArg("mode")) {
    String mode = server.arg("mode");
    if (mode == "internal") state.clockSource = CLOCK_INTERNAL;
    if (mode == "link") state.clockSource = CLOCK_LINK_EXTERNAL;
  }

  saveSettings();
  restartPulseTimer();
  server.send(200, "application/json", statusJson());
}

void handleLinkUpdate() {
  if (server.hasArg("tempo")) {
    double t = server.arg("tempo").toDouble();
    if (t >= 20.0 && t <= 300.0) state.linkState.tempoBpm = t;
  }

  if (server.hasArg("beat")) {
    state.linkState.beatPhase = server.arg("beat").toDouble();
  }

  if (server.hasArg("locked")) {
    state.linkState.locked = (server.arg("locked").toInt() == 1);
  }

  state.linkState.lastSyncUs = esp_timer_get_time();

  if (state.clockSource == CLOCK_LINK_EXTERNAL && state.clockRunning) {
    restartPulseTimer();
  }

  server.send(200, "application/json", linkStatusJson());
}

void handleStart() {
  state.clockRunning = true;

  if (state.clockSource == CLOCK_INTERNAL) {
    resetInternalPhaseNow();
  }

  saveSettings();
  restartPulseTimer();
  server.send(200, "application/json", statusJson());
}

void handleStop() {
  state.clockRunning = false;
  saveSettings();
  restartPulseTimer();
  server.send(200, "application/json", statusJson());
}

void handleReset() {
  state.bpm = 120.0f;
  state.pulsesPerBeat = 2;
  state.pulseWidthMs = 15;
  state.ledEnabled = true;
  state.clockRunning = false;
  state.clockSource = CLOCK_INTERNAL;

  if (state.syncPin != 25) {
    setSyncPin(25);
  }

  resetInternalPhaseNow();
  state.linkState.locked = false;
  state.linkState.tempoBpm = 120.0;
  state.linkState.beatPhase = 0.0;
  state.linkState.lastSyncUs = 0;

  saveSettings();
  restartPulseTimer();
  server.send(200, "application/json", statusJson());
}

void handlePhaseReset() {
  if (state.clockSource == CLOCK_INTERNAL) {
    resetInternalPhaseNow();
  } else if (state.clockSource == CLOCK_LINK_EXTERNAL) {
    state.linkState.beatPhase = floor(state.linkState.beatPhase);
    state.linkState.lastSyncUs = esp_timer_get_time();
  }

  restartPulseTimer();
  server.send(200, "application/json", statusJson());
}

void initApiServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/api/link-status", HTTP_GET, handleLinkStatus);
  server.on("/api/set", HTTP_POST, handleApiSet);
  server.on("/api/clock-source", HTTP_POST, handleClockSource);
  server.on("/api/link-update", HTTP_POST, handleLinkUpdate);
  server.on("/start", HTTP_POST, handleStart);
  server.on("/stop", HTTP_POST, handleStop);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/phase-reset", HTTP_POST, handlePhaseReset);

  server.begin();
  Serial.println("HTTP server started");
}

void handleApiServer() {
  server.handleClient();
}
