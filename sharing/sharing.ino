#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h> // مكتبة متوافقة مع ESP8266

// شاشة LCD
LiquidCrystal_PCF8574 lcd(0x27);

// معرف Google Apps Script
const char *GScriptUrl = "https://script.google.com/macros/s/AKfycbw4p_voN86MrnyU0aCdeICoPHmzvWR4TAY7VGpEncwYzvuN4ca9Es32o440sAqwDdSr/exec";

// بيانات WiFi
const char* ssid = "TurkTelekom_ZTCCA";
const char* password = "01414F1443cDd";

// إعدادات الاتصال بـ Google Sheets
WiFiClientSecure client;

// إعدادات قارئ RFID
int blocks[] = {4, 5, 6, 8, 9};
#define total_blocks (sizeof(blocks) / sizeof(blocks[0]))

#define RST_PIN  0  // D3
#define SS_PIN   2  // D4
#define BUZZER   4  // D2

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

// متغيرات البيانات
String student_id;
String payload;

/****************************************************************************************************
 * setup Function - تهيئة النظام
****************************************************************************************************/
void setup() {
  Serial.begin(115200);
  delay(10);
  
  // تهيئة شاشة LCD
  Wire.begin();
  lcd.begin(16, 2);
  lcd.setBacklight(255); // تفعيل الإضاءة الخلفية
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to WiFi...");

  // الاتصال بـ WiFi
  connectToWiFi();

  // تهيئة قارئ RFID
  SPI.begin();
  mfrc522.PCD_Init();

  // تعطيل التحقق من الشهادة لتجنب مشاكل SSL
  client.setInsecure();
}

/****************************************************************************************************
 * loop Function - التشغيل الأساسي
****************************************************************************************************/
void loop() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan your Tag");

  mfrc522.PCD_Init();

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  Serial.println("\nReading from RFID...");

  // تجميع البيانات من بطاقة RFID
  String values = collectRFIDData();

  // إذا لم يتم الحصول على بيانات صالحة، لا ترسلها
  if (values.length() == 0) {
    Serial.println("No valid data read. Skipping...");
    return;
  }

  // إنشاء الـ payload للإرسال
  payload = "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": \"" + values + "\"}";

  Serial.println("Publishing data...");
  Serial.println(payload);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Publishing Data...");

  // إرسال البيانات
  if (sendDataToGoogleSheets(payload)) {
    Serial.println("[OK] Data published.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Student ID: " + student_id);
    lcd.setCursor(0, 1);
    lcd.print("Thanks!");
  } else {
    Serial.println("Error while sending data.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Failed to Send!");
    lcd.setCursor(0, 1);
    lcd.print("Try Again!");
  }

  delay(5000);
}

/****************************************************************************************************
 * connectToWiFi() - وظيفة الاتصال بـ WiFi مع إعادة المحاولة
****************************************************************************************************/
void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.println(WiFi.localIP());
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    delay(2000);
  } else {
    Serial.println("\nFailed to connect to WiFi!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed!");
  }
}

/****************************************************************************************************
 * collectRFIDData() - وظيفة قراءة البيانات من بطاقة RFID
****************************************************************************************************/
String collectRFIDData() {
  String values = "";
  bool firstValue = true;
  byte readBlockData[18];
  byte bufferLen = 18;

  for (byte i = 0; i < total_blocks; i++) {
    ReadDataFromBlock(blocks[i], readBlockData, bufferLen);
    String data = String((char*)readBlockData);
    data.trim();

    if (data.length() == 0) continue;

    if (firstValue) {
      student_id = data;
      values = data;
      firstValue = false;
    } else {
      values += String(",") + data;
    }
  }

  values += String(",") + "Gate1";
  return values;
}

/****************************************************************************************************
 * sendDataToGoogleSheets() - وظيفة إرسال البيانات إلى Google Sheets
****************************************************************************************************/
bool sendDataToGoogleSheets(String payload) {
  if (!client.connect("script.google.com", 443)) {
    Serial.println("Error! Not connected to host.");
    return false;
  }

  // إرسال الطلب HTTP
  String request = "POST " + String(GScriptUrl) + " HTTP/1.1\r\n" +
                   "Host: script.google.com\r\n" +
                   "Content-Type: application/json\r\n" +
                   "Content-Length: " + String(payload.length()) + "\r\n" +
                   "Connection: close\r\n\r\n" +
                   payload;

  client.print(request);

  // قراءة الرد من الخادم
  String response = "";
  while (client.available()) {
    response += client.readString();
  }

  Serial.println("Response from Google Sheets:");
  Serial.println(response);

  // تحليل الرد
  return response.indexOf("success") != -1;
}

/****************************************************************************************************
 * ReadDataFromBlock() - وظيفة قراءة البيانات من بطاقة RFID
****************************************************************************************************/
void ReadDataFromBlock(int blockNum, byte readBlockData[], byte bufferLen) { 
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

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

  readBlockData[16] = ' ';
  readBlockData[17] = ' ';
  Serial.println("Block read successfully");
}
