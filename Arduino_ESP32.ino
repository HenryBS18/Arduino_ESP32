#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

// Firebase Library
#include <Firebase_ESP_Client.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "p"
#define WIFI_PASSWORD "12345678"

#define pinLocker_1 32
#define pinLocker_2 33

// Insert Firebase project API Key
#define API_KEY "AIzaSyDEGTZFAiPJlZLqGXunyGbbvz3ggyFTFTU"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://lokerkufirebase-default-rtdb.asia-southeast1.firebasedatabase.app/" 

// RFID Library
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  5  // ESP32 pin GIOP5 
#define RST_PIN 13   // ESP32 pin GIOP27 

// RFID
MFRC522 rfid(SS_PIN, RST_PIN);

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int intValue;
float floatValue;
bool signupOK = false;

void setup() {
  Serial.begin(115200);

  // Set The pinMode
  pinMode(pinLocker_1, OUTPUT); // loker 1
  pinMode(pinLocker_2, OUTPUT); // loker 2

  // D2 LED
  pinMode(2, OUTPUT);

  // WiFi Connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  }
  else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  SPI.begin(); // init SPI bus
  rfid.PCD_Init(); // init MFRC522
}

int prevIntValue = -1;
int prevIntValue2 = -1;

void loop() {
  // Firebase
  if (Firebase.ready() && signupOK) {

    // Locker 1
    if (Firebase.RTDB.getInt(&fbdo, "/Loker/loker_1/status")) {
      if (fbdo.dataType() == "int") {
        intValue = fbdo.intData();
        if (intValue != prevIntValue) {
          prevIntValue = intValue;
          Serial.println(intValue);
          
          // LED
          if (intValue == 1) {
            digitalWrite(pinLocker_1, HIGH);
            digitalWrite(2, HIGH);

            Serial.println("ON");
          }
          else {
            digitalWrite(pinLocker_1, LOW);
            digitalWrite(2, LOW);
            Serial.println("OFF");
          }          
        }
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }

    // Locker 2
    if (Firebase.RTDB.getInt(&fbdo, "/Loker/loker_2/status")) {
      if (fbdo.dataType() == "int") {
        intValue = fbdo.intData();
        if (intValue != prevIntValue2) {
          prevIntValue2 = intValue;
          Serial.println(intValue);
          
          // LED
          if (intValue == 1) {
            digitalWrite(pinLocker_2, HIGH);
            Serial.println("ON");
          }
          else {
            digitalWrite(pinLocker_2, LOW);
            Serial.println("OFF");
          }          
        }
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }
  }

  // RFID 
  if (rfid.PICC_IsNewCardPresent()) { // new tag is available
    if (rfid.PICC_ReadCardSerial()) { // NUID has been readed
      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
      Serial.print("RFID/NFC Tag Type: ");
      Serial.println(rfid.PICC_GetTypeName(piccType));

      // print UID in Serial Monitor in the hex format
      Serial.print("UID:");
      String UID = "";
      for (int i = 0; i < rfid.uid.size; i++) {
        Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(rfid.uid.uidByte[i], HEX);
        UID += rfid.uid.uidByte[i], HEX;
        // rfid.uid.
        
      }

      if (UID.equals("671211557")){
         digitalWrite(pinLocker_2, HIGH);
         delay(2000);
         digitalWrite(pinLocker_2, LOW);

        Serial.print("Loker 2 Terbuka");
      }
      if (UID.equals("21516015096")){
         digitalWrite(pinLocker_1, HIGH);
         delay(2000);
         digitalWrite(pinLocker_1, LOW);

        Serial.print("Loker 1 Terbuka");
      }

      rfid.PICC_HaltA(); // halt PICC
      rfid.PCD_StopCrypto1(); // stop encryption on PCD
    }
  }
}