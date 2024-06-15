#include <Arduino.h>

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
