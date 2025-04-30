#include <Arduino.h>
 #include <ESP8266WiFi.h>
 #include <SPI.h>
 #include <MFRC522.h>
 #include <HTTPSRedirect.h>
 #include<Wire.h>
 #include<LiquidCrystal_I2C.h>
 

 LiquidCrystal_I2C lcd(0x27, 16, 2);
 //---------------------------------------------------------------------------------------------------------
 // Google Script Dağıtım Kimliğini Girin:
 const char *GScriptId = "AKfycbwcvfgSUjj4bbN4QI9_ueXfim1PP3ctOsOp_kUOgOfEPysi_QSFdqRv_h-kDUUDoIJl";
 String gate_number = "Gate1";
 //---------------------------------------------------------------------------------------------------------
 // Ağ kimlik bilgilerini girin:
 const char* ssid = "TurkTelekom_ZTCCA";
 const char* password = "01414F1443cDd";
 //---------------------------------------------------------------------------------------------------------
 // Komutu girin (insert_row veya append_row) ve Google Sheets sayfa adınızı (varsayılan: Sheet1):
 String payload_base =  "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
 String payload = "";
 //---------------------------------------------------------------------------------------------------------
 // Google Sheets ayarları (düzenlemeyin)
 const char* host        = "script.google.com";
 const int   httpsPort   = 443;
 const char* fingerprint = "";
 String url = String("/macros/s/") + GScriptId + "/exec";
 HTTPSRedirect* client = nullptr;
 //------------------------------------------------------------
 // Google Sheets'e gönderilecek değişkenleri tanımlayın
 String student_id;
 //------------------------------------------------------------
 int blocks[] = {4,5,6,8,9};
 #define total_blocks  (sizeof(blocks) / sizeof(blocks[0]))
 //------------------------------------------------------------
 #define RST_PIN  0  //D3
 #define SS_PIN   2  //D4
 #define BUZZER   4  //D2
 //------------------------------------------------------------
 MFRC522 mfrc522(SS_PIN, RST_PIN);
 MFRC522::MIFARE_Key key;  
 MFRC522::StatusCode status;

 //------------------------------------------------------------
 /* Sektör Römork Bloklarına dikkat edin */
 int blockNum = 2;  
 /* Bloktan veri okumak için başka bir dizi oluşturun */
 /* Tamponun (buffer) uzunluğu, Blok boyutundan (16 Bayt) 2 Bayt daha fazla olmalıdır */
 byte bufferLen = 18;
 byte readBlockData[18];
 //------------------------------------------------------------
 
 /****************************************************************************************************
  * setup Fonksiyonu
 ****************************************************************************************************/
 void setup() {
   //----------------------------------------------------------
   Serial.begin(9600);        
   delay(10);
   Serial.println('\n');
   //----------------------------------------------------------
   SPI.begin();
   //----------------------------------------------------------
   // LCD ekranı başlatın
   lcd.init();
   // turn on the backlight
   lcd.backlight();
   lcd.clear();
   lcd.setCursor(0,0); //col=0 row=0
   lcd.print("Connecting to");
   lcd.setCursor(0,1); //col=0 row=0
   lcd.print("WiFi...");
   //----------------------------------------------------------
   // WiFi'ye bağlan
   WiFi.begin(ssid, password);             
   Serial.print("Connecting to ");
   Serial.print(ssid); Serial.println(" ...");
   
   while (WiFi.status() != WL_CONNECTED) {
     delay(1000);
     Serial.print(".");
   }
   Serial.println('\n');
   Serial.println("WiFi Connected!");
   /// Serial.print("IP adresi:\t");
   Serial.println(WiFi.localIP());
   //----------------------------------------------------------
   // Yeni bir TLS bağlantısı oluşturmak için HTTPSRedirect sınıfını kullanın
   client = new HTTPSRedirect(httpsPort);
   client->setInsecure();
   client->setPrintResponseBody(true);
   client->setContentTypeHeader("application/json");
   //----------------------------------------------------------
   lcd.clear();
   lcd.setCursor(0,0); //col=0 row=0
   lcd.print("Connecting to");
   lcd.setCursor(0,1); //col=0 row=0
   lcd.print("Google ");
   delay(5000);
   //----------------------------------------------------------
   Serial.print("Connecting to ");
   Serial.println(host);
   //----------------------------------------------------------
   // Maksimum 5 kez bağlanmayı deneyin
   bool flag = false;
   for(int i=0; i<5; i++){ 
     int retval = client->connect(host, httpsPort);
     //*************************************************
     if (retval == 1){
       flag = true;
       String msg = "Connected. OK";
       Serial.println(msg);
       lcd.clear();
       lcd.setCursor(0,0); //col=0 row=0
       lcd.print(msg);
       delay(2000);
       break;
     }
     //*************************************************
     else
       Serial.println("Connection failed. Retrying...");
     //*************************************************
   }
   //----------------------------------------------------------
   if (!flag){
     //____________________________________________
     lcd.clear();
     lcd.setCursor(0,0); //col=0 row=0
     lcd.print("Connection fail");
     //____________________________________________
     Serial.print("Could not connect to server: ");
     Serial.println(host);
     delay(5000);
     return;
     //____________________________________________
   }
   //----------------------------------------------------------
   delete client;    // HTTPSRedirect nesnesini silin
   client = nullptr; // HTTPSRedirect nesnesini sil
   //----------------------------------------------------------
 }
 
 /****************************************************************************************************
  * loop Fonksiyonu
 ****************************************************************************************************/
 void loop() {
   // Serial.println("[TEST] döngü başlıyor");
   //----------------------------------------------------------------
   static bool flag = false;
   if (!flag){
     client = new HTTPSRedirect(httpsPort);
     client->setInsecure();
     flag = true;
     client->setPrintResponseBody(true);
     client->setContentTypeHeader("application/json");
   }
   if (client != nullptr){
     // Aşağıdaki if koşulu TRUE olduğunda, normalden daha fazla zaman alır, bu cihazın
     // Google Sheets sunucusundan bağlantının kesildiği ve tekrar bağlanmanın zaman aldığı anlamına gelir
     //NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
     if (!client->connected()){
       int retval = client->connect(host, httpsPort);
       if (retval != 1){
         Serial.println("Disconnected. Retrying...");
         lcd.clear();
         lcd.setCursor(0,0); //col=0 row=0
         lcd.print("Disconnected.");
         lcd.setCursor(0,1); //col=0 row=0
         lcd.print("Retrying...");
         return; // Döngüyü sıfırla
       }
     }
     //NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
   }
   else{Serial.println("Error creating client object!"); Serial.println("else");}
   //----------------------------------------------------------------
   lcd.clear();
   lcd.setCursor(0,0); //col=0 row=0
   lcd.print("Scan your Tag");
   // Serial.println("[TEST] Etiketinizi okutun");
   /* MFRC522 Modülünü başlatın */
   mfrc522.PCD_Init();
   /* Yeni kartları arayın */
   /* RC522 Okuyucuda yeni bir kart yoksa döngüyü sıfırla */
   if ( ! mfrc522.PICC_IsNewCardPresent()) {return;}
   /* Kartlardan birini seçin */
   if ( ! mfrc522.PICC_ReadCardSerial()) {return;}
   /* Aynı bloktan veri okuyun */
   Serial.println();
   Serial.println(F("Reading last data from RFID..."));  
   //----------------------------------------------------------------
   String values = "", data;
   /*
   // Yük oluşturuluyor - yöntem 1
   //----------------------------------------------------------------
   öğrenci kimliği (student id)
   Veriyi stringe dönüştür ve boşlukları temizle
   Öğrenci kimliğini (student ID) veriye ata
   //----------------------------------------------------------------
   ad (first name)  
   Veriyi stringe dönüştür ve boşlukları temizle  
   Ad bilgisini veriye ata
   //----------------------------------------------------------------
   soyad (last name)  
   Veriyi stringe dönüştür ve boşlukları temizle  
   Soyad bilgisini veriye ata
   //----------------------------------------------------------------
   telefon numarası (phone number)  
   Veriyi stringe dönüştür ve boşlukları temizle  
   Telefon numarası bilgisini veriye ata
   //----------------------------------------------------------------
   adres (address)  
   Veriyi stringe dönüştür ve boşlukları temizle  
   Adres bilgisini veriye ata ve geçici değişkeni temizle
   //----------------------------------------------------------------
   values = "\"" + student_id + ","; // Değerler dizisini öğrenci kimliği ile başlat  
   values += first_name + ","; // Ad bilgisini ekle  
   values += last_name + ","; // Soyad bilgisini ekle  
   values += phone_number + ","; // Telefon numarasını ekle  
   values += address + "\"}"; // Adres bilgisini ekleyerek JSON formatını kapat
   //----------------------------------------------------------------*/
   // Yük oluşturuluyor - yöntem 2 - Daha verimli
   for (byte i = 0; i < total_blocks; i++) {
     ReadDataFromBlock(blocks[i], readBlockData);
     //*************************************************
     if(i == 0){
       data = String((char*)readBlockData);
       data.trim();
       student_id = data;
       values = "\"" + data + ",";
     }
     //*************************************************
  /* else if(i == total_blocks-1) {
       data = String((char*)readBlockData); // Veriyi stringe dönüştür
       data.trim(); // Boşlukları temizle
       values += data + "\"}"; // Son veriyi ekleyerek JSON formatını kapat
     } */

     //*************************************************
     else{
       data = String((char*)readBlockData);
       data.trim();
       values += data + ",";
     }
   }
   values += gate_number + "\"}";
   //----------------------------------------------------------------
   // Google Sheets'e göndermek için JSON nesne dizesi oluşturun
   // values = "\"" + value0 + "," + value1 + "," + value2 + "\"}"
   payload = payload_base + values;
   //----------------------------------------------------------------
   lcd.clear();
   lcd.setCursor(0,0); //col=0 row=0
   lcd.print("Publishing Data");
   lcd.setCursor(0,1); //col=0 row=0
   lcd.print("Please Wait...");
   //----------------------------------------------------------------
   // Verileri Google Sheets'e yayınla
   Serial.println("Publishing data...");
   Serial.println(payload);
   if(client->POST(url, host, payload)){ 
     // Yayınlama başarılı olduysa burada işlemleri gerçekleştir
     Serial.println("[OK] Data published.");
     lcd.clear();
     lcd.setCursor(0,0); //col=0 row=0
     lcd.print("Student ID: "+student_id);
     lcd.setCursor(0,1); //col=0 row=0
     lcd.print("Thanks");
   }
   //----------------------------------------------------------------
   else{
     // Yayınlama başarısız olduysa burada işlemleri gerçekleştir
     Serial.println("Error while connecting");
     lcd.clear();
     lcd.setCursor(0,0); //col=0 row=0
     lcd.print("Failed.");
     lcd.setCursor(0,1); //col=0 row=0
     lcd.print("Try Again");
   }
   //----------------------------------------------------------------
   // Tekrar yayınlamadan önce birkaç saniyelik bir gecikme gereklidir    
   Serial.println("[TEST] delay(5000)");
   delay(5000);
 }
 
 
 /****************************************************************************************************
  * 
 ****************************************************************************************************/
 /****************************************************************************************************
  * ReadDataFromBlock() fonksiyonu
  ****************************************************************************************************/
 void ReadDataFromBlock(int blockNum, byte readBlockData[]) 
 { 
   //----------------------------------------------------------------------------
   /* Kimlik doğrulama için anahtarı hazırla */
   /* Tüm anahtarlar, fabrikadan teslim edildiğinde FFFFFFFFFFFFh olarak ayarlanmıştır */
   for (byte i = 0; i < 6; i++) {
     key.keyByte[i] = 0xFF;
   }
   //----------------------------------------------------------------------------
   /* Okuma erişimi için istenen veri bloğunu Anahtar A kullanarak kimlik doğrulama */
   status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
   //----------------------------------------------------------------------------s
   if (status != MFRC522::STATUS_OK){
      Serial.print("Authentication failed for Read: ");
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
   }
   //----------------------------------------------------------------------------
   else {
     Serial.println("Authentication success");
   }
   //----------------------------------------------------------------------------
   /* Bloktan veri okunuyor */
   status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
   if (status != MFRC522::STATUS_OK) {
     Serial.print("Reading failed: ");
     Serial.println(mfrc522.GetStatusCodeName(status));
     return;
   }
   //----------------------------------------------------------------------------
   else {
     readBlockData[16] = ' ';
     readBlockData[17] = ' ';
     Serial.println("Block was read successfully");  
   }
   //----------------------------------------------------------------------------
 } 