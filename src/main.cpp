#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_task_wdt.h>

// OLED display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64 // or 32 for smaller display
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)
#define START_BUTTON 32  // progess start button - should be change
#define MODE_SELECT_BUTTON 33

// LED Indicators
#define DisplayErrorLED 12 // Indicate the errors occurred on the display

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Object in Adafruit_SSD1306 class

volatile bool continueWelcomeScreen = true; // Flag to control showing welcome screen

void startButtonInterrupt()
{
  // Toggle the flag to stop showing welcome screen
  continueWelcomeScreen = false;
}

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

// Function for testing OLED display - success
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

  delay(200); // Wait before updating again
}

// success
void showBootScreen()
{
  display.clearDisplay();

  // Calculate the cursor position for center alignment
  const char *line1 = "Track-ME";
  const char *line2 = "Powered by EIT @ UOC";
  const char *line3 = "(20/21 Batch)";

  int16_t x1, y1;
  uint16_t w, h;

  display.setTextSize(2);
  display.getTextBounds(line1, 0, 0, &x1, &y1, &w, &h);
  int16_t cursorX1 = (SCREEN_WIDTH - w) / 2;
  int16_t cursorY1 = (SCREEN_HEIGHT / 2) - h - 5; // adjust as needed for vertical position

  display.setTextSize(1);
  display.getTextBounds(line2, 0, 0, &x1, &y1, &w, &h);
  int16_t cursorX2 = (SCREEN_WIDTH - w) / 2;
  int16_t cursorY2 = (SCREEN_HEIGHT / 2) + 5; // adjust as needed for vertical position

  display.setTextSize(1);
  display.getTextBounds(line3, 0, 0, &x1, &y1, &w, &h);
  int16_t cursorX3 = (SCREEN_WIDTH - w) / 2;
  int16_t cursorY3 = (SCREEN_HEIGHT / 2) + 15; // adjust as needed for vertical position

  // Set the cursor position and print the text
  display.setTextSize(2);
  display.setCursor(cursorX1, cursorY1);
  display.println(line1);

  display.setTextSize(1);
  display.setCursor(cursorX2, cursorY2);
  display.println(line2);

  display.setTextSize(1);
  display.setCursor(cursorX3, cursorY3);
  display.println(line3);

  display.display();
  delay(2000);
}

void showWelcomeScreen()
{
  const char *line1 = "Welcome to";
  const char *line2 = "Track-ME";
  const char *line3 = " Press START Button      to continue...";

  int16_t x1, y1;
  uint16_t w1, h1, w2, h2, w3, h3, w11, h11;

  display.setTextSize(2);
  display.getTextBounds(line1, 0, 0, &x1, &y1, &w1, &h1);
  display.setTextSize(2);
  display.getTextBounds(line2, 0, 0, &x1, &y1, &w2, &h2);
  display.setTextSize(1);
  display.getTextBounds(line3, 0, 0, &x1, &y1, &w3, &h3);
  display.getTextBounds(line1, 0, 0, &x1, &y1, &w11, &h11);

  int len3 = strlen(line3);
  for (int i = 0; i <= len3; i++)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor((SCREEN_WIDTH - w11) / 2, 0); // Upper center horizontal alignment
    display.println(line1);
    display.setTextSize(2);
    display.setCursor((SCREEN_WIDTH - w2) / 2, (SCREEN_HEIGHT / 2) - h2 / 2 - 2); // Center vertical position
    display.println(line2);
    display.setTextSize(1);
    String part3 = String(line3).substring(0, i);
    display.setCursor((SCREEN_WIDTH - w3) / 2, (SCREEN_HEIGHT / 2) + 16); // Lower center horizontal alignment
    display.println(part3);
    display.display();
    delay(20);
  }
}

void showRegisterParcelsScreen()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 16);
  display.println("Scan RFID to Register");
  display.display();
}

void showTrackParcelsScreen()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Tracking Mode");
  display.display();
}

void showModeSelectionScreen()
{
  display.clearDisplay();

  const char *line1 = "Please select mode";
  const char *mode1 = "1. Register Parcels";
  const char *mode2 = "2. Track Parcels";
  const char *confirmMessage = "Press START to confirm";

  int selectedMode = 0; // 0 for Register, 1 for Track

  while (true)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor((SCREEN_WIDTH - display.width()) / 2, 0);
    display.println(line1);

    // Display modes with indication of selection
    if (selectedMode == 0)
    {
      display.setCursor((SCREEN_WIDTH - display.width()) / 2, 20);
      display.println("> " + String(mode1)); // Indicate selection
      display.setCursor((SCREEN_WIDTH - display.width()) / 2, 36);
      display.println("  " + String(mode2));
    }
    else
    {
      display.setCursor((SCREEN_WIDTH - display.width()) / 2, 20);
      display.println("  " + String(mode1));
      display.setCursor((SCREEN_WIDTH - display.width()) / 2, 36);
      display.println("> " + String(mode2)); // Indicate selection
    }

    // Display confirmation message
    display.setCursor((SCREEN_WIDTH - display.width()) / 2, 56);
    display.println(confirmMessage);

    display.display();

    // Wait for button press
    if (digitalRead(MODE_SELECT_BUTTON) == LOW)
    {
      selectedMode = !selectedMode; // Toggle selection
      delay(300);                   // Debounce delay
    }

    if (digitalRead(START_BUTTON) == LOW)
    {
      delay(300); // Debounce delay
      break;      // Exit loop on confirmation
    }

    delay(100);
  }

  // Proceed based on selected mode
  if (selectedMode == 0)
  {
    showRegisterParcelsScreen();
  }
  else
  {
    showTrackParcelsScreen();
  }
}

void setup()
{
  // Initialize the serial communication at 115200 baud rate
  Serial.begin(115200);

  // Initialized the start button.
  pinMode(START_BUTTON, INPUT_PULLDOWN);
  pinMode(MODE_SELECT_BUTTON, INPUT_PULLDOWN);

  // Attach interrupt to START_BUTTON pin
  attachInterrupt(digitalPinToInterrupt(START_BUTTON), startButtonInterrupt, FALLING);

  // Wait for the serial port to connect (useful for some boards)
  while (!Serial) //*************Remove this while loop from final deployment. */
  {
    ; // wait for serial port to connect. Needed for native USB
  }
  Serial.println("Serial Monitor Test: Hello, World!");

  Wire.begin();
  esp_task_wdt_init(60, true); // 10 seconds timeout
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

  testDisplay(); // Run the display function to test
  showBootScreen();
  display.clearDisplay();
  while (continueWelcomeScreen)
  {
    showWelcomeScreen();
  }
  display.clearDisplay();
}

void loop()
{
  esp_task_wdt_reset(); // Reset the watchdog timer periodically

  showModeSelectionScreen();
}
