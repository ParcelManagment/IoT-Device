#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_task_wdt.h>

// OLED display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64    // or 32 for smaller display ****should be chnage.
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // Address found using I2C scanner **** can be 0x3D, so switch if it isn't working.

// LED Indicators
#define DisplayErrorLED 12 // Indicate the errors occured on the display

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Object in Adafruit_SSD1306 class

// Function to notify user of an error in OLED display initialization process.
void notifyUserAboutDisplayError()
{
  // Blink an LED to notify the display error & send a message to a connected app
  pinMode(DisplayErrorLED, OUTPUT);
  for (int i = 0; i < 10; i++)
  {
    digitalWrite(DisplayErrorLED, HIGH);
    Serial.println("Warning: A Error in display Initialization...Check the connections.");
    delay(500);
    digitalWrite(DisplayErrorLED, LOW);
    delay(500);
  }
}

// function for initialize the OLED display with retry mechanism.
bool initDisplay()
{
  for (int attempts = 0; attempts < 3; attempts++)
  {
    if (display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
      Serial.println("Display initialization is successful");
      return true; // initialization is successful.
    }

    notifyUserAboutDisplayError(); // notify about error
    delay(1000);
  }

  Serial.println("Display initialization is failed!");
  return false; // Initialization failed after 3 attempts
}

void setup()
{
  // Initialize the serial communication at 115200 baud rate
  Serial.begin(115200);
  // Wait for the serial port to connect (useful for some boards)
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB
  }
  Serial.println("Serial Monitor Test: Hello, World!");
}

void loop()
{
  // Print "Hello, World!" every second
  Serial.println("Hello, World!");
  delay(1000); // wait for 1 second
}
