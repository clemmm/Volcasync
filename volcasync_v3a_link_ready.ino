#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <esp_timer.h>
#include <math.h>

/*
  ESP32 Volca Sync Box v3A - Link Ready
  -------------------------------------
  - INTERNAL clock mode
  - LINK_EXTERNAL mode
  - External tempo/beat injection API
  - Beat prediction between updates
  - Web UI
  - Saved settings
  - mDNS
*/

const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
const char* MDNS_NAME = "volca-sync";

WebServer server(80);
Preferences prefs;

enum ClockSource {
  CLOCK_INTERNAL = 0,
  CLOCK_LINK_EXTERNAL = 1
};

ClockSource clockSource = CLOCK_INTERNAL;

float bpm = 120.0;
int pulsesPerBeat = 2;
int pulseWidthMs = 15;
int syncPin = 25;
bool clockRunning = false;
bool ledEnabled = true;
const int LED_PIN = 2;

const int allowedPins[] = {25, 26, 27, 14, 12, 13, 32, 33};
const int allowedPinsCount = sizeof(allowedPins) / sizeof(allowedPins[0]);

struct LinkState {
  bool locked = false;
  double tempoBpm = 120.0;
  double beatPhase = 0.0;
  uint64_t lastSyncUs = 0;
};

LinkState linkState;

double internalBeatPhase = 0.0;
uint64_t internalBeatRefUs = 0;

volatile bool pulseHigh = false;
esp_timer_handle_t pulseTimer = nullptr;
double nextPulseBeat = 0.0;

bool isAllowedPin(int pin) {
  for (int i = 0; i < allowedPinsCount; i++) {
    if (allowedPins[i] == pin) return true;
  }
  return false;
}

double getCurrentTempoBpm() {
  if (clockSource == CLOCK_LINK_EXTERNAL && linkState.locked) {
    return linkState.tempoBpm;
  }
  return bpm;
}

double beatDurationUs(double tempo) {
  return 60000000.0 / tempo;
}

double getPredictedBeatNow() {
  uint64_t nowUs = esp_timer_get_time();

  if (clockSource == CLOCK_LINK_EXTERNAL && linkState.locked) {
    double elapsedUs = (double)(nowUs - linkState.lastSyncUs);
    double beatsElapsed = elapsedUs / beatDurationUs(linkState.tempoBpm);
    return linkState.beatPhase + beatsElapsed;
  }

  double elapsedUs = (double)(nowUs - internalBeatRefUs);
  double beatsElapsed = elapsedUs / beatDurationUs(bpm);
  return internalBeatPhase + beatsElapsed;
}

