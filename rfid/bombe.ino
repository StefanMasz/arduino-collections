#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#define LEDPIN A1
#define SUBTRACTBUTTON A2
#define ADDBUTTON A3
int localCounterAddress = 0;  // Define the memory address

// Globale Variablen für Animation
unsigned long lastUpdate = 0;
int brightness = 0;
int fadeDirection = 1;  // 1 = heller, -1 = dunkler

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(28, LEDPIN, NEO_GRB + NEO_KHZ800);

#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

void setup() {
  Serial.begin(9600);  // Initiate a serial communication
  SPI.begin();         // Initiate  SPI bus
  mfrc522.PCD_Init();  // Initiate MFRC522
  //EEPROM.write(localCounterAddress, 0); //reset memory

  Serial.println("Approximate your card to the reader...");
  Serial.println();

  pinMode(SUBTRACTBUTTON, INPUT_PULLUP);
  pinMode(ADDBUTTON, INPUT_PULLUP);

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
  int noOfSuccessfulLoadings = EEPROM.read(localCounterAddress);
  Serial.print("noOfSuccessfulLoadings on start up:");
  Serial.println(noOfSuccessfulLoadings);
}

void oneColor(uint32_t color, int ledCount) {
  for (uint16_t i = 0; i < ledCount; i = i + 1) {
    strip.setPixelColor(i, color);
    strip.show();
  }
}

void renderState(int noOfSuccessfulLoadings) {
  unsigned long currentMillis = millis();

  // Update alle X ms
  int updateInterval = 30;
  int speed = 10;
  char color = 'g';

  switch (noOfSuccessfulLoadings) {
    case 1:
      color = 'g';
      updateInterval = 30;
      speed = 7;
      break;
    case 2:
      color = 'b';
      updateInterval = 20;
      speed = 12;
      break;
    case 3:
      color = 'b';
      updateInterval = 15;
      speed = 15;
      break;
    case 4:
      color = 'r';
      updateInterval = 15;
      speed = 15;
      break;
    case 5:
      color = 'r';
      updateInterval = 10;
      speed = 20;
      break;
    default:
      color = 'r';
      updateInterval = 10;
      speed = 20;
      break;
  }

  if (currentMillis - lastUpdate >= updateInterval) {
    lastUpdate = currentMillis;

    // Helligkeit anpassen
    brightness += fadeDirection * speed;
    if (brightness >= 255) {
      brightness = 255;
      fadeDirection = -1;
    } else if (brightness <= 0) {
      brightness = 0;
      fadeDirection = 1;
    }

    // Berechne Anzahl LEDs, die leuchten sollen
    int ledsToLight = noOfSuccessfulLoadings * 7;
    if (ledsToLight > strip.numPixels()) {
      ledsToLight = strip.numPixels();
    }

    updateLEDs(ledsToLight, color);
  }
}

void updateLEDs(int ledsToLight, char color) {
  // LEDs aktualisieren
  for (int i = 0; i < strip.numPixels(); i++) {
    if (i < ledsToLight) {
      if (color == 'g') {
        strip.setPixelColor(i, strip.Color(0, brightness, 0));  // Pulsierend grün
      } else if (color == 'r') {
        strip.setPixelColor(i, strip.Color(brightness, 0, 0));  // Pulsierend rot
      } else {
        strip.setPixelColor(i, strip.Color(0, 0, brightness));  // Pulsierend blau
      }
    } else {
      strip.setPixelColor(i, 0);  // Aus
    }
  }
  strip.show();
}

