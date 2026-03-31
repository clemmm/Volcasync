#include <WiFi.h>
#include <ESPmDNS.h>
#include "network.h"
#include "config.h"

void initWiFi() {
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

void initMDNS() {
  if (MDNS.begin(MDNS_NAME)) {
    Serial.print("mDNS: http://");
    Serial.print(MDNS_NAME);
    Serial.println(".local");
  } else {
    Serial.println("mDNS start failed");
  }
}
