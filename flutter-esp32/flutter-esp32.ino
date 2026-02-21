#include <Arduino.h>
#include <NimBLEDevice.h>

static const char* DEVICE_NAME = "ESP32_LED";
static const char* SERVICE_UUID = "12345678-1234-1234-1234-1234567890ab";
static const char* LED_CHAR_UUID = "abcdefab-1234-1234-1234-abcdefabcdef";

// NEW: notify characteristic UUID (ESP32 -> phone)
static const char* VALUE_CHAR_UUID = "11111111-2222-3333-4444-555555555555";

static const int LED_PIN = 2;          // LED
static const int BTN_PIN = 0;          // BOOT button on most ESP32 dev boards (GPIO0)
static bool lastBtn = HIGH;

NimBLECharacteristic* valueChr = nullptr;

class LedCharCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    std::string v = pCharacteristic->getValue();
    if (v.length() == 0) return;
    uint8_t b = v[0];

    if (b == 1) {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED ON");
    } else {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED OFF");
    }
  }
};

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // BOOT button typically needs INPUT_PULLUP
  pinMode(BTN_PIN, INPUT_PULLUP);

  NimBLEDevice::init(DEVICE_NAME);

  NimBLEServer* server = NimBLEDevice::createServer();
  NimBLEService* service = server->createService(SERVICE_UUID);

  // LED write characteristic (phone -> ESP32)
  NimBLECharacteristic* ledChr = service->createCharacteristic(
    LED_CHAR_UUID,
    NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  ledChr->setCallbacks(new LedCharCallbacks());

  // VALUE notify characteristic (ESP32 -> phone)
  valueChr = service->createCharacteristic(
    VALUE_CHAR_UUID,
    NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ
  );

  // Optional: initial value
  int32_t initVal = 0;
  valueChr->setValue((uint8_t*)&initVal, sizeof(initVal));

  service->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setName(DEVICE_NAME);
  adv->enableScanResponse(true);
  adv->start();

  Serial.println("BLE Advertising started...");

  // seed random
  randomSeed((uint32_t)esp_random());
}

void loop() {
  // Simple debounce-ish edge detect
  bool btn = digitalRead(BTN_PIN);

  // detect press (HIGH -> LOW)
  if (lastBtn == HIGH && btn == LOW) {
    int32_t rnd = random(0, 1000);  // random int 0..999
    Serial.printf("BOOT pressed, send: %ld\n", (long)rnd);

    if (valueChr) {
      valueChr->setValue((uint8_t*)&rnd, sizeof(rnd));  // send as 4 bytes (int32 little-endian)
      valueChr->notify();
    }
  }

  lastBtn = btn;
  delay(30);
}
