#include <Arduino.h>
#include <TinyGsmClient.h>
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
#define CHARGING_PIN 13    // GPIO pin connected to CHRG pin of the charging module
// Battery settings
#define ADC_PIN 34
#define CONV_FACTOR 1.8
#define READS 20
//--------------------------------------------
// Define constants for serial communication and LED indication
#define MODEM_TX 17 // Connect to SIM808 RX
#define MODEM_RX 16 // Connect to SIM808 TX
#define MODEM_RST 5 // Optional, connect to SIM808 RST
#define LED_MODEM 2 // LED pin for modem status indication
#define LED_GPRS 4  // LED pin for GPRS status indication
#define LED_GPS 13  // LED pin for GPS status indication
#define SERIAL_BAUD 115200
#define MODEM_BAUD 9600
#define MAX_RETRIES 5

// Initialize HardwareSerial port
HardwareSerial modemSerial(2); // Use UART2
TinyGsm modem(modemSerial);
Pangodream_18650_CL BL(ADC_PIN, CONV_FACTOR, READS);                      // object in Pangodreaam_18650_CL class
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Object in Adafruit_SSD1306 class
//--------------------------------------------
const long intervalFast = 100;
const long intervalSlow = 1000;
unsigned long previousMillis = 0;
int ledState = LOW;
//--------------------------------------------
// Global variables for signal strength and network type
int signalStrength = 100;  // Example value
String networkType = "3G"; // Example value

// variables to keep track of the timing of recent interrupts (button bouncing)
unsigned long button_time = 0;
unsigned long last_button_time = 0;

//-------------------------------------------
// Multitasking handler
TaskHandle_t setupTaskHandle;
TaskHandle_t initRegisterTaskHandle = NULL;
TaskHandle_t initTrackTaskHandle = NULL;
//-------------------------------------------

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
volatile bool displayTrackParcelsScreen = true;
volatile bool displayRegisterParcelsScreen = false;
volatile bool RFIDisOK = false;  // flag for RFID initialization
volatile bool MODEMisOK = false; // flag for SIM808 initialization
volatile bool GPRSisOK = false;  // flag for GPRS initialization
volatile bool GPSisOK = false;   // flag for GPS initialization

// strutures for external button interrupts
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
  String data1 = "Testing Display...";
  String data2 = "Done!";
  // Update display
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 25);
  display.println(data1);
  display.setCursor(50, 40);
  display.println(data2);
  display.display();
  delay(2000); // Wait before updating again
}
// Function for display the current operating mode
void showOperateMode()
{
  String mode;
  if (displayTrackParcelsScreen)
  {
    mode = "TM";
  }
  if (displayRegisterParcelsScreen)
  {
    mode = "RM";
  }
  display.setTextSize(1);
  display.setCursor(50, 2);
  display.print(mode);
}

// Function to draw the battery icon and text on the display
void drawBatteryStatus()
{
  float volts = BL.getBatteryVolts();
  int level = BL.getBatteryChargeLevel();
  String status = String(level) + "%";

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
}

