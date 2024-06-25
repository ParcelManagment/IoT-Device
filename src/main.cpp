#include <Arduino.h>
#include <TinyGsmClient.h>

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

const long intervalFast = 100;
const long intervalSlow = 1000;
unsigned long previousMillis = 0;
int ledState = LOW;

// APN details for GPRS connection
const char apn[] = "your_apn";
const char user[] = "";
const char password[] = "";

// Initialize HardwareSerial port
HardwareSerial modemSerial(2); // Use UART2
TinyGsm modem(modemSerial);

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

// Function to configure GPRS connection
bool configureGPRS()
{
  Serial.println("Configuring GPRS...");

  // Indicate trying to connect (fast blink)
  indicateStatus(LED_GPRS, 0);

  // Set the APN
  modemSerial.print("AT+CSTT=\"");
  modemSerial.print(apn);
  modemSerial.print("\",\"");
  modemSerial.print(user);
  modemSerial.print("\",\"");
  modemSerial.print(password);
  modemSerial.println("\"");
  if (!modemSerial.find("OK"))
  {
    Serial.println("Failed to set APN.");
    return false;
  }

  // Bring up the wireless connection
  modemSerial.println("AT+CIICR");
  if (!modemSerial.find("OK"))
  {
    Serial.println("Failed to bring up wireless connection.");
    return false;
  }

  // Get the local IP address
  modemSerial.println("AT+CIFSR");
  if (modemSerial.find("ERROR"))
  {
    Serial.println("Failed to get IP address.");
    return false;
  }

  Serial.println("GPRS connected.");
  return true;
}

// Function to check signal strength
void checkSignalStrength()
{
  modemSerial.println("AT+CSQ");
  String response = modemSerial.readString();
  Serial.print("Signal strength: ");
  Serial.println(response);
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

// Function to establish GPRS connection
bool establishGPRS()
{
  for (int attempt = 1; attempt <= MAX_RETRIES; attempt++)
  {
    Serial.printf("Attempt %d of %d to configure GPRS...\n", attempt, MAX_RETRIES);
    indicateStatus(LED_GPRS, 0); // Indicate trying to connect

    // Check signal strength before attempting GPRS configuration
    checkSignalStrength();

    if (configureGPRS())
    {
      Serial.println("GPRS configured successfully.");
      indicateStatus(LED_GPRS, 2); // Indicate successfully connected
      return true;
    }
    else
    {
      Serial.println("Failed to configure GPRS. Re-Trying...!");
      delay(500);
    }
  }

  Serial.println("GPRS failed to configure after maximum retries.");
  indicateStatus(LED_GPRS, 1); // Indicate unable to connect
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

void setup()
{
  // Initialize serial communication for debugging
  Serial.begin(SERIAL_BAUD);
  delay(10);

  // Initialize LED pins
  pinMode(LED_MODEM, OUTPUT);
  digitalWrite(LED_MODEM, LOW);
  pinMode(LED_GPRS, OUTPUT);
  digitalWrite(LED_GPRS, LOW);
  pinMode(LED_GPS, OUTPUT);
  digitalWrite(LED_GPS, LOW);

  // Initialize modem
  if (!initializeModem())
  {
    Serial.println("Modem initialization failed. Halting execution.");
    while (true)
    {
      indicateStatus(LED_MODEM, 1); // Indicate unable to connect
    }
  }

  // Establish GPRS connection
  if (!establishGPRS())
  {
    Serial.println("GPRS configuration failed. Halting execution.");
    while (true)
    {
      indicateStatus(LED_GPRS, 1); // Indicate unable to connect
    }
  }

  // Configure GPS
  if (!configureGPS())
  {
    Serial.println("GPS configuration failed. Halting execution.");
    while (true)
    {
      indicateStatus(LED_GPS, 1); // Indicate unable to connect
    }
  }

  Serial.println("Modem, GPRS, and GPS configured successfully.");
  indicateStatus(LED_MODEM, 2); // Indicate successfully connected
  indicateStatus(LED_GPRS, 2);  // Indicate successfully connected
  indicateStatus(LED_GPS, 2);   // Indicate successfully connected
}

void loop()
{
  // Fetch GPS data
  fetchGPSData();
  delay(5000); // Fetch GPS data every 5 seconds
}
