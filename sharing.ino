/* ------------------------------------------------------------------------
 * Created by: Tauseef Ahmad
 * Created on: 10 March, 2023
 *  
 * Tutorial: https://youtu.be/aNjkNmHRx4o
 * ------------------------------------------------------------------------
 * Download Resources
 * ------------------------------------------------------------------------
 * Preferences--> Additional boards Manager URLs : 
 * For ESP8266 and NodeMCU - Board Version 2.6.3
 * http://arduino.esp8266.com/stable/package_esp8266com_index.json
 * ------------------------------------------------------------------------
 * HTTPS Redirect Library:
 * https://github.com/jbuszkie/HTTPSRedirect
 * Example Arduino/ESP8266 code to upload data to Google Sheets
 * Follow setup instructions found here:
 * https://github.com/StorageB/Google-Sheets-Logging
 * ------------------------------------------------------------------------*/
 
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
   Serial.println("Initializing RFID System...");
   SPI.begin();
   mfrc522.PCD_Init();
   
   if (!mfrc522.PCD_PerformSelfTest()) {
     Serial.println("ERROR: RFID Module not detected! Check wiring and restart.");
     return;
   }
   Serial.println("RFID Module detected successfully.");
   Serial.println("Scan a MIFARE 1K Tag to write new data...");
   
   for (byte i = 0; i < 6; i++) {
     key.keyByte[i] = 0xFF;
   }
 }
 
 void loop() {
   if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
     return;
   }
 
   Serial.println("\n** Card Detected **");
   Serial.print("Card UID:");
   for (byte i = 0; i < mfrc522.uid.size; i++) {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
   }
   Serial.println();
 
   Serial.println("Writing new data to the card...");
   writeNewData();
   Serial.println("New data written successfully!");
   delay(5000);
 }
 
 void writeNewData() {
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
 
   while (len == 0) {
     while (!Serial.available());
     len = Serial.readBytesUntil('#', (char *)buffer, 16);
     if (len == 0) {
       Serial.println("No input detected. Please try again.");
     }
   }
 
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
 