// Updated function to draw the GPRS signal strength and network type on the display
void drawSignalStatus()
{
  // Calculate the number of bars to display based on signal strength
  int numBars = (signalStrength + 19) / 20; // Divide signal strength by 20 to get bars

  // Draw the signal strength bars
  for (int i = 0; i < numBars; i++)
  {
    display.fillRect(2 + (i * 6), 8 - (i * 2), 4, i * 2, SSD1306_WHITE);
  }

  // Draw the network type
  display.setTextSize(1);
  display.setCursor(33, 2);
  display.print(networkType);
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

// Function to indicate status with LEDs using millis for non-blocking delays
void indicateStatus(int ledPin, int status)
{
  unsigned long currentMillis = millis();

  switch (status)
  {
  case 0: // Trying to connect (fast blink)
    if (currentMillis - previousMillis >= intervalFast)
    {
      previousMillis = currentMillis;
      ledState = (ledState == LOW) ? HIGH : LOW;
      digitalWrite(ledPin, ledState);
    }
    break;
  case 1: // Unable to connect (slow blink)
    if (currentMillis - previousMillis >= intervalSlow)
    {
      previousMillis = currentMillis;
      ledState = (ledState == LOW) ? HIGH : LOW;
      digitalWrite(ledPin, ledState);
    }
    break;
  case 2: // Successfully connected (solid on)
    digitalWrite(ledPin, HIGH);
    break;
  default: // Off
    digitalWrite(ledPin, LOW);
    break;
  }
}

// Function to test modem communication
bool modemTest()
{
  Serial.println("Testing modem...");

  // Send basic AT command to check communication
  modemSerial.println("AT");
  if (modemSerial.find("OK"))
  {
    Serial.println("Modem is responding to AT commands.");
    return true;
  }
  else
  {
    Serial.println("No response from modem.");
    return false;
  }
}

// Function to initialize the modem
bool initializeModem()
{
  // Set modem reset, if used
  pinMode(MODEM_RST, OUTPUT);
  digitalWrite(MODEM_RST, HIGH);

  // Start communication with the modem
  modemSerial.begin(MODEM_BAUD, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000); // Give time for the modem to initialize

  for (int attempt = 1; attempt <= MAX_RETRIES; attempt++)
  {
    Serial.printf("Attempt %d of %d to initialize modem...\n", attempt, MAX_RETRIES);
    indicateStatus(LED_MODEM, 0); // Indicate trying to connect
    if (modemTest())
    {
      Serial.println("Modem initialized successfully.");
      indicateStatus(LED_MODEM, 2); // Indicate successfully connected
      return true;
    }
    else
    {
      Serial.println("Failed to communicate with the modem. Re-Trying...!");
      delay(500);
    }
  }

  Serial.println("Modem failed to initialize after maximum retries.");
  indicateStatus(LED_MODEM, 1); // Indicate unable to connect
  return false;
}
// Function to configure GPS
bool configureGPS()
{
  Serial.println("Configuring GPS...");

  // Indicate trying to connect (fast blink)
  indicateStatus(LED_GPS, 0);

  // Power on the GPS
  modemSerial.println("AT+CGNSPWR=1");
  if (!modemSerial.find("OK"))
  {
    Serial.println("Failed to power on GPS.");
    return false;
  }

  // Configure the GPS NMEA output
  modemSerial.println("AT+CGNSSEQ=\"RMC\"");
  if (!modemSerial.find("OK"))
  {
    Serial.println("Failed to configure GPS NMEA output.");
    return false;
  }

  Serial.println("GPS configured.");
  return true;
}

// Function to fetch GPS data
void fetchGPSData()
{
  Serial.println("Fetching GPS data...");

  // Send command to get GPS info
  modemSerial.println("AT+CGNSINF");

  // Read the response
  String response = modemSerial.readString();
  Serial.println(response);

  // Parse the response to extract GPS info
  if (response.indexOf("+CGNSINF:") != -1)
  {
    // Split the response based on commas
    int start = response.indexOf(":") + 2; // Skip the "+CGNSINF: " part
    String gpsInfo = response.substring(start);
    char gpsData[gpsInfo.length() + 1];
    gpsInfo.toCharArray(gpsData, gpsInfo.length() + 1);

    // Use strtok to parse the gpsData
    char *token = strtok(gpsData, ",");
    int gpsFix = 0;
    float latitude = 0.0, longitude = 0.0, altitude = 0.0, speed = 0.0, course = 0.0;
    float hdop = 0.0, pdop = 0.0, vdop = 0.0;
    int satellitesInView = 0, satellitesUsed = 0;

    // Iterate through tokens to find the GPS fix status, latitude, longitude, altitude, etc.
    for (int i = 0; token != NULL; i++)
    {
      switch (i)
      {
      case 1: // GPS fix status
        gpsFix = atoi(token);
        break;
      case 3: // Latitude
        latitude = atof(token);
        break;
      case 4: // Longitude
        longitude = atof(token);
        break;
      case 5: // Altitude
        altitude = atof(token);
        break;
      case 6: // Speed over ground
        speed = atof(token);
        break;
      case 7: // Course over ground
        course = atof(token);
        break;
      case 9: // HDOP
        hdop = atof(token);
        break;
      case 10: // PDOP
        pdop = atof(token);
        break;
      case 11: // VDOP
        vdop = atof(token);
        break;
      case 14: // Satellites in view
        satellitesInView = atoi(token);
        break;
      case 15: // Satellites used
        satellitesUsed = atoi(token);
        break;
      }
      token = strtok(NULL, ",");
    }

    if (gpsFix == 1)
    {
      Serial.println("GPS fix acquired.");
      Serial.print("Latitude: ");
      Serial.println(latitude, 6);
      Serial.print("Longitude: ");
      Serial.println(longitude, 6);
      Serial.print("Altitude: ");
      Serial.println(altitude, 2);
      Serial.print("Speed: ");
      Serial.println(speed, 2);
      Serial.print("Course over ground (degrees): ");
      Serial.println(course, 2);
      Serial.print("Horizontal Dilution of Precision (HDOP): ");
      Serial.println(hdop, 2);
      Serial.print("Position Dilution of Precision (PDOP): ");
      Serial.println(pdop, 2);
      Serial.print("Vertical Dilution of Precision (VDOP): ");
      Serial.println(vdop, 2);
      Serial.print("Satellites in view: ");
      Serial.println(satellitesInView);
      Serial.print("Satellites used: ");
      Serial.println(satellitesUsed);

      // Indicate GPS fix acquired (solid on)
      indicateStatus(LED_GPS, 2);
    }
    else
    {
      Serial.println("No valid GPS fix.");
      // Indicate no valid GPS fix (slow blink)
      indicateStatus(LED_GPS, 1);
    }
  }
  else
  {
    Serial.println("No GPS data available.");
    // Indicate no GPS data available (slow blink)
    indicateStatus(LED_GPS, 1);
  }
}

void initRegisterParcelMode(void *pvParameters)
{
  // Initialize RFID
  delay(5000);     // Must be removed in production, for testing only
  RFIDisOK = true; // Must be removed in production, for testing only

  // Initialize modem
  if (!initializeModem())
  {
    Serial.println("Modem initialization failed. Halting execution.");
    while (true)
    {
      indicateStatus(LED_MODEM, 1); // Indicate unable to connect
    }
  }
  MODEMisOK = true;

  // Initialize GPRS
  delay(5000);     // Must be removed in production, for testing only
  GPRSisOK = true; // Must be removed in production, for testing only

  // Notify the display task that initialization is complete
  if (initRegisterTaskHandle != NULL)
  {
    xTaskNotifyGive(initRegisterTaskHandle);
  }
  vTaskDelete(NULL); // Delete the task once done
}

void showRegisterParcelsScreen(void *pvParameters)
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

  // Notify the setup function that the display task is complete
  if (setupTaskHandle != NULL)
  {
    xTaskNotifyGive(setupTaskHandle);
  }
  vTaskDelete(NULL); // Delete the task once done
}

