#include <WiFi.h>
#include <time.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>

#define WIFI_SSID     "ssid"
#define WIFI_PASS     "password"

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES   4
#define CLK_PIN       18
#define DATA_PIN      23
#define CS_PIN        21

#define NTP_SERVER1   "0.bd.pool.ntp.org"
#define NTP_SERVER2   "pool.ntp.org"
#define NTP_SERVER3   "time.google.com"
#define GMT_OFFSET    21600
#define DAYLIGHT_OFFSET 0

MD_Parola display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

char timeStr[8];
char prevStr[8] = "";

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting WiFi");
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nFailed — restarting");
    delay(2000);
    ESP.restart();
  }
}

bool syncNTP() {
  configTime(GMT_OFFSET, DAYLIGHT_OFFSET, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
  struct tm t;
  int tries = 0;
  while (tries < 20) {
    if (getLocalTime(&t, 1000)) return true;
    Serial.print(".");
    tries++;
    delay(500);
  }
  return false;
}

void setup() {
  Serial.begin(115200);

  display.begin();
  display.setIntensity(5);
  display.displayClear();
  display.setTextAlignment(PA_CENTER);
  display.print("CONN");

  connectWiFi();
  display.print("NTP.");

  if (!syncNTP()) {
    display.print("ERR");
    delay(10000);
    ESP.restart();
  }

  display.displayClear();
}

void loop() {
  static unsigned long lastSync = 0;
  if (millis() - lastSync > 6UL * 3600UL * 1000UL) {
    syncNTP();
    lastSync = millis();
  }

  struct tm t;
  if (!getLocalTime(&t, 500)) {
    delay(1000);
    return;
  }

  // Blink colon every second
  bool colonOn = (t.tm_sec % 2 == 0);

  // Only HH:MM — no seconds
  snprintf(timeStr, sizeof(timeStr),
    "%02d%s%02d",
    t.tm_hour,
    colonOn ? ":" : " ",
    t.tm_min
  );

  if (strcmp(timeStr, prevStr) != 0) {
    display.setTextAlignment(PA_CENTER);   // centered on all 4 modules
    display.print(timeStr);
    strcpy(prevStr, timeStr);
  }

  delay(200);
}