#define LIGHT_THRESHOLD 15
#define OFF_DELAY 2000   // 3 seconds

bool screenOn = false;

unsigned long onStartTime = 0;
unsigned long totalOnTime = 0;

unsigned long lowStartTime = 0;
bool lowDetected = false;


#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define OLED_SDA D4
#define OLED_SCL D8
#define LIGHT_SENSOR_PIN D0


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


unsigned long getScreenOnTime()
{
    unsigned long elapsed = totalOnTime;

    if (screenOn) {
        elapsed += millis() - onStartTime;
    }

    return elapsed;
}


void updateScreenState()
{
    int sensorValue = analogRead(LIGHT_SENSOR_PIN);
    unsigned long now = millis();

    // ---------- SCREEN ON CONDITION ----------
    if (sensorValue > LIGHT_THRESHOLD) {

        // cancel low detection
        lowDetected = false;

        if (!screenOn) {
            screenOn = true;
            onStartTime = now;
            Serial.println("Screen turned ON");
        }
    }

    // ---------- POSSIBLE SCREEN OFF ----------
    else {

        // first time going low
        if (!lowDetected) {
            lowDetected = true;
            lowStartTime = now;
        }

        // stayed low long enough
        if (screenOn && (now - lowStartTime >= OFF_DELAY)) {
            screenOn = false;
            totalOnTime += now - onStartTime;

            Serial.println("Screen turned OFF (3s confirmed)");
        }
    }
}



void homeScreen() {

    unsigned long elapsed = getScreenOnTime();

    unsigned long totalSeconds = elapsed / 1000;
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;

    char timeBuffer[20];
    sprintf(timeBuffer, "%02dh %02dm", hours, minutes);

    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setFont(NULL);

    display.setTextSize(1);
    display.setCursor(30, 2);
    display.println("Screen Time");

    display.setTextSize(2);
    display.setCursor(20, 15);
    display.println(timeBuffer);

    display.display();
}
void offScreen() {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(5, 10);
    display.println("Screen OFF");
    display.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(OLED_SDA, OLED_SCL);

  // Initialize display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found");
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0,0);
  display.println("XIAO ESP32-C6");
  display.println("Light Sensor Init");
  display.display();

  delay(2000);
}

void loop() {

    updateScreenState();

    if (screenOn) {
        homeScreen();
    } else {
        offScreen();
    }

    delay(500); // refresh twice per second
}