void initTrackParcelMode(void *pvParameters)
{
  // Initialize modem
  if (!initializeModem())
  {
    Serial.println("Modem initialization failed. Halting execution.");
    while (true)
    {
      indicateStatus(LED_MODEM, 1); // Indicate unable to connect
    }
  }
  MODEMisOK = true;

  // Configure GPS
  if (!configureGPS())
  {
    Serial.println("GPS configuration failed. Halting execution.");
    while (true)
    {
      indicateStatus(LED_GPS, 1); // Indicate unable to connect
    }
  }
  GPSisOK = true;

  // Initialize GPRS
  delay(5000);     // Must be removed in production, for testing only
  GPRSisOK = true; // Must be removed in production, for testing only

  // Notify the display task that initialization is complete
  if (initTrackTaskHandle != NULL)
  {
    xTaskNotifyGive(initTrackTaskHandle);
  }
  vTaskDelete(NULL); // Delete the task once done
}

void showTrackParcelsScreen(void *pvParameters)
{
  int frame = 0;
  while (!MODEMisOK)
  {
    displayInitializingProcess("Initializing MODEM", frame);
  }

  while (!GPSisOK)
  {
    displayInitializingProcess("Initializing GPS", frame);
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

  // Notify the setup function that the display task is complete
  if (setupTaskHandle != NULL)
  {
    xTaskNotifyGive(setupTaskHandle);
  }
  vTaskDelete(NULL); // Delete the task once done
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
  // Initialize the button
  pinMode(START_BUTTON.PIN, INPUT_PULLUP);
  pinMode(MODE_SELECT_BUTTON.PIN, INPUT_PULLUP);
  // Initialize LED pins
  pinMode(LED_MODEM, OUTPUT);
  digitalWrite(LED_MODEM, LOW);
  pinMode(LED_GPRS, OUTPUT);
  digitalWrite(LED_GPRS, LOW);
  pinMode(LED_GPS, OUTPUT);
  digitalWrite(LED_GPS, LOW);
  // Initialize the serial communication at 115200 baud rate
  Serial.begin(115200); // Initialize the serial communication at 115200 baud rate
  while (!Serial)       // Wait for the serial port to connect (useful for some boards)
  {
    ; // wait for serial port to connect. Needed for native USB
  }
  Serial.println("Serial Monitor Test is successed: Hello, World!");

  //-------------------------------------------------------------------------------------------
  attachInterrupt(START_BUTTON.PIN, startButtonInterrupt, FALLING);            // Attach interrupt to START_BUTTON pin
  attachInterrupt(MODE_SELECT_BUTTON.PIN, modeSelectButtonInterrupt, FALLING); // Attach interrupt to MODE_SELECT_BUTTON pin

  //-------------------------------------------------------------------------------------------
  esp_task_wdt_init(300, true); // 60 seconds timeout
  esp_task_wdt_add(NULL);       // Add current thread to WDT

  // Serial communication
  Wire.begin();
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

  // Get current task handle for notification
  setupTaskHandle = xTaskGetCurrentTaskHandle();

  // Process based on selected mode
  if (displayRegisterParcelsScreen)
  {
    // Create display task for register parcels screen
    xTaskCreate(
        showRegisterParcelsScreen,
        "RegisterDisplayTask",
        2048,
        NULL,
        1,
        &initRegisterTaskHandle);

    // Initialize register parcels mode
    xTaskCreate(
        initRegisterParcelMode,
        "InitRegisterTask",
        2048,
        NULL,
        1,
        NULL);

    // Wait for register parcels display task to complete
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  }

  if (displayTrackParcelsScreen)
  {
    // Create display task for track parcels screen
    xTaskCreate(
        showTrackParcelsScreen,
        "TrackDisplayTask",
        2048,
        NULL,
        1,
        &initTrackTaskHandle);

    // Initialize track parcels mode
    xTaskCreate(
        initTrackParcelMode,
        "InitTrackTask",
        2048,
        NULL,
        1,
        NULL);

    // Wait for track parcels display task to complete
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  }

  display.display();
  delay(2000); // Pause for 2 seconds
  display.clearDisplay();

  xTaskCreatePinnedToCore(
      [](void *arg)
      {
        for (;;)
        {
          display.clearDisplay();
          drawBatteryStatus();
          drawSignalStatus();
          showOperateMode();
          display.display();
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
  // Fetch and print GPS data every 10 seconds
  static unsigned long lastFetchTime = 0;
  if (millis() - lastFetchTime >= 10000)
  {
    fetchGPSData();
    lastFetchTime = millis();
  }

  // Add any other operations needed for your specific use case

  esp_task_wdt_reset(); // Reset the watchdog timer periodically
  testDisplay();        // Run the display function to test
  delay(2000);          // Wait before running the next test
}
