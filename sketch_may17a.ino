// Libraries
#include <SPI.h>      //https://www.arduino.cc/en/reference/SPI
#include <MFRC522.h>  //https://github.com/miguelbalboa/rfid
#include "DFRobotDFPlayerMini.h"
// Constants
#define SS_PIN 32
#define RST_PIN 2

#define RX_PIN 14
#define TX_PIN 27

struct card {
  byte ID[4];
  int audioFile;
};

// Variables
card cards[] = {
  { { 121, 15, 20, 142 }, 1 },
  { { 227, 178, 199, 02 }, 2 },
  { { 60, 226, 112, 24 }, 3 }
};

MFRC522::MIFARE_Key key;
DFRobotDFPlayerMini player;
MFRC522 rfid = MFRC522(SS_PIN, RST_PIN);
byte nuidPICC[4] = { 0, 0, 0, 0 };

// Code
void setup() {
  // Init Serial USB
  Serial.begin(115200);
  Serial2.begin(9600);
  Serial.println("Initialize System HUH");
  // Start communication with DFPlayer Mini
  Serial.println("Initializing DFPlayer");
  while (!player.begin(Serial2)) {
    Serial.println("Trying to connect...");
  }
  player.volume(25);
  // init rfid D8,D5,D6,D7
  SPI.begin();
  rfid.PCD_Init();
  Serial.print(F("Reader :"));
  rfid.PCD_DumpVersionToSerial();
}

void readPersonalData() {
  MFRC522::StatusCode status;

  //rfid.PICC_DumpToSerial(&(rfid.uid));
  rfid.PICC_DumpDetailsToSerial(&(rfid.uid));
  // first name
  Serial.print(F("Name: "));
  byte buffer1[18];
  byte block = 4;
  byte len = 18;

  rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Auth failed: "));
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }

  status = rfid.MIFARE_Read(block, buffer1, &len);
   if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Read failed: "));
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }

  // print first name
  for (uint8_t i = 0; i < 16; i++) {
    if (buffer1[i] != 32) {
      Serial.write(buffer1[i]);
    }
  }
  Serial.print(" ");
  // last name
  byte buffer2[18];
  block = 1;

  status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &key, &(rfid.uid)); //line 834
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }

  status = rfid.MIFARE_Read(block, buffer2, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }

  //PRINT LAST NAME
  for (uint8_t i = 0; i < 16; i++) {
    Serial.write(buffer2[i]);
  }
}

void loop() {
  ////Read RFID card
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  // Look for new 1 cards
  if (!rfid.PICC_IsNewCardPresent())
    return;
  // Verify if the NUID has been readed
  if (!rfid.PICC_ReadCardSerial())
    return;
  // Store NUID into nuidPICC array
  readPersonalData();
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }
  Serial.print(F("\n--------------\nRFID In dec: "));
  printDec(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();
  Serial.print(F("RFID in hex: "));
  printHex(rfid.uid.uidByte, rfid.uid.size);
  Serial.println(F("\n--------------"));
  // Halt PICC

  for (card currentCard: cards) {
    if (equals(currentCard.ID, nuidPICC)) {
      Serial.println("Found and played");
      player.play(currentCard.audioFile);
      break;
    }
  }
  rfid.PICC_HaltA();
  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

bool equals(byte a[], byte b[]) {
  for (int i = 0; i < sizeof(a) / sizeof(byte); i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true; 
}

void writeCardData(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    nuidPICC[i] = buffer[i];
  }
}

void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}
