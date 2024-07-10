#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_task_wdt.h>

// OLED display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64 // or 32 for smaller display
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)

// LED Indicators
#define DisplayErrorLED 12 // Indicate the errors occurred on the display

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Object in Adafruit_SSD1306 class

// Loading animation frames
const char *loadingFrames[] = {
    "|",
    "/",
    "-",
    "\\"};
const int numFrames = 4;

volatile bool START = false;                // prevent from unexpected button inputs.
volatile bool SELECTMODE = false;           // prevent from unexpected button inputs.
volatile bool continueWelcomeScreen = true; // Flag to control showing welcome screen
volatile bool selectModeOption = true;      // Flag to select the operating mode.
volatile bool confirmMode = true;           // flag for the confirmatio of the selected mode by start button
volatile bool displayTrackParcelsScreen = false;
volatile bool displayRegisterParcelsScreen = false;
volatile bool RFIDisOK = true;   // flag for RFID initialization
volatile bool MODEMisOK = false; // flag for SIM808 initialization
volatile bool GPRSisOK = false;  // flag for GPRS initialization
volatile bool GPSisOK = false;   // flag for GPS initialization

// variables to keep track of the timing of recent interrupts (button bouncing)
unsigned long button_time = 0;
unsigned long last_button_time = 0;

struct Button
{
  const uint8_t PIN;
};

Button START_BUTTON = {32};
Button MODE_SELECT_BUTTON = {33};

void IRAM_ATTR startButtonInterrupt()
{
  button_time = millis();
  if (button_time - last_button_time > 250)
  {
    if (START && !SELECTMODE)
    {
      continueWelcomeScreen = false;
    }
    if (START && SELECTMODE)
    {
      confirmMode = false;
    }
    last_button_time = button_time;
  }
}

void IRAM_ATTR modeSelectButtonInterrupt()
{
  button_time = millis();
  if (button_time - last_button_time > 250)
  {
    if (START && SELECTMODE)
    {
      selectModeOption = !selectModeOption;
    }
    last_button_time = button_time;
  }
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
  String data = "Testing display... Done!";
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
  delay(3000);
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

void displayInitializingProcess(const char *message, int &frame)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 40);
  display.println("Please wait...");
  display.setCursor(0, 55);
  display.println(message);

  // Draw the loading icon
  display.setCursor(55, 8); // Position the loading icon
  display.setTextSize(3);
  display.print(loadingFrames[frame]);
  display.display();

  frame = (frame + 1) % numFrames; // Update the frame

  delay(50); // Delay to control animation speed
}

void showRegisterParcelsScreen()
{
  int frame = 0;
  while (!RFIDisOK)
  {
    displayInitializingProcess("Initializing RFID", frame);
  }
  while (!MODEMisOK)
  {
    displayInitializingProcess("Initializing MODEM", frame);
  }
  while (!GPRSisOK)
  {
    displayInitializingProcess("Initializing GPRS", frame);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 50);
  display.println("Device is ready..");
  display.display();
}

void showTrackParcelsScreen()
{
  int frame = 0;
  while (!MODEMisOK)
  {
    displayInitializingProcess("Initializing MODEM", frame);
  }
  while (!GPRSisOK)
  {
    displayInitializingProcess("Initializing GPRS", frame);
  }
  while (!GPSisOK)
  {
    displayInitializingProcess("Initializing GPS", frame);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 50);
  display.println("Device is ready..");
  display.display();
}

void showModeSelectionScreen()
{
  display.clearDisplay();
  const char *line1 = "Please select mode";
  const char *mode1 = "1. Register Parcels";
  const char *mode2 = "2. Track Parcels";
  const char *confirmMessage = "Press START to confirm";
  while (confirmMode)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor((SCREEN_WIDTH - display.width()) / 2, 0);
    display.println(line1);
    // Display modes with indication of selection
    if (selectModeOption)
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
    delay(100); // Add a small delay for debouncing
  }
  // Set the screen flag based on the selected mode
  if (selectModeOption)
  {
    displayRegisterParcelsScreen = true;
  }
  else
  {
    displayTrackParcelsScreen = true;
  }
  // Reset flags for next operations
  confirmMode = true;
  selectModeOption = true;
}

void setup()
{

  Serial.begin(115200); // Initialize the serial communication at 115200 baud rate
  while (!Serial)       // Wait for the serial port to connect (useful for some boards)
  {
    ; // wait for serial port to connect. Needed for native USB
  }
  Serial.println("Serial Monitor Test: Hello, World!");

  //-------------------------------------------------------------------------------------------
  // Initialize the button
  pinMode(START_BUTTON.PIN, INPUT_PULLUP);
  pinMode(MODE_SELECT_BUTTON.PIN, INPUT_PULLUP);

  //-------------------------------------------------------------------------------------------
  attachInterrupt(START_BUTTON.PIN, startButtonInterrupt, FALLING);            // Attach interrupt to START_BUTTON pin
  attachInterrupt(MODE_SELECT_BUTTON.PIN, modeSelectButtonInterrupt, FALLING); // Attach interrupt to MODE_SELECT_BUTTON pin

  //-------------------------------------------------------------------------------------------
  // Serial communication
  Wire.begin();
  esp_task_wdt_init(60, true); // 60 seconds timeout
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
  START = true; // Allow button inputs
  while (continueWelcomeScreen)
  {
    showWelcomeScreen();
  }
  SELECTMODE = true; // Allow mode selection
  showModeSelectionScreen();

  display.clearDisplay();

  if (displayRegisterParcelsScreen)
  {
    showRegisterParcelsScreen();
  }
  if (displayTrackParcelsScreen)
  {
    showTrackParcelsScreen();
  }
}

void loop()
{
  esp_task_wdt_reset(); // Reset the watchdog timer periodically
}


