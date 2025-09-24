#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <MFRC522v2.h>
#include <MFRC522Constants.h>
#include <MFRC522Debug.h>
#include <MFRC522Driver.h>
#include <MFRC522DriverI2C.h>
#include <MFRC522DriverPin.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522Hack.h>

#define LEDPIN A1
#define SUBTRACTBUTTON A2
#define ADDBUTTON A3
int localCounterAddress = 0;  // Define the memory address
#define plotCounterBlockAddress 16

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

MFRC522DriverPinSimple ss_pin(SS_PIN);  //Configurable ss pin
MFRC522DriverSPI driver{ ss_pin };      //Create SPI driver.
MFRC522 mfrc522{ driver };              //Instanciate MFRC522 lib
MFRC522::MIFARE_Key currKey;            //Access key for sectors of card
bool keyType;                           //true to use keyA, false to use keyB

void setup() {
  Serial.begin(9600);  // Initiate a serial communication
  SPI.begin();         // Initiate  SPI bus
  mfrc522.PCD_Init();  //Init MFRC522 board
  //EEPROM.write(localCounterAddress, 0); //reset memory
  setupRFID();

  Serial.println("Approximate your card to the reader...");
  Serial.println();

  pinMode(SUBTRACTBUTTON, INPUT_PULLUP);
  pinMode(ADDBUTTON, INPUT_PULLUP);

  initKey();
  setupLED();
}

void setupRFID(){
  if (mfrc522.PCD_PerformSelfTest()) {
    Serial.println("Card reader ready");
  } else {
    Serial.println("Card reader failed!");
    Serial.println("Resetting...");
    Serial.flush();
  }
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);  //Show details of PCD - MFRC522 Card Reader details
}

void initKey() {
  //desired EE key:
  byte rawKey[6] = { 0x36, 0x5A, 0xC4, 0x22, 0xFE, 0x35 };
  memcpy(currKey.keyByte, rawKey, 6);
}

