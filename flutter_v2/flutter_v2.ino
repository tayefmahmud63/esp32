#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NimBLEDevice.h>

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_SDA D4
#define OLED_SCL D8

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= Light Sensor =================
#define LIGHT_SENSOR_PIN D0
#define ON_THRESHOLD   25   // must be > OFF_THRESHOLD
#define OFF_THRESHOLD  15
#define LIGHT_THRESHOLD 15
#define OFF_DELAY 2000   // 3 seconds (you wrote 2000 but comment says 3s; using 3000)
bool blinkVisible = true;
unsigned long lastBlinkToggle = 0;
const unsigned long BLINK_INTERVAL = 500; // ms (0.5s ON/OFF)

bool screenOn = false;

unsigned long onStartTime = 0;
unsigned long totalOnTime = 0;

unsigned long lowStartTime = 0;
bool lowDetected = false;

unsigned long getScreenOnTimeMs() {
  unsigned long elapsed = totalOnTime;
  if (screenOn) elapsed += millis() - onStartTime;
  return elapsed;
}

const int FILTER_N = 10;
int samples[FILTER_N];
int sampleIdx = 0;
long sampleSum = 0;
bool filterInit = false;

int readLightFiltered() {
  int v = analogRead(LIGHT_SENSOR_PIN);

  if (!filterInit) {
    // initialize buffer with first value
    for (int i = 0; i < FILTER_N; i++) samples[i] = v;
    sampleSum = (long)v * FILTER_N;
    filterInit = true;
    return v;
  }

  sampleSum -= samples[sampleIdx];
  samples[sampleIdx] = v;
  sampleSum += v;
  sampleIdx = (sampleIdx + 1) % FILTER_N;

  return (int)(sampleSum / FILTER_N);
}

void updateScreenState() {
  int sensorValue = readLightFiltered();   // ✅ filtered ADC
  unsigned long now = millis();

  // ---------- TURN ON (hysteresis) ----------
  // Only turn ON when it's clearly bright enough
  if (!screenOn && sensorValue > ON_THRESHOLD) {
    screenOn = true;
    onStartTime = now;
    lowDetected = false;  // reset any pending off
    Serial.printf("Screen ON (adc=%d)\n", sensorValue);
    return;
  }

  // ---------- TURN OFF (hysteresis + delay) ----------
  // Only consider OFF when it's clearly dark enough
  if (screenOn && sensorValue < OFF_THRESHOLD) {
    if (!lowDetected) {
      lowDetected = true;
      lowStartTime = now;
    }

    if (now - lowStartTime >= OFF_DELAY) {
      screenOn = false;
      totalOnTime += now - onStartTime;
      lowDetected = false;
      Serial.printf("Screen OFF (adc=%d)\n", sensorValue);
    }
  } else {
    // Not low anymore → cancel off timer
    lowDetected = false;
  }
}


// ================= BLE =================
static const char* DEVICE_NAME     = "Escape Clip";
static const char* SERVICE_UUID    = "12345678-1234-1234-1234-1234567890ab";
static const char* LED_CHAR_UUID   = "abcdefab-1234-1234-1234-abcdefabcdef";
static const char* VALUE_CHAR_UUID = "11111111-2222-3333-4444-555555555555";

static const int LED_PIN = 2;
static const int BTN_PIN = 0;

static volatile bool g_connected = false;

NimBLECharacteristic* valueChr = nullptr;

// Pack notify payload: [int32 seconds little-endian][uint8 screenOn]
void notifyApp(uint32_t elapsedSeconds, bool isOn) {
  if (!g_connected || !valueChr) return;

  uint8_t payload[5];
  payload[0] = (uint8_t)(elapsedSeconds & 0xFF);
  payload[1] = (uint8_t)((elapsedSeconds >> 8) & 0xFF);
  payload[2] = (uint8_t)((elapsedSeconds >> 16) & 0xFF);
  payload[3] = (uint8_t)((elapsedSeconds >> 24) & 0xFF);
  payload[4] = isOn ? 1 : 0;

  valueChr->setValue(payload, sizeof(payload));
  valueChr->notify();
}

class MyServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    g_connected = true;
    Serial.println("BLE Connected");
    // immediately push initial state
    notifyApp(getScreenOnTimeMs() / 1000, screenOn);
  }

  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    g_connected = false;
    Serial.printf("BLE Disconnected (reason=%d)\n", reason);
    NimBLEDevice::startAdvertising();
  }
};

class LedCharCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    std::string v = pCharacteristic->getValue();
    if (v.empty()) return;
    uint8_t b = (uint8_t)v[0];
    digitalWrite(LED_PIN, b == 1 ? HIGH : LOW);
  }
};

// ================= OLED UI =================
void showPairing() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setFont(NULL);
  display.setTextSize(2);
  display.setCursor(0, 8);
  display.print("Pairing");
  display.display();
}

void showScreenTimeUI(bool isCounting) {

  // -------- Blink logic --------
  unsigned long now = millis();
  if (isCounting && (now - lastBlinkToggle >= BLINK_INTERVAL)) {
    blinkVisible = !blinkVisible;
    lastBlinkToggle = now;
  }

  unsigned long elapsed = getScreenOnTimeMs();

  unsigned long totalSeconds = elapsed / 1000;
  int hours = totalSeconds / 3600;
  int minutes = (totalSeconds % 3600) / 60;

  char timeBuffer[20];
  sprintf(timeBuffer, "%02dh %02dm", hours, minutes);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setFont(NULL);

  display.setTextSize(1);
  display.setCursor(24, 2);

  // -------- Title --------
  display.print("Screen Time");

  // blink "+" only while counting
  if (isCounting && blinkVisible) {
    display.print("+");
  }

  display.setTextSize(2);
  display.setCursor(20, 15);
  display.println(timeBuffer);

  display.display();
}


void offScreen() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setFont(NULL);

  display.setTextSize(2);
  display.setCursor(5, 10);
  display.println("Screen OFF");

  display.display();
}

// ================= Setup =================
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(BTN_PIN, INPUT_PULLUP);

  // I2C OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found (try 0x3D)");
    while (true) delay(100);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setFont(NULL);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("XIAO ESP32-C6");
  display.println("Booting...");
  display.display();

  // BLE init
  NimBLEDevice::init(DEVICE_NAME);
  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new MyServerCallbacks());

  NimBLEService* service = server->createService(SERVICE_UUID);

  NimBLECharacteristic* ledChr = service->createCharacteristic(
    LED_CHAR_UUID,
    NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  ledChr->setCallbacks(new LedCharCallbacks());

  valueChr = service->createCharacteristic(
    VALUE_CHAR_UUID,
    NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ
  );

  // initial payload
  notifyApp(0, false);

  service->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setName(DEVICE_NAME);
  adv->enableScanResponse(true);
  adv->start();

  Serial.println("BLE Advertising started...");
  showPairing();
}

// ================= Loop =================
unsigned long lastUiMs = 0;
unsigned long lastNotifyMs = 0;

void loop() {
  updateScreenState();
  showScreenTimeUI(screenOn);
  delay(500);
}

