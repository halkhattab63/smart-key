#include <SPI.h>
#include <MFRC522.h>

//--------------------------------------------------
// GPIO bağlantıları (ESP8266)
const uint8_t RST_PIN = D3;  // Reset pini
const uint8_t SS_PIN = D4;   // Slave Select pini (SPI bağlantısı)

// RFID okuyucu nesnesini oluştur
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// Veri yazma/okuma için değişkenler
int blockNum = 4;           // Yazılacak blok numarası
byte bufferLen = 18;        // Okunacak verinin uzunluğu (16 byte + 2 ekstra boşluk)
byte readBlockData[18];     // Okunan veriyi saklamak için dizi
MFRC522::StatusCode status; // RFID işlemlerinin durumu

//--------------------------------------------------

void setup() {
   Serial.begin(9600);  // Seri monitörü başlat
   SPI.begin();         // SPI haberleşmesini başlat
   mfrc522.PCD_Init();  // RFID modülünü başlat
   Serial.println("Kartınızı okutun, verileri yazmak için hazır...");
}

//--------------------------------------------------

void loop() {
   // Kart kimlik doğrulama anahtarını hazırla (0xFF -> varsayılan fabrika ayarı)
   for (byte i = 0; i < 6; i++) {
       key.keyByte[i] = 0xFF;
   }

   // Yeni kart var mı? Eğer yoksa döngüden çık
   if (!mfrc522.PICC_IsNewCardPresent()) { return; } 

   // Kartın UID'sini oku
   if (!mfrc522.PICC_ReadCardSerial()) { return; }   

   Serial.println("\n**Kart Algılandı**");
   Serial.print("Kart UID: ");
   for (byte i = 0; i < mfrc522.uid.size; i++) {
       Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
       Serial.print(mfrc522.uid.uidByte[i], HEX);
   }
   Serial.print("\n");

   // Kart türünü ekrana yazdır
   Serial.print("Kart Tipi: ");
   MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
   Serial.println(mfrc522.PICC_GetTypeName(piccType));

   // Kullanıcıdan veri al ve kartın bloklarına yaz
   veriAlVeYaz();
}

//--------------------------------------------------

void veriAlVeYaz() {
   byte buffer[18];
   byte len;
   Serial.setTimeout(20000L);

   Serial.println("\n---------------------------------------");
   Serial.println("Öğrenci Numarasını Girin (sonuna # koyun):");
   len = Serial.readBytesUntil('#', (char *) buffer, 16);
   for (byte i = len; i < 16; i++) buffer[i] = ' ';
   blockNum = 4;
   WriteDataToBlock(blockNum, buffer);
   ReadDataFromBlock(blockNum, readBlockData);
   dumpSerial(blockNum, readBlockData);

   Serial.println("\n---------------------------------------");
   Serial.println("Adınızı Girin (sonuna # koyun):");
   len = Serial.readBytesUntil('#', (char *) buffer, 16);
   for (byte i = len; i < 16; i++) buffer[i] = ' ';
   blockNum = 5;
   WriteDataToBlock(blockNum, buffer);
   ReadDataFromBlock(blockNum, readBlockData);
   dumpSerial(blockNum, readBlockData);

   Serial.println("\n---------------------------------------");
   Serial.println("Soyadınızı Girin (sonuna # koyun):");
   len = Serial.readBytesUntil('#', (char *) buffer, 16);
   for (byte i = len; i < 16; i++) buffer[i] = ' ';
   blockNum = 6;
   WriteDataToBlock(blockNum, buffer);
   ReadDataFromBlock(blockNum, readBlockData);
   dumpSerial(blockNum, readBlockData);

   Serial.println("\n---------------------------------------");
   Serial.println("Telefon Numaranızı Girin (sonuna # koyun):");
   len = Serial.readBytesUntil('#', (char *) buffer, 16);
   for (byte i = len; i < 16; i++) buffer[i] = ' ';
   blockNum = 8;
   WriteDataToBlock(blockNum, buffer);
   ReadDataFromBlock(blockNum, readBlockData);
   dumpSerial(blockNum, readBlockData);

   Serial.println("\n---------------------------------------");
   Serial.println("Adresinizi Girin (sonuna # koyun):");
   len = Serial.readBytesUntil('#', (char *) buffer, 16);
   for (byte i = len; i < 16; i++) buffer[i] = ' ';
   blockNum = 9;
   WriteDataToBlock(blockNum, buffer);
   ReadDataFromBlock(blockNum, readBlockData);
   dumpSerial(blockNum, readBlockData);
}

//--------------------------------------------------

void WriteDataToBlock(int blockNum, byte blockData[]) {
   status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
   if (status != MFRC522::STATUS_OK) {
       Serial.print("Blok yazma doğrulaması başarısız: ");
       Serial.println(mfrc522.GetStatusCodeName(status));
       return;
   }

   status = mfrc522.MIFARE_Write(blockNum, blockData, 16);
   if (status != MFRC522::STATUS_OK) {
       Serial.print("Blok yazma işlemi başarısız: ");
       Serial.println(mfrc522.GetStatusCodeName(status));
       return;
   }
}

//--------------------------------------------------

void ReadDataFromBlock(int blockNum, byte readBlockData[]) {
   for (byte i = 0; i < 6; i++) {
       key.keyByte[i] = 0xFF;
   }

   status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
   if (status != MFRC522::STATUS_OK) {
       Serial.print("Blok okuma doğrulaması başarısız: ");
       Serial.println(mfrc522.GetStatusCodeName(status));
       return;
   }

   status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
   if (status != MFRC522::STATUS_OK) {
       Serial.print("Blok okuma işlemi başarısız: ");
       Serial.println(mfrc522.GetStatusCodeName(status));
       return;
   }
}

//--------------------------------------------------

void dumpSerial(int blockNum, byte blockData[]) {
   Serial.print("\nBlok ");
   Serial.print(blockNum);
   Serial.print(" üzerindeki veri: ");
   for (int j=0 ; j<16 ; j++){
       Serial.write(readBlockData[j]);
   }
   Serial.print("\n");

   for(int i = 0; i < sizeof(readBlockData); ++i) {
       readBlockData[i] = 0;
   }
}
