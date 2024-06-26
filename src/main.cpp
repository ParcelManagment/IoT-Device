#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_task_wdt.h>
#include "Pangodream_18650_CL.h"

// OLED display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64 // or 32 for smaller display
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)

// LED Indicators
#define DisplayErrorLED 12 // Indicate the errors occurred on the display

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Object in Adafruit_SSD1306 class

// Battery settings
#define ADC_PIN 34
#define CONV_FACTOR 1.8
#define READS 20
Pangodream_18650_CL BL(ADC_PIN, CONV_FACTOR, READS);

// I2C Scanner Function
int scanI2C()
{
  Serial.println("Scanning I2C bus...");
  byte error, address; // two variables of type byte (an 8-bit unsigned integer).
  int nDevices = 0;
  int foundAddress = -1;
  for (address = 1; address < 127; address++)
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      nDevices++;
      if (address == 0x3C || address == 0x3D)
      {
        foundAddress = address;
      }
    }
    else if (error == 4)
    {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
  {
    Serial.println("No I2C devices found\n");
    return -1;
  }
  else
  {
    Serial.println("done\n");
    return foundAddress;
  }
}

// Function to notify user of an error in OLED display initialization process.
void notifyUserAboutDisplayError(const char *message)
{
  // Blink an LED to notify the display error & send a message to a connected app
  pinMode(DisplayErrorLED, OUTPUT);
  for (int i = 0; i < 10; i++)
  {
    digitalWrite(DisplayErrorLED, HIGH);
    Serial.println(message);
    delay(500);
    digitalWrite(DisplayErrorLED, LOW);
    delay(500);
  }
}

// Function for initializing the OLED display with retry mechanism.
bool initDisplay(int screenAddress)
{
  for (int attempts = 0; attempts < 3; attempts++)
  {
    if (display.begin(SSD1306_SWITCHCAPVCC, screenAddress))
    {
      Serial.println("Display initialization is successful");
      return true; // initialization is successful.
    }

    notifyUserAboutDisplayError("Warning: An error in display Initialization...Check the connections."); // notify about error
    delay(1000);
  }

  Serial.println("Display initialization has failed!");
  return false; // Initialization failed after 3 attempts
}

// Function for testing OLED display
void testDisplay()
{
  String data = "RFID: 12345678\nLocation: XYZ";
  // Update display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(data);
  display.display();

  delay(2000); // Wait before updating again
}

// Function to draw the battery icon and text on the display
void drawBatteryStatus()
{
  float volts = BL.getBatteryVolts();
  int level = BL.getBatteryChargeLevel();
  String status = String(level) + "%";

  display.clearDisplay();

  // Draw the battery icon outline
  display.drawRect(SCREEN_WIDTH - 26, 0, 24, 12, SSD1306_WHITE); // Battery rectangle
  display.fillRect(SCREEN_WIDTH - 4, 2, 2, 8, SSD1306_WHITE);    // Battery tip

  // Fill the battery level
  int batteryLevelWidth = map(level, 0, 100, 0, 20); // Map the battery level to a width
  display.fillRect(SCREEN_WIDTH - 24, 2, batteryLevelWidth, 8, SSD1306_WHITE);

  // Draw the battery percentage
  display.setTextSize(1);
  display.setCursor(SCREEN_WIDTH - 60, 2);
  display.print(status);

  display.setTextSize(1);
  display.setCursor(0, 54);
  display.print("Volts: ");
  display.print(volts, 2);

  display.display();
}

void setup()
{
  // Initialize the serial communication at 115200 baud rate
  Serial.begin(115200);
  // Wait for the serial port to connect (useful for some boards)
  while (!Serial) //*************Remove this while loop from final deployment. */
  {
    ; // wait for serial port to connect. Needed for native USB
  }
  Serial.println("Serial Monitor Test: Hello, World!");

  Wire.begin();
  esp_task_wdt_init(10, true); // 10 seconds timeout
  esp_task_wdt_add(NULL);      // Add current thread to WDT

  int screenAddress = scanI2C();
  if (screenAddress == -1 || !initDisplay(screenAddress))
  {
    Serial.println("Initialization failed, entering error loop...");
    esp_task_wdt_reset(); // reset the watchdog timeout
    for (;;)
    {
      notifyUserAboutDisplayError("Warning: An error in display Initialization...Check the connections."); // Notify the user continuously
      esp_task_wdt_reset();                                                                                // Reset watchdog to prevent system reset
    }
  }

  display.display();
  delay(2000); // Pause for 2 seconds
  display.clearDisplay();

  xTaskCreatePinnedToCore(
      [](void *arg)
      {
        for (;;)
        {
          drawBatteryStatus();
          vTaskDelay(pdMS_TO_TICKS(1000));
        }
      },
      "BatteryDisplayTask",
      2048,
      NULL,
      1,
      NULL,
      1);
}

void loop()
{
  esp_task_wdt_reset(); // Reset the watchdog timer periodically
  testDisplay();        // Run the display function to test
  delay(2000);          // Wait before running the next test
}
