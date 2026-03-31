#include "config.h"
#include "app_state.h"
#include "storage.h"
#include "clock_engine.h"
#include "network.h"
#include "api_server.h"

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(250);

  loadSettings();

  initClockEngine();
  initWiFi();
  initMDNS();
  initApiServer();

  if (state.clockRunning) {
    restartPulseTimer();
  }

  Serial.println("Volca Sync Box ready");
}

void loop() {
  handleApiServer();
  updateLinkTimeout();
}