double getPulseGrid() {
  return (double)pulsesPerBeat;
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
  digitalWrite(syncPin, high ? HIGH : LOW);
  if (ledEnabled) {
    digitalWrite(LED_PIN, high ? HIGH : LOW);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

void stopClockOutputs() {
  setOutputs(false);
  pulseHigh = false;
}

void scheduleNextPulseStart();

void pulseTimerCallback(void* arg) {
  if (!clockRunning) {
    stopClockOutputs();
    return;
  }

  if (!pulseHigh) {
    setOutputs(true);
    pulseHigh = true;

    esp_timer_stop(pulseTimer);
    esp_timer_start_once(pulseTimer, (uint64_t)pulseWidthMs * 1000ULL);
  } else {
    setOutputs(false);
    pulseHigh = false;
    scheduleNextPulseStart();
  }
}

void scheduleNextPulseStart() {
  if (!pulseTimer || !clockRunning) return;

  double tempo = getCurrentTempoBpm();
  double currentBeat = getPredictedBeatNow();
  double candidate = getNextPulseBoundary(currentBeat);

  if (candidate <= currentBeat + 0.000001) {
    candidate += 1.0 / getPulseGrid();
  }

  nextPulseBeat = candidate;

  uint64_t waitUs = microsUntilBeat(currentBeat, nextPulseBeat, tempo);
  if (waitUs < 1000) waitUs = 1000;

  esp_timer_stop(pulseTimer);
  esp_timer_start_once(pulseTimer, waitUs);
}

void restartPulseTimer() {
  if (!pulseTimer) return;

  esp_timer_stop(pulseTimer);
  stopClockOutputs();

  if (clockRunning) {
    scheduleNextPulseStart();
  }
}

void resetInternalPhaseNow() {
  internalBeatPhase = 0.0;
  internalBeatRefUs = esp_timer_get_time();
}

void saveSettings() {
  prefs.begin("volca-sync", false);
  prefs.putFloat("bpm", bpm);
  prefs.putInt("ppb", pulsesPerBeat);
  prefs.putInt("width", pulseWidthMs);
  prefs.putInt("pin", syncPin);
  prefs.putBool("led", ledEnabled);
  prefs.putBool("running", clockRunning);
  prefs.putInt("source", (int)clockSource);
  prefs.end();
}

void loadSettings() {
  prefs.begin("volca-sync", true);
  bpm = prefs.getFloat("bpm", 120.0);
  pulsesPerBeat = prefs.getInt("ppb", 2);
  pulseWidthMs = prefs.getInt("width", 15);
  syncPin = prefs.getInt("pin", 25);
  ledEnabled = prefs.getBool("led", true);
  clockRunning = prefs.getBool("running", false);
  clockSource = (ClockSource)prefs.getInt("source", 0);
  prefs.end();

  if (bpm < 20.0 || bpm > 300.0) bpm = 120.0;
  if (pulsesPerBeat < 1 || pulsesPerBeat > 24) pulsesPerBeat = 2;
  if (pulseWidthMs < 1 || pulseWidthMs > 100) pulseWidthMs = 15;
  if (!isAllowedPin(syncPin)) syncPin = 25;
  if (!(clockSource == CLOCK_INTERNAL || clockSource == CLOCK_LINK_EXTERNAL)) {
    clockSource = CLOCK_INTERNAL;
  }
}

String statusJson() {
  String json = "{";
  json += "\"running\":" + String(clockRunning ? "true" : "false") + ",";
  json += "\"clockSource\":\"" + String(clockSource == CLOCK_INTERNAL ? "internal" : "link") + "\",";
  json += "\"bpm\":" + String(bpm, 1) + ",";
  json += "\"activeTempo\":" + String(getCurrentTempoBpm(), 3) + ",";
  json += "\"pulsesPerBeat\":" + String(pulsesPerBeat) + ",";
  json += "\"pulseWidthMs\":" + String(pulseWidthMs) + ",";
  json += "\"syncPin\":" + String(syncPin) + ",";
  json += "\"ledEnabled\":" + String(ledEnabled ? "true" : "false") + ",";
  json += "\"predictedBeat\":" + String(getPredictedBeatNow(), 6) + ",";
  json += "\"nextPulseBeat\":" + String(nextPulseBeat, 6) + ",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
  json += "}";
  return json;
}

String linkStatusJson() {
  uint64_t nowUs = esp_timer_get_time();
  uint64_t ageUs = linkState.lastSyncUs == 0 ? 0 : (nowUs - linkState.lastSyncUs);

  String json = "{";
  json += "\"locked\":" + String(linkState.locked ? "true" : "false") + ",";
  json += "\"tempoBpm\":" + String(linkState.tempoBpm, 3) + ",";
  json += "\"beatPhase\":" + String(linkState.beatPhase, 6) + ",";
  json += "\"lastSyncUs\":" + String((unsigned long long)linkState.lastSyncUs) + ",";
  json += "\"ageUs\":" + String((unsigned long long)ageUs);
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

void handleRoot() { server.send(200, "text/html", htmlPage()); }
void handleStatus() { server.send(200, "application/json", statusJson()); }
void handleLinkStatus() { server.send(200, "application/json", linkStatusJson()); }

void handleApiSet() {
  if (server.hasArg("bpm")) {
    float newBpm = server.arg("bpm").toFloat();
    if (newBpm >= 20.0 && newBpm <= 300.0) bpm = newBpm;
  }
  if (server.hasArg("ppb")) {
    int newPpb = server.arg("ppb").toInt();
    if (newPpb >= 1 && newPpb <= 24) pulsesPerBeat = newPpb;
  }
  if (server.hasArg("width")) {
    int newWidth = server.arg("width").toInt();
    if (newWidth >= 1 && newWidth <= 100) pulseWidthMs = newWidth;
  }
  if (server.hasArg("led")) {
    ledEnabled = server.arg("led").toInt() == 1;
  }
  if (server.hasArg("pin")) {
    int newPin = server.arg("pin").toInt();
    if (isAllowedPin(newPin) && newPin != syncPin) {
      stopClockOutputs();
      pinMode(syncPin, INPUT);
      syncPin = newPin;
      pinMode(syncPin, OUTPUT);
      digitalWrite(syncPin, LOW);
    }
  }
  if (server.hasArg("source")) {
    String src = server.arg("source");
    if (src == "internal") clockSource = CLOCK_INTERNAL;
    if (src == "link") clockSource = CLOCK_LINK_EXTERNAL;
  }
  saveSettings();
  restartPulseTimer();
  server.send(200, "application/json", statusJson());
}

void handleClockSource() {
  if (server.hasArg("mode")) {
    String mode = server.arg("mode");
    if (mode == "internal") clockSource = CLOCK_INTERNAL;
    if (mode == "link") clockSource = CLOCK_LINK_EXTERNAL;
  }
  saveSettings();
  restartPulseTimer();
  server.send(200, "application/json", statusJson());
}

void handleLinkUpdate() {
  if (server.hasArg("tempo")) {
    double t = server.arg("tempo").toDouble();
    if (t >= 20.0 && t <= 300.0) linkState.tempoBpm = t;
  }
  if (server.hasArg("beat")) {
    linkState.beatPhase = server.arg("beat").toDouble();
  }
  if (server.hasArg("locked")) {
    linkState.locked = (server.arg("locked").toInt() == 1);
  }
  linkState.lastSyncUs = esp_timer_get_time();
  if (clockSource == CLOCK_LINK_EXTERNAL && clockRunning) {
    restartPulseTimer();
  }
  server.send(200, "application/json", linkStatusJson());
}

void handleStart() {
  clockRunning = true;
  if (clockSource == CLOCK_INTERNAL) {
    resetInternalPhaseNow();
  }
  saveSettings();
  restartPulseTimer();
  server.send(200, "application/json", statusJson());
}

void handleStop() {
  clockRunning = false;
  saveSettings();
  restartPulseTimer();
  server.send(200, "application/json", statusJson());
}

void handleReset() {
  bpm = 120.0;
  pulsesPerBeat = 2;
  pulseWidthMs = 15;
  ledEnabled = true;
  clockRunning = false;
  clockSource = CLOCK_INTERNAL;

  if (syncPin != 25) {
    stopClockOutputs();
    pinMode(syncPin, INPUT);
    syncPin = 25;
    pinMode(syncPin, OUTPUT);
    digitalWrite(syncPin, LOW);
  }

  resetInternalPhaseNow();
  linkState.locked = false;
  linkState.tempoBpm = 120.0;
  linkState.beatPhase = 0.0;
  linkState.lastSyncUs = 0;

  saveSettings();
  restartPulseTimer();
  server.send(200, "application/json", statusJson());
}

void handlePhaseReset() {
  if (clockSource == CLOCK_INTERNAL) {
    resetInternalPhaseNow();
  } else if (clockSource == CLOCK_LINK_EXTERNAL) {
    linkState.beatPhase = floor(linkState.beatPhase);
    linkState.lastSyncUs = esp_timer_get_time();
  }
  restartPulseTimer();
  server.send(200, "application/json", statusJson());
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Wi-Fi connected. IP: ");
  Serial.println(WiFi.localIP());
}

void setupMDNS() {
  if (MDNS.begin(MDNS_NAME)) {
    Serial.print("mDNS: http://");
    Serial.print(MDNS_NAME);
    Serial.println(".local");
  } else {
    Serial.println("mDNS start failed");
  }
}

void setupServer() {
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

void setupPulseTimer() {
  esp_timer_create_args_t timerArgs = {};
  timerArgs.callback = &pulseTimerCallback;
  timerArgs.arg = nullptr;
  timerArgs.dispatch_method = ESP_TIMER_TASK;
  timerArgs.name = "pulse-timer";
  esp_timer_create(&timerArgs, &pulseTimer);
}

void setup() {
  Serial.begin(115200);
  delay(250);

  loadSettings();

  pinMode(syncPin, OUTPUT);
  digitalWrite(syncPin, LOW);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  resetInternalPhaseNow();
  setupPulseTimer();
  setupWiFi();
  setupMDNS();
  setupServer();

  if (clockRunning) {
    restartPulseTimer();
  }

  Serial.println("Volca Sync Box v3A ready");
}

void loop() {
  server.handleClient();

  if (clockSource == CLOCK_LINK_EXTERNAL && linkState.locked) {
    uint64_t nowUs = esp_timer_get_time();
    if (linkState.lastSyncUs > 0 && (nowUs - linkState.lastSyncUs) > 3000000ULL) {
      linkState.locked = false;
    }
  }
}
