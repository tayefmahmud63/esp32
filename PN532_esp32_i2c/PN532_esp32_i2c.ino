#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>

#define I2C_SDA 21
#define I2C_SCL 22

PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc(pn532_i2c);

String lastUid = "";
unsigned long lastSeenMs = 0;

// If the same UID is seen again within this window, ignore it (tag still present)
const unsigned long SAME_TAG_HOLDOFF_MS = 1500;

// How often to poll for a tag
const unsigned long POLL_MS = 200;

void setup() {
  Serial.begin(115200);
  delay(200);

  // ESP32: start I2C explicitly
  Wire.begin(I2C_SDA, I2C_SCL);

  Serial.println("System initialized (ESP32 + PN532 I2C)");
  nfc.begin();

  // Optional: small delay to let PN532 settle
  delay(200);
}

void loop() {
  readNFC();
  delay(POLL_MS);
}

void readNFC() {
  if (!nfc.tagPresent()) return;

  NfcTag tag = nfc.read();
  String uid = tag.getUidString();

  

  Serial.print("Tag detected. UID: ");
  Serial.println(uid);

  // If you want extra info, uncomment:
  // tag.print();
}
