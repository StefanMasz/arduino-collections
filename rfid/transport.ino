#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#define LEDPIN A1

// Globale Variablen für Animation
unsigned long lastRainbowUpdate = 0;
int rainbowPosition = 0;
bool rainbowActive = false;

//
unsigned long lastRFIDCheck = 0;
const unsigned long rfidCheckInterval = 5000;

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(6, LEDPIN, NEO_GRB + NEO_KHZ800);

#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;
const int reedPin = 6;

void setup() {
  Serial.begin(9600);  // Initiate a serial communication
  SPI.begin();         // Initiate  SPI bus
  mfrc522.PCD_Init();  // Initiate MFRC522

  pinMode(reedPin, INPUT_PULLUP);
  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);

  Serial.println("Approximate your card to the reader...");
  Serial.println();

  initKey();
  setupLED();
}

void initKey() {
  //desired EE key:
  byte rawKey[6] = { 0x36, 0x5A, 0xC4, 0x22, 0xFE, 0x35 };
  memcpy(key.keyByte, rawKey, 6);
}

void setupLED() {
  strip.begin();
  strip.setBrightness(255);
  strip.show();  // Initialize all pixels to 'off'
}

void oneColor(uint32_t color, int ledCount) {
  for (uint16_t i = 0; i < ledCount; i = i + 1) {
    strip.setPixelColor(i, color);
    strip.show();
  }
}

void updateRainbowNonBlocking() {
  const unsigned long rainbowInterval = 5;  // Sehr schnell! (kleinere Zahl = schneller)
  if (!rainbowActive) return;

  unsigned long currentMillis = millis();
  if (currentMillis - lastRainbowUpdate >= rainbowInterval) {
    lastRainbowUpdate = currentMillis;

    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      // Jeder Pixel bekommt einen leicht versetzten Farbbereich
      int colorPosition = (rainbowPosition + i * 90) % 256;
      strip.setPixelColor(i, wheel(colorPosition));
    }

    strip.show();
    rainbowPosition = (rainbowPosition + 9) % 256;  // schneller vorwärts!
  }
}

int readCounterWrapper(){
  byte block = 16;
  byte len = 18;
  decryptBlock(block, key);
  byte buffer[18];

  return readCounterValue(block, buffer, 18);
}

bool readCard() {
  mfrc522.PCD_Init();  // HACK: Reset des Lesers erzwingt "neue Karte"
  delay(5);
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    return true;
  }
  return false;
}

unsigned long openSince = 0;
bool initialReadDone = false;
unsigned long lastRFIDAttempt = 0;
const unsigned long RFID_INTERVAL = 10000; // 10 Sekunden

void loop() {
  updateRainbowNonBlocking();
  delay(20);

  unsigned long currentMillis = millis();
  int state = digitalRead(reedPin);

  if (state == HIGH) {  // offen (kein Magnet)
    if (openSince == 0) {
      openSince = currentMillis;
      initialReadDone = false;
      lastRFIDAttempt = 0;
    }

    if (currentMillis - openSince >= 2000) {
      if (!initialReadDone || currentMillis - lastRFIDAttempt >= RFID_INTERVAL) {
        initialReadDone = true;
        lastRFIDAttempt = currentMillis;

        Serial.println("RFID-Check nach Kiste offen >= 2s");
        mfrc522.PCD_SoftPowerUp();
        delay(50);
        if (readCard()) {
          Serial.println("Karte ist da!");
          String uuid = readUUID();
          if (uuid.substring(1) == "43 0D FE 27") {
            int counter = readCounterWrapper();
            Serial.print("Counter: ");
            Serial.println(counter);
            mfrc522.PCD_SoftPowerDown();
            if (counter % 2 == 0) {
              Serial.println("Even counter - LEDs ON");
              rainbowActive = true;
            } else {
              Serial.println("Odd counter - LEDs OFF");
              rainbowActive = false;
              oneColor(0, 6);
            }
          } else {
            Serial.println("Access denied - unknown card");
          }
        } else {
          Serial.println("Karte ist NICHT da!");
        }
      }
    }
  } else {  // geschlossen
    if (openSince != 0) {
      Serial.println("Kiste zu!");
    }
    openSince = 0;
    initialReadDone = false;
    lastRFIDAttempt = 0;
    delay(200);
  }
}

int readCounterValue(byte block, byte *buffer, byte len) {
  status = mfrc522.MIFARE_Read(block, buffer, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  dump_byte_array(buffer, len);
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  return (int)buffer[0];
}

void decryptBlock(byte block, MFRC522::MIFARE_Key key) {
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
}

String readUUID() {
  //Show UID on serial monitor
  Serial.print("UID tag :");
  String content = "";
  byte letter;

  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  return content;
}


/*
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
  Serial.println("");
}

void flashAllLeds(uint32_t color, int durationMs, int repetitions) {
  for (int i = 0; i < repetitions; i++) {
    // Turn all LEDs ON
    for (int j = 0; j < strip.numPixels(); j++) {
      strip.setPixelColor(j, color);
    }
    strip.show();
    delay(durationMs);

    // Turn all LEDs OFF
    for (int j = 0; j < strip.numPixels(); j++) {
      strip.setPixelColor(j, 0);  // Black = off
    }
    strip.show();
    delay(durationMs);
  }
}

// Helper-Funktion für Regenbogenfarben
uint32_t wheel(byte pos) {
  if (pos < 85) {
    return strip.Color(pos * 3, 255 - pos * 3, 0);
  } else if (pos < 170) {
    pos -= 85;
    return strip.Color(255 - pos * 3, 0, pos * 3);
  } else {
    pos -= 170;
    return strip.Color(0, pos * 3, 255 - pos * 3);
  }
}

/*
void writeEEKeyToSektor4() {
  byte trailerBlock = 19;
  MFRC522::MIFARE_Key defaultKey;
  for (byte i = 0; i < 6; i++) defaultKey.keyByte[i] = 0xFF; // default key

  byte sectorTrailerData[16] = {
    0x36, 0x5A, 0xC4, 0x22, 0xFE, 0x35, // Key A
    0xFF, 0x07, 0x80, 0x69,             // Access Bits
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  // Key B (optional)
  };

  // Warten auf Karte
  Serial.println("Karte auflegen...");
  while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial());

  // Authentifizieren mit default Key A
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &defaultKey, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // Schreiben
  status = mfrc522.MIFARE_Write(trailerBlock, sectorTrailerData, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Write failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  Serial.println("Sektor-Trailer erfolgreich geschrieben!");
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
*/
