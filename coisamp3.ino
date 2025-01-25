#include <b64.h>
#include <HttpClient.h>
#include <WiFi.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include <AudioOutputI2SNoDAC.h>
#include <AudioFileSourceHTTPStream.h>
#include <SPI.h>      //https://www.arduino.cc/en/reference/SPI
#include <MFRC522.h>  //https://github.com/miguelbalboa/RFID

// WiFi credentials
#define WIFI_SSID "gabriel-notebook"
#define WIFI_PASSWORD "12345678"

// URL to the MP3 file
#define MP3_URL "http://10.42.0.1:5000/"

// RFID Constants
#define SS_PIN 32
#define RST_PIN 2
#define RX_PIN 14
#define TX_PIN 27

// Objects for audio playback
AudioGeneratorMP3 *mp3;
AudioFileSourceHTTPStream *file;
AudioOutputI2S *out;

// Card struct
struct card {
  byte ID[4];
  int audioFile;
};

// RFID variables
MFRC522::MIFARE_Key key;
MFRC522 rfid = MFRC522(SS_PIN, RST_PIN);
byte nuidPICC[4] = { 0, 0, 0, 0 }; // array to store current/last read object

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Trying to connect...");
    delay(1000);
  }
  Serial.println("\nConnected to WiFi");

  SPI.begin(); // not sure what this is for. probably RFID.
  rfid.PCD_Init();
  Serial.print(F("Reader :"));
  rfid.PCD_DumpVersionToSerial();
  out = new AudioOutputI2SNoDAC();                // Output via DAC
  mp3 = new AudioGeneratorMP3();
  out->SetPinout(0, 0, 25);                   // I2S pin configuration, only "out" used
  out->SetGain(1.0);                            // Set volume (0.0 to 1.0)
}

void loop() {
  if (mp3->isRunning()) {
    if (!mp3->loop()) { 
        mp3->stop();
        Serial.println("MP3 done");
    } else {
        Serial.println("MP3 playing");
        return;
    }
  }  // Read RFID card
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  // Look for new 1 cards
  if (!rfid.PICC_IsNewCardPresent())
    return;
  // Verify if the NUID has been read
  if (!rfid.PICC_ReadCardSerial())
    return;
  // Store NUID into nuidPICC array
  readPersonalData();
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }
  // Print out RFID info
  Serial.print(F("\n--------------\nRFID In dec: "));
  printDec(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();
  Serial.print(F("RFID in hex: "));
  printHex(rfid.uid.uidByte, rfid.uid.size);
  Serial.println(F("\n--------------"));
  // Continue streaming the MP3 file
  playAudio();
  rfid.PICC_HaltA();
  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

void convertBytesToHexString(byte* byteArray, char* outString, int length) {
  int index = 0;
  for (int i = 0; i < length; i++) {
    // Format each byte as "0xXX"
    sprintf(outString + index, "0x%02X", byteArray[i]);
    index += 4;  // Move the index by 4 (because "0x" and two hex characters are 4 chars)
  }
  outString[index] = '\0';  // Null-terminate the string
}

void playAudio() {
  // Initialize audio Objects
  char url[256];
  char hexString[20];
  convertBytesToHexString(nuidPICC, hexString, 4);
  snprintf(url, sizeof(url), "%s%s", MP3_URL, hexString);
  Serial.println(hexString);
  Serial.println(url);
  file = new AudioFileSourceHTTPStream(url); // Stream MP3 file from URL
  mp3->begin(file, out);

  Serial.println("MP3 streaming started...");
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
