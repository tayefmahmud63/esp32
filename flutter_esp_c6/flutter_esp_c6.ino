#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NimBLEDevice.h>

// ===== OLED =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_SDA D4
#define OLED_SCL D8
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

static void oledShowPairing() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 8);
  display.print("Pairing");
  display.display();
}

static void oledShowTime(uint32_t sec) {
  uint32_t h = sec / 3600;
  uint32_t m = (sec % 3600) / 60;

  char buf[16];
  snprintf(buf, sizeof(buf), "%02luh %02lum", (unsigned long)h, (unsigned long)m);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("Clip Connected");

  display.setTextSize(2);
  display.setCursor(0, 14);
  display.print(buf);

  display.display();
}

// ===== BLE UUIDs =====
static const char* DEVICE_NAME     = "Escape Clip";
static const char* SERVICE_UUID    = "12345678-1234-1234-1234-1234567890ab";
static const char* LED_CHAR_UUID   = "abcdefab-1234-1234-1234-abcdefabcdef";
static const char* VALUE_CHAR_UUID = "11111111-2222-3333-4444-555555555555";

// Pins (keep your working pins)
static const int LED_PIN = 2;
static const int BTN_PIN = 0;

static bool g_connected = false;
static uint32_t startMs = 0;
static uint32_t lastTickMs = 0;

NimBLECharacteristic* valueChr = nullptr;

class MyServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    g_connected = true;
    startMs = millis();       // ✅ timer starts at connect
    lastTickMs = 0;
    Serial.println("BLE Connected");

    oledShowTime(0);

    // Optionally send 0 immediately
    if (valueChr) {
      int32_t zero = 0;
      valueChr->setValue((uint8_t*)&zero, sizeof(zero));
      valueChr->notify();
    }
  }

  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    g_connected = false;
    Serial.printf("BLE Disconnected (reason=%d)\n", reason);
    NimBLEDevice::startAdvertising();
    oledShowPairing();
  }
};

class LedCharCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    std::string v = pCharacteristic->getValue();
    if (v.empty()) return;
    uint8_t b = (uint8_t)v[0];

    digitalWrite(LED_PIN, b == 1 ? HIGH : LOW);
    Serial.println(b == 1 ? "LED ON" : "LED OFF");
  }
};

static void setupOLED() {
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found (try 0x3D)");
    while (true) delay(100);
  }
  display.clearDisplay();
  display.display();
}

static void setupBLE() {
  NimBLEDevice::init(DEVICE_NAME);

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new MyServerCallbacks());

  NimBLEService* service = server->createService(SERVICE_UUID);

  // Phone -> ESP32 LED write
  NimBLECharacteristic* ledChr = service->createCharacteristic(
    LED_CHAR_UUID,
    NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  ledChr->setCallbacks(new LedCharCallbacks());

  // ESP32 -> Phone notify time (int32 seconds)
  valueChr = service->createCharacteristic(
    VALUE_CHAR_UUID,
    NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ
  );

  int32_t initVal = 0;
  valueChr->setValue((uint8_t*)&initVal, sizeof(initVal));

  service->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setName(DEVICE_NAME);
  adv->enableScanResponse(true);
  adv->start();

  Serial.println("BLE Advertising started...");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(BTN_PIN, INPUT_PULLUP);

  setupOLED();
  oledShowPairing();

  setupBLE();
}

void loop() {
  // ✅ Update time + notify once per second while connected
  if (g_connected) {
    uint32_t now = millis();
    if (now - lastTickMs >= 1000) {
      lastTickMs = now;

      uint32_t elapsedSec = (now - startMs) / 1000;

      // OLED shows hh/mm
      oledShowTime(elapsedSec);

      // Send int32 seconds to phone
      if (valueChr) {
        int32_t sendSec = (int32_t)elapsedSec;
        valueChr->setValue((uint8_t*)&sendSec, sizeof(sendSec));
        valueChr->notify();
      }
    }
  }

  delay(10);
}