void loop() {
  int noOfSuccessfulLoadings = EEPROM.read(localCounterAddress);
  renderState(noOfSuccessfulLoadings);
  int addButtonValue = digitalRead(ADDBUTTON);
  int subButtonValue = digitalRead(SUBTRACTBUTTON);

  if (addButtonValue == LOW) {
    delay(300);
    loadingAnimation();
    int noOfSuccessfulLoadings = increaseMemCounter();
  }

  if (subButtonValue == LOW) {
    delay(300);
    loadingAnimation();
    int noOfSuccessfulLoadings = decreaseMemCounter();
  }

  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  String uuid = "";
  uuid = readUUID();

  if (uuid.substring(1) == "43 0D FE 27")  //nur meine karte erlaubt!
  {
    Serial.println("Authorized access - correct card");

    byte buffer[18];
    byte block;
    byte len;

    block = 16;
    len = 18;

    decryptBlock(block, key);

    int counter;
    counter = readCounterValue(block, buffer, len);
    Serial.print("Counter on card before:");
    Serial.println(counter);

    if (counter % 2 == 0) {  // counter ist gerade - das Transport-Artefakt ist geladen!
      Serial.print("Counter on card is even - starting loading animation!");
      int noOfSuccessfulLoadings = increaseCounter(block, buffer, len);
      loadingAnimation();
    } else {
      Serial.print("Counter on card is uneven - do nothing!");
    }
  }

  else {
    Serial.println("Access denied - unknown card");
    return;
  }

  Serial.println(F("\n**End**\n"));

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

int increaseMemCounter() {
  int noOfSuccessfulLoadings = EEPROM.read(localCounterAddress);
  Serial.print("Counter in memory before:");
  Serial.println(noOfSuccessfulLoadings);
  noOfSuccessfulLoadings++;
  EEPROM.write(localCounterAddress, noOfSuccessfulLoadings);
  Serial.print("Counter in memory after:");
  Serial.println(noOfSuccessfulLoadings);
  return noOfSuccessfulLoadings;
}

int decreaseMemCounter() {
  int noOfSuccessfulLoadings = EEPROM.read(localCounterAddress);
  Serial.print("Counter in memory before:");
  Serial.println(noOfSuccessfulLoadings);
  if (noOfSuccessfulLoadings == 0) {
    return noOfSuccessfulLoadings;
  }
  noOfSuccessfulLoadings--;
  EEPROM.write(localCounterAddress, noOfSuccessfulLoadings);
  Serial.print("Counter in memory after:");
  Serial.println(noOfSuccessfulLoadings);
  return noOfSuccessfulLoadings;
}

int increaseCounter(byte block, byte *buffer, byte len) {
  int noOfSuccessfulLoadings = increaseMemCounter();

  //Reset card to start Value
  //buffer[0] = 1;

  buffer[0]++;
  buffer[1] = 0x02;

  writeToBlock(block, buffer);
  int counter;
  counter = readCounterValue(block, buffer, len);
  Serial.print("Counter on card after:");
  Serial.println(counter);

  return noOfSuccessfulLoadings;
}

int readCounterValue(byte block, byte *buffer, byte len) {
  status = mfrc522.MIFARE_Read(block, buffer, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  dump_byte_array(buffer, len);
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

void writeToBlock(byte block, byte buff[]) {
  // Write block
  status = mfrc522.MIFARE_Write(block, buff, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  } else Serial.println(F("MIFARE_Write() success!"));
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

void loadingAnimation() {
  int cycles = 3;             // Wie oft die Welle durchläuft
  int delayMs = 40;           // Geschwindigkeit der Animation
  int tailLength = 8;         // Länge des Schweifs (mehr LEDs gleichzeitig)

  for (int c = 0; c < cycles; c++) {
        for (int head = 0; head < strip.numPixels() + tailLength; head++) {
      strip.clear();
      for (int t = 0; t < tailLength; t++) {
        int ledIndex = head - t;
        uint32_t color = strip.Color(random(50, 256), random(50, 256), random(50, 256));
        if (ledIndex >= 0 && ledIndex < strip.numPixels()) {
          // Schweif wird dunkler
          int brightness = 255 - (t * (255 / tailLength));
          uint32_t dimColor = strip.Color(
            ((color >> 16) & 0xFF) * brightness / 255,
            ((color >> 8) & 0xFF) * brightness / 255,
            (color & 0xFF) * brightness / 255
          );
          strip.setPixelColor(ledIndex, dimColor);
        }
      }
      strip.show();
      delay(delayMs);
    }
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
