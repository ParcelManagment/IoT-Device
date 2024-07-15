#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// Define your Wi-Fi credentials
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// AWS IoT endpoint and certificates
const char* aws_endpoint = "your-aws-endpoint.iot.us-east-1.amazonaws.com"; // Your AWS IoT broker address
const int mqtt_port = 8883; // AWS IoT uses port 8883 for secure MQTT

const char* certificate_pem_crt = R"(
-----BEGIN CERTIFICATE-----
YOUR_CERTIFICATE_HERE
-----END CERTIFICATE-----
)";
const char* private_pem_key = R"(
-----BEGIN RSA PRIVATE KEY-----
YOUR_PRIVATE_KEY_HERE
-----END RSA PRIVATE KEY-----
)";
const char* aws_root_ca_pem = R"(
-----BEGIN CERTIFICATE-----
YOUR_ROOT_CA_HERE
-----END CERTIFICATE-----
)";

// MQTT unique identifier
const char *mqtt_identifier = "clientId-xXtkhFEyyP";

// Initialize the WiFi and MQTT clients
WiFiClientSecure wifiClient;
PubSubClient mqtt(wifiClient);

bool connectWiFi();
void reconnectMQTT();
void callback(char *topic, byte *payload, unsigned int length);

void setup()
{
  // Set up the serial for debugging
  Serial.begin(115200);

  // Initialize WiFi connection
  if (connectWiFi())
  {
    // Set up SSL/TLS certificates
    wifiClient.setCACert(aws_root_ca_pem);
    wifiClient.setCertificate(certificate_pem_crt);
    wifiClient.setPrivateKey(private_pem_key);

    // Initialize MQTT client
    mqtt.setServer(aws_endpoint, mqtt_port);
    mqtt.setCallback(callback);

    // Attempt to connect to the MQTT broker
    reconnectMQTT();
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
    String message = "Hello from ESP32 via Wi-Fi/GPS-Lat:6.695780655737321,Lon:80.07557582195379";
    Serial.print("Publishing message: ");
    Serial.println(message);
    mqtt.publish("your_topic", message.c_str());
  }
}

bool connectWiFi()
{
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int retry_count = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if (++retry_count >= 20) // Retry for 10 seconds
    {
      Serial.println("Failed to connect to WiFi");
      return false;
    }
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  return true;
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
