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

// I2C Scanner Function
void scanI2C()
{
  Serial.println("Scanning I2C bus...");
  byte error, address;  //two variables of type byte (an 8-bit unsigned integer). 
  int nDevices = 0;
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
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}

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

// function For Test OLED Display
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
  scanI2C(); // Scan for I2C devices
  esp_task_wdt_init(10, true); // 10 seconds timeout
  esp_task_wdt_add(NULL);      // Add current thread ro WDt

  if (!initDisplay())
  {
    esp_task_wdt_reset(); // reset the watchdog timout
    for (;;)
      ; // Halt the program or implement further error handling
  }

  display.display();
  delay(2000); // Pause for 2 seconds
  display.clearDisplay();
}

void loop()
{
  esp_task_wdt_reset(); // Reset the watchdog timer periodically
  testDisplay();        // Run the display function to test
}
