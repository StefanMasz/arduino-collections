#include <Adafruit_NeoPixel.h>
#include <RCSwitch.h>

#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN A1

RCSwitch recevier = RCSwitch();
const int pinSwitch = A2; //Pin Reed
int running = 0;
int statusPhase1 = HIGH;
int outputToMp3Slave = 2;   // PIN to slave 

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(13, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() {
  recevier.enableReceive(3);
  pinMode(outputToMp3Slave, OUTPUT); 
  digitalWrite(outputToMp3Slave, LOW);
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code
  
  pinMode(pinSwitch, INPUT_PULLUP);
  strip.begin();
  strip.setBrightness(50);
  strip.show(); // Initialize all pixels to 'off'
  Serial.begin(9600);
  Serial.println("Monolith");
}

void loop() { 
  statusPhase1 = digitalRead(pinSwitch); 
  if (statusPhase1 == LOW || isRadioSignaltriggert()){ //swtich triggert
    Serial.println("swtich triggert");
    if (running == 1) {
      Serial.println("Stopping");
      digitalWrite(outputToMp3Slave, HIGH);
      running = 0;
      colorWipe(strip.Color(0, 0, 0), 50); // off
    } else {
      Serial.println("Starting");
      digitalWrite(outputToMp3Slave, LOW);
      colorWipe(strip.Color(255, 0, 0), 50); // red
      running = 1; 
    }
  }
  
  if (running == 1) {
    rainbow(30);
  } 
}

int isRadioSignaltriggert (){
  if (recevier.available()) // Wenn ein Code Empfangen wird...
  {
    int value = recevier.getReceivedValue(); // Empfangene Daten werden unter der Variable "value" gespeichert.
  
    if (value == 0) {
      Serial.println("Unbekannter Code");
    } // Wenn die Empfangenen Daten "0" sind, wird "Unbekannter Code" angezeigt.
    
    else {
      Serial.print("Empfangen: ");
      Serial.println( recevier.getReceivedValue() );
      if (value == 1111){
        return true;
      }
    } // Wenn der Empfangene Code brauchbar ist, wird er hier an den Serial Monitor gesendet.

    recevier.resetAvailable(); // Hier wird der Empfänger "resettet"
  }
  return false;
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}


void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    statusPhase1 = digitalRead(pinSwitch); 
    if (statusPhase1 == LOW){ //swtich triggert
       Serial.println("Stopping");
       digitalWrite(outputToMp3Slave, HIGH);
       colorWipe(strip.Color(0, 0, 0), 50); // off
       running = 0;
       break;
    }
    delay(wait);
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
