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
  Serial.println("RFID Reader Initialized. Scan a MIFARE 1K Tag to format...");
  
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;  // المفتاح الافتراضي
  }
}

void loop() {
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

  // تأكيد من المستخدم قبل التنسيق
  Serial.println("Are you sure you want to format this card? (Y/N)");
  while (!Serial.available()); // انتظار إدخال المستخدم
  char userInput = Serial.read();
  if (userInput != 'Y' && userInput != 'y') {
    Serial.println("Formatting canceled.");
    return;
  }

  Serial.println("Formatting card...");
  formatCard();
  Serial.println("Card formatted successfully!");
  delay(5000);
}

void formatCard() {
  byte emptyBlock[16] = {0};  // كتلة فارغة

  // تنسيق الكتل من 4 إلى 63 (تجنب كتل النظام)
  for (int block = 4; block <= 63; block++) {
    // تجنب كتل النظام (كل كتلة 4 في القطاع)
    if ((block + 1) % 4 == 0) {
      Serial.print("Skipping system block: "); Serial.println(block);
      continue;
    }

    // كتابة الكتلة الفارغة
    if (WriteDataToBlock(block, emptyBlock)) {
      Serial.print("Block "); Serial.print(block); Serial.println(" cleared.");
    } else {
      Serial.print("Failed to clear block "); Serial.println(block);
    }
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

void ReadDataFromBlock(int blockNum, byte readBlockData[]) {
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed for Read: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  Serial.print("Read Data from Block "); Serial.print(blockNum); Serial.print(": ");
  Serial.println((char*)readBlockData);
}