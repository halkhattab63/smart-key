#include <SPI.h>
#include <MFRC522.h>

//--------------------------------------------------
// GPIO Mapping for ESP8266
const uint8_t RST_PIN = D3;  // Reset pin
const uint8_t SS_PIN = D4;   // SDA/SS pin
//--------------------------------------------------
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
//--------------------------------------------------
int blockNum;
byte bufferLen = 18;
byte readBlockData[18];
MFRC522::StatusCode status;
//--------------------------------------------------

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("Scan a MIFARE 1K Tag to write data...");
  
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;  // المفتاح الافتراضي
  }
}

void loop() {
  // تهيئة المفتاح الافتراضي
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // انتظار بطاقة جديدة
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    Serial.println("Error: Unable to read card serial number.");
    return;
  }

  Serial.println("\n** Card Detected **");
  Serial.print("Card UID:");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  // كتابة البيانات على البطاقة
  writeDataToCard();
  delay(5000); // تأخير بين القراءات
}

void writeDataToCard() {
  Serial.println("Please enter the following details:");
  processInput("Enter Student ID, ending with #", 4);
  processInput("Enter First Name, ending with #", 5);
  processInput("Enter Last Name, ending with #", 6);
  processInput("Enter Phone Number, ending with #", 8);
  processInput("Enter Address, ending with #", 9);
}

void processInput(const char* prompt, int blockNum) {
  Serial.println(F("---------------------------------------"));
  Serial.println(prompt);

  byte buffer[18];
  byte len = 0;

  // انتظار إدخال البيانات من المستخدم
  while (len == 0) {
    while (!Serial.available()); // انتظار حتى يتم إدخال البيانات
    len = Serial.readBytesUntil('#', (char *)buffer, 16);
    if (len == 0) {
      Serial.println("No input detected. Please try again.");
    }
  }

  // ملء باقي البافر بفراغات إذا كان الإدخال أقل من 16 حرفًا
  for (byte i = len; i < 16; i++) buffer[i] = ' ';
  
  Serial.print("Writing to block ");
  Serial.print(blockNum);
  Serial.println("...");

  if (WriteDataToBlock(blockNum, buffer)) {
    Serial.print("Block "); Serial.print(blockNum); Serial.println(" updated successfully.");
  } else {
    Serial.print("Failed to write to block "); Serial.println(blockNum);
  }

  if (ReadDataFromBlock(blockNum, readBlockData)) {
    dumpSerial(blockNum, readBlockData);
  } else {
    Serial.print("Failed to read from block "); Serial.println(blockNum);
  }
}

bool WriteDataToBlock(int blockNum, byte blockData[]) {
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed for Write: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }
  status = mfrc522.MIFARE_Write(blockNum, blockData, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Writing to Block failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }
  return true;
}

bool ReadDataFromBlock(int blockNum, byte readBlockData[]) {
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed for Read: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }
  return true;
}

void dumpSerial(int blockNum, byte blockData[]) {
  Serial.print("\nData saved on block ");
  Serial.print(blockNum);
  Serial.print(": ");
  for (int j = 0; j < 16; j++) {
    Serial.write(blockData[j]);
  }
  Serial.println();

  memset(readBlockData, 0, sizeof(readBlockData));
}