//Function to reboot by software
void (*resetFunc)(void) = 0;

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

  mfrc522.PCD_SoftPowerUp();
  delay(100);
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

  if (uuid.substring(1) == "89 9F 38 69")  //nur meine karte erlaubt!
  {
    Serial.println("Authorized access - correct card");

    byte buffer[18];
    byte block;
    byte len;

    block = 16;
    len = 18;

    decryptBlock(block, currKey);

    int counter;
    counter = readCounterValue(block, buffer, len);
    int i;
    Serial.print("Buffer: ");
    for (i=0;i<len;i++){
      Serial.print(buffer[i]);
    }
    Serial.println("");
    Serial.print("Counter on card before:");
    Serial.println(counter);

    if (counter % 2 == 0) {  // counter ist gerade - das Transport-Artefakt ist geladen!
      Serial.print("Counter on card is even - starting loading animation!");
      int noOfSuccessfulLoadings = increaseCounter(block, buffer, len);
      mfrc522.PCD_SoftPowerDown();
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
  Serial.println("hoch zaehlen!");
  increasePlotCounter(block, buffer, len);
  int counter;
  counter = readCounterValue(block, buffer, len);
  Serial.print("Counter on card after:");
  Serial.println(counter);

  return noOfSuccessfulLoadings;
}

/**
 * Reads the current plot counter and increases it
 *
 * @return true if write is successful, else false
 */
bool increasePlotCounter(byte block, byte *buffer, byte len) {
  byte counterBlock[18] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  //Initialize buffer
    
  int counter;
  counter = readCounterValue(block, buffer, len);
  counter++;
  byte counterWriteBlock[16] = { counter, 2 };
  Serial.println(counter);
  writeDataToBlock(plotCounterBlockAddress, counterWriteBlock, currKey);
}

int readCounterValue(byte block, byte *buffer, byte len) {
  MFRC522::StatusCode status;
  status = mfrc522.MIFARE_Read(block, buffer, &len);
  if (status != MFRC522Constants::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    return;
  }

  dump_byte_array(buffer, len);
  return (int)buffer[0];
}

void decryptBlock(byte block, MFRC522::MIFARE_Key key) {
  MFRC522::StatusCode status;
  status = mfrc522.PCD_Authenticate(MFRC522Constants::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522Constants::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
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

/*void writeToBlock(byte block, byte buff[]) {
  // Write block
  status = mfrc522.MIFARE_Write(block, buff, 18);
  if (status != MFRC522::) {
    Serial.print(F("MIFARE_Write() failed: "));
    return;
  } else Serial.println(F("MIFARE_Write() success!"));
}*/

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

/*
* @author Daniel Mayer, Kai Krämer
* Last Edited: 2024-06-26 11:30
*/

byte rfidBufferLen = 18;

//Array which blocks are trailer blocks
short trailerBlocks[] = { 3, 7, 11, 15, 19, 23, 27, 31, 35, 39,
                          43, 47, 51, 55, 59, 63, 67, 71, 75, 79,
                          83, 87, 91, 95, 99, 103, 107, 111, 115, 119, 123, 127,
                          143, 159, 175, 191, 207, 223, 239, 255 };

//Array which sectors are writable by 3rd parties
short unsecuredSectorTrailers[] = { 3 };

/**
 * Authenticates to a sector and reads a data block
 *
 * @param blockNum number of block to read from
 * @param readBlockData byte array as destination for read data
 * @param rfidKey the Key to use for authentikation
 * @param keyType true to use keyA, false to use keyB for Authentication
 * @return pointer to byte array with data from card block
 */
bool readDataFromBlock(short blockNum, byte readBlockData[], MFRC522::MIFARE_Key rfidKey, bool keyType) {
  MFRC522::StatusCode status;
  //Source: https://www.electronicshub.org/write-data-to-rfid-card-using-rc522-rfid/
  //Authenticating the desired data block for write access using Key
  if (keyType) {
    status = mfrc522.PCD_Authenticate(MFRC522Constants::PICC_CMD_MF_AUTH_KEY_A, blockNum, &rfidKey, &(mfrc522.uid));
  } else {
    status = mfrc522.PCD_Authenticate(MFRC522Constants::PICC_CMD_MF_AUTH_KEY_B, blockNum, &rfidKey, &(mfrc522.uid));
  }

#ifdef DEBUG
  if (status != MFRC522Constants::STATUS_OK) {
    Serial.print("Authentication failed for Read: ");
    Serial.println(MFRC522Debug::GetStatusCodeName(status));
    return false;
  } else {
    Serial.println("Authentication success");
  }

  //Read data from the Block
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &rfidBufferLen);
  if (status != MFRC522Constants::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(MFRC522Debug::GetStatusCodeName(status));
    return false;
  } else {
    Serial.print("Data was read from Block ");
    Serial.print(blockNum);
    Serial.println(" successfully");
    return true;
  }
#else
  if (status != MFRC522Constants::STATUS_OK) {
    if (status == MFRC522Constants::STATUS_NO_ROOM) {
      resetFunc();
    }
    return false;
  }

  //Read data from the Block
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &rfidBufferLen);
  if (status != MFRC522Constants::STATUS_OK) {
    if (status == MFRC522Constants::STATUS_NO_ROOM) {
      resetFunc();
    }
    return false;
  } else {
    return true;
  }
#endif
}

/**
 * Authenticates to a sector and writes a data block
 *
 * @param blockNum number of block to write to
 * @param blockData byte array to write to card
 * @param rfidKey the Key to use for authentikation
 * @param keyType true to use keyA, false to use keyB for Authentication
 * @return true if write is successful, else false
 */
bool writeDataToBlock(short blockNum, byte blockData[], MFRC522::MIFARE_Key rfidKey) {
  MFRC522::StatusCode status;
  //Source: https://www.electronicshub.org/write-data-to-rfid-card-using-rc522-rfid/
  //Authenticating the desired data block for write access using Key
  status = mfrc522.PCD_Authenticate(MFRC522Constants::PICC_CMD_MF_AUTH_KEY_A, blockNum, &rfidKey, &(mfrc522.uid));
  
  //Print infromation about authentication success
  if (status != MFRC522Constants::STATUS_OK) {
    Serial.print("Authentication failed for Write: ");
    Serial.println(MFRC522Debug::GetStatusCodeName(status));
    return false;
  } else {
    Serial.println("Authentication success");
  }

  //Write data to the block
  status = mfrc522.MIFARE_Write(blockNum, blockData, 16);
  //Print information about data transfer to card
  if (status != MFRC522Constants::STATUS_OK) {
    Serial.print("Writing to Block failed: ");
    Serial.println(MFRC522Debug::GetStatusCodeName(status));
    return false;
  } else {
    Serial.print("Data was written into Block ");
    Serial.print(blockNum);
    Serial.println(" successfully");
    return true;
  }
}


/**
 * Searches for cards in ready state and selects it to active state
 *
 * @return true if card activation was successful
 */
bool connectCard() {
  // Look for new cards
  if (mfrc522.PICC_IsNewCardPresent()) {
    // Select one of the cards
    if (mfrc522.PICC_ReadCardSerial()) {
      return true;
    }
  }
  return false;
}

/**
 * Identifies if a block is a trailer block
 *
 * @param blockNum number of block to check
 * @return true if block is trailer block, else false
 */
bool isBlockTrailer(short blockNum) {
  for (short i = 0; i < sizeof(trailerBlocks) / sizeof(short); i++) {
    if (blockNum == trailerBlocks[i]) {
      return true;
    }
  }
  return false;
}

/**
 * Transforms a byte Array to MIFARE Key struct
 *
 * @param keyBytes bytes for new MIFARE Key
 * @return MIFARE Key struct with given bytes
 */
MFRC522::MIFARE_Key writeKeyBytesToKey(byte keyBytes[6]) {
  MFRC522::MIFARE_Key retKey;
  for (int i = 0; i < 6; i++) {  //Write key Bytes to key structs
    retKey.keyByte[i] = keyBytes[i];
  }
  return retKey;
}