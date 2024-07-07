#include <TinyGsmClient.h>
#include <PubSubClient.h>

// Define your credentials
const char apn[] = "ppwap";                     // Your APN
const char user[] = "";                         // APN username if any
const char pass[] = "";                         // APN password if any
const char mqtt_server[] = "broker.hivemq.com"; // Your MQTT broker address
const int mqtt_port = 1883;

// Global state variables
bool MODEMisOK = false;
bool GPRSisOK = false;
String ipAddress = "";
int signalStrength = 0;
const int maxRetry = 5; // Number of retry attempts

// MQTT unique identifier
const char *mqtt_identifier = "clientId-xXtkhFEyyP";

// Initialize the GSM and MQTT clients
HardwareSerial SerialAT(1);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

bool initializeModem();
bool connectGPRS();
int getSignalStrength();
void reconnectMQTT();
void callback(char *topic, byte *payload, unsigned int length);

void setup()
{
  // Set up the serial for debugging and communication with the modem
  Serial.begin(115200);
  SerialAT.begin(9600, SERIAL_8N1, 16, 17); // TX, RX

  if (initializeModem())
  {
    if (connectGPRS())
    {
      ipAddress = modem.getLocalIP();
      Serial.print("IP Address: ");
      Serial.println(ipAddress);

      signalStrength = getSignalStrength();
      Serial.print("Signal Strength: ");
      Serial.print(signalStrength);
      Serial.println("%");

      // Initialize MQTT client
      mqtt.setServer(mqtt_server, mqtt_port);
      mqtt.setCallback(callback);

      // Attempt to connect to the MQTT broker
      reconnectMQTT();
    }
  }
}

void loop()
{
  // Maintain MQTT connection
  if (!mqtt.connected())
  {
    reconnectMQTT();
  }
  mqtt.loop();

  // Example of publishing a message to the MQTT broker every minute
  static long lastMsg = 0;
  long now = millis();
  if (now - lastMsg > 60000)
  { // 1 minute
    lastMsg = now;
    String message = "Hello from ESP32 via GPRS/GPS-Lat:6.695780655737321,Lon:80.07557582195379";
    Serial.print("Publishing message: ");
    Serial.println(message);
    mqtt.publish("your_topic", message.c_str());
  }

  // Example of restarting modem and reconnecting to GPRS if needed
  if (!MODEMisOK || !GPRSisOK)
  {
    if (initializeModem())
    {
      if (connectGPRS())
      {
        ipAddress = modem.getLocalIP();
        Serial.print("IP Address: ");
        Serial.println(ipAddress);

        signalStrength = getSignalStrength();
        Serial.print("Signal Strength: ");
        Serial.print(signalStrength);
        Serial.println("%");
      }
    }
  }
}

bool initializeModem()
{
  Serial.println("Initializing modem...");
  for (int attempt = 0; attempt < maxRetry; attempt++)
  {
    if (modem.restart())
    {
      Serial.println("Modem restarted successfully.");
      if (modem.init())
      {
        Serial.println("Modem initialized successfully.");
        MODEMisOK = true;
        Serial.print("Modem Info: ");
        Serial.println(modem.getModemInfo());
        return true;
      }
      else
      {
        Serial.println("Modem initialization failed.");
      }
    }
    else
    {
      Serial.println("Modem restart failed.");
    }
    Serial.print("Retry attempt ");
    Serial.print(attempt + 1);
    Serial.println("...");
    delay(2000); // Wait for 2 seconds before retrying
  }
  MODEMisOK = false;
  return false;
}

bool connectGPRS()
{
  Serial.println("Connecting to GPRS...");
  for (int attempt = 0; attempt < maxRetry; attempt++)
  {
    if (modem.gprsConnect(apn, user, pass))
    {
      Serial.println("GPRS connected successfully.");
      GPRSisOK = true;
      return true;
    }
    else
    {
      Serial.print("Attempt ");
      Serial.print(attempt + 1);
      Serial.println(" failed. Retrying...");
      delay(2000); // Wait for 2 seconds before retrying
    }
  }
  Serial.println("Failed to connect to GPRS.");
  GPRSisOK = false;
  return false;
}

int getSignalStrength()
{
  int signalQuality = modem.getSignalQuality();
  // Assuming signal quality ranges from 0 to 31 (3GPP TS 27.007 standard)
  return map(signalQuality, 0, 31, 0, 100);
}

void reconnectMQTT()
{
  // Loop until we're reconnected
  while (!mqtt.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(mqtt_identifier))
    {
      Serial.println("Connected to MQTT broker");
      // Subscribe to topics here if needed
      mqtt.subscribe("your_topic"); // Example topic
    }
    else
    {
      Serial.print("Failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  Serial.println();

  // Handle the message if needed
  // Example:
  // if (String(topic) == "your/topic") {
  //   Serial.print("Message: ");
  //   Serial.println(messageTemp);
  // }
}
