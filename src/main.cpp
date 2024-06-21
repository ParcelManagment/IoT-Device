#include <Arduino.h>
#include <TinyGsmClient.h>

// Define constants for serial communication and LED indication
#define MODEM_TX 17 // Connect to SIM808 RX
#define MODEM_RX 16 // Connect to SIM808 TX
#define MODEM_RST 5 // Optional, connect to SIM808 RST
#define LED_PIN 2   // LED pin for status indication
#define SERIAL_BAUD 115200
#define MODEM_BAUD 9600
#define MAX_RETRIES 5

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

// Function to configure GPRS connection
bool configureGPRS()
{
  Serial.println("Configuring GPRS...");

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

// Function to indicate modem status with LED
void indicateStatus(int status)
{
  switch (status)
  {
  case 0: // Trying to connect (fast blink)
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    break;
  case 1: // Unable to connect (slow blink)
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    delay(1000);
    break;
  case 2: // Successfully connected (solid on)
    digitalWrite(LED_PIN, HIGH);
    break;
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
    indicateStatus(0); // Indicate trying to connect
    if (modemTest())
    {
      Serial.println("Modem initialized successfully.");
      indicateStatus(2); // Indicate successfully connected
      return true;
    }
    else
    {
      Serial.println("Failed to communicate with the modem. Re-Trying...!");
      for (int i = 0; i < 10; i++)
      {
        Serial.print(".......");
        delay(500);
      }
      Serial.println("");
    }
  }

  Serial.println("Modem failed to initialize after maximum retries.");
  return false;
}

// Function to establish GPRS connection
bool establishGPRS()
{
  for (int attempt = 1; attempt <= MAX_RETRIES; attempt++)
  {
    Serial.printf("Attempt %d of %d to configure GPRS...\n", attempt, MAX_RETRIES);
    if (configureGPRS())
    {
      Serial.println("GPRS configured successfully.");
      return true;
    }
    else
    {
      Serial.println("Failed to configure GPRS. Re-Trying...!");
      for (int i = 0; i < 10; i++)
      {
        Serial.print(".......");
        delay(500);
      }
      Serial.println("");
    }
  }

  Serial.println("GPRS failed to configure after maximum retries.");
  return false;
}

void setup()
{
  // Initialize serial communication for debugging
  Serial.begin(SERIAL_BAUD);
  delay(10);

  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize modem
  if (!initializeModem())
  {
    Serial.println("Modem initialization failed. Halting execution.");
    while (true)
    {
      indicateStatus(1); // Indicate unable to connect
    }
  }

  // Establish GPRS connection
  if (!establishGPRS())
  {
    Serial.println("GPRS configuration failed. Halting execution.");
    while (true)
    {
      indicateStatus(1); // Indicate unable to connect
    }
  }

  Serial.println("Modem initialized and GPRS configured successfully.");
  indicateStatus(2); // Indicate successfully connected
}

void loop()
{
  // Main loop can be used for other tasks
}
