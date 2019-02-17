#include <deprecated.h>
#include <MFRC522.h>
#include <require_cpp11.h>
#include <Adafruit_NeoPixel.h>

/*
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 */
 
#include <SPI.h>
#define NEOPIXELPIN 5
#define RST_PIN   9     // SPI Reset Pin
#define SS_PIN    10    // SPI Slave Select Pin
 
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Instanz des MFRC522 erzeugen

byte reset_uid[] = {0xD6, 0x5D, 0xC1, 0x93};
byte red_uid[] = {0xA6, 0x3D, 0xC5, 0x93};
byte green_uid[] = {0xE6, 0xF9, 0xC3, 0x93};
byte pink_uid[] = {0x57, 0xC3, 0x3C, 0x5B};
byte blue_uid[] = {0xAE, 0x2A, 0x8B, 0xAB};
byte disco_uid[] = {0xC1, 0x79, 0x91, 0xAB};

int reset_check = false;
int red_check = false;
int blue_check = false;
int green_check = false;
int pink_check = false;
int disco_check = false;

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(2, NEOPIXELPIN, NEO_GRB + NEO_KHZ800); 
 
void setup() {
  // Diese Funktion wird einmalig beim Start ausgeführt
  Serial.begin(9600);  // Serielle Kommunikation mit dem PC initialisieren
  SPI.begin();         // Initialisiere SPI Kommunikation
  mfrc522.PCD_Init();  // Initialisiere MFRC522 Lesemodul
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}
 
void loop() {
  // Diese Funktion wird in Endlosschleife ausgeführt
  // Nur wenn eine Karte gefunden wird und gelesen werden konnte, wird der Inhalt von IF ausgeführt
  // PICC = proximity integrated circuit card = kontaktlose Chipkarte
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial() ) {
    Serial.print("Gelesene UID:");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      // Abstand zwischen HEX-Zahlen und führende Null bei Byte < 16
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
    } 
    Serial.println();

    // UID Vergleichen mit reset_uid
    reset_check = true;
    for (int j=0; j<4; j++) {
      if (mfrc522.uid.uidByte[j] != reset_uid[j]) {
        reset_check = false;
      }
    }
 
    // UID Vergleichen mit red_uid
    red_check = true;
    for (int j=0; j<4; j++) {
      if (mfrc522.uid.uidByte[j] != red_uid[j]) {
        red_check = false;
      }
    }

    // UID Vergleichen mit blue_uid
    blue_check = true;
    for (int j=0; j<4; j++) {
      if (mfrc522.uid.uidByte[j] != blue_uid[j]) {
        blue_check = false;
      }
    }

    // UID Vergleichen mit pink_uid
    pink_check = true;
    for (int j=0; j<4; j++) {
      if (mfrc522.uid.uidByte[j] != pink_uid[j]) {
        pink_check = false;
      }
    }

    // UID Vergleichen mit green_uid
    green_check = true;
    for (int j=0; j<4; j++) {
      if (mfrc522.uid.uidByte[j] != green_uid[j]) {
        green_check = false;
      }
    }

    // UID Vergleichen mit green_uid
    disco_check = true;
    for (int j=0; j<4; j++) {
      if (mfrc522.uid.uidByte[j] != disco_uid[j]) {
        disco_check = false;
      }
    }
     
    if (reset_check) {
      oneColor(strip.Color(0, 0, 0));
    }
         
    if (disco_check) {
      oneColor(strip.Color(255, 255, 255));
    }
 
    if (green_check) {
      oneColor(strip.Color(0, 255, 0));
    }
     
    if (blue_check) {
      oneColor(strip.Color(0, 0, 255));
    }
 
    if (red_check) {
      oneColor(strip.Color(255, 0, 0));
    }
     
    if (pink_check) {
      oneColor(strip.Color(255, 5, 100));
    }

    // Versetzt die gelesene Karte in einen Ruhemodus, um nach anderen Karten suchen zu können.
    mfrc522.PICC_HaltA();
    delay(1000);
  }
}

void oneColor (uint32_t color){
  for (uint16_t i=0; i < strip.numPixels(); i = i +1) {
      strip.setPixelColor(i, color);
      strip.show(); 
  }
}
