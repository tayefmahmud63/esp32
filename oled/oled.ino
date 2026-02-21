#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define OLED_SDA D4
#define OLED_SCL D8

#define LIGHT_SENSOR_PIN D0

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(115200);

  // Start I2C with custom pins
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
// STEP 1 — LOW BRIGHTNESS
//showCountdown("Starting", "Calibration", "", 10);
//showDone("Start");

//showCountdown("Set Brightness", "LOW", "Step 1", 10);
//showDone("DONE");

// STEP 2 — FULL BRIGHTNESS
//showCountdown("Set Brightness", "FULL", "Step 2", 10);
//showDone("DONE");

// STEP 3 — TURN OFF SCREEN
//showCountdown("Turn Off", "The Screen", "Step 3", 10);
//showComplete("Complete");
offScreen();


display.display();

}






void showCountdown(const char* title,
                   const char* subtitle,
                   const char* stepText,
                   int seconds)
{
    for (int i = seconds; i >= 0; i--) {

        display.clearDisplay();
        display.setTextColor(WHITE);
        display.setFont(NULL);

        // Step label
        display.setTextSize(1);
        display.setCursor(30, 0);
        display.println(stepText);

        // Title
        display.setCursor(0, 12);
        display.println(title);

        // Subtitle
        display.setCursor(0, 24);
        display.println(subtitle);

        // Countdown number
        display.setTextSize(3);
        display.setCursor(95, 6);

        if (i > 0)
            display.println(i);
        else
            display.println("");

        display.display();
        delay(1000);
    }
}

void showDone(const char* message)
{
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(25, 15);
    display.println(message);
    display.display();

    delay(3000);
}


void showComplete(const char* message)
{
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(15, 15);
    display.println(message);
    display.display();

    delay(3000);
}



void homeScreen() {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setFont(NULL);
    display.setCursor(30, 2);
    display.println("Screen Time");
    display.setTextSize(2);
    display.setCursor(20, 15);
    display.println("12h 30m");
    display.display();
}



void offScreen() {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setFont(NULL);
    display.setTextSize(2);
    display.setCursor(5, 10);
    display.println("Screen OFF");
    display.display();
}

