#include <SPI.h>
#include <MFRC522.h>

// RFID reader pins
#define SS_PIN 5
#define RST_PIN 4

// Create an instance of the RFID reader
MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);

  // Initialize SPI bus
  SPI.begin();

  // Initialize RFID reader
  rfid.PCD_Init();
  Serial.println("RFID reader initialized.");
}

void loop() {
  // Look for new cards
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Show UID on serial monitor
  Serial.print("RFID Tag ID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();
  
  // Halt PICC
  rfid.PICC_HaltA();
  
  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}
