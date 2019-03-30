#include <RCSwitch.h>

RCSwitch sender = RCSwitch();
const int pinSwitch = 10; //Pin Reed1
const int pinSwitchSender = 8; //Pin Reed2
const int pinSender = 7; //Pin Sender
const int pinFire1 = 2; //Pin Rohr 1
const int pinFire2 = 3; //Pin Rohr 2
const int pinFire3 = 4; //Pin Rohr 3
const int pinFire4 = 5; //Pin Rohr 4

int actionFire = 0;
int statusPhase1 = HIGH;
int statusPhase2 = HIGH;
int statusSwitchSender = HIGH;
int statusSwitchSenderPhase2 = HIGH;

void setup()
{
  Serial.begin(9600);
  Serial.println("Start");
  sender.enableTransmit(pinSender);  
  pinMode(pinFire1, OUTPUT);
  pinMode(pinFire2, OUTPUT);
  pinMode(pinFire3, OUTPUT);
  pinMode(pinFire4, OUTPUT); 
  pinMode(pinSwitch, INPUT_PULLUP);
  pinMode(pinSwitchSender, INPUT_PULLUP);
}  
  
void loop()
{
  statusSwitchSender = digitalRead(pinSwitchSender);
  if (statusSwitchSender == LOW){
    Serial.println("Sender Phase 1 Active");
    delay(1000);
    statusSwitchSenderPhase2 = digitalRead(pinSwitchSender);
  }
  if (statusSwitchSenderPhase2 == LOW){
      Serial.println("Sende Funksignal 1111");
      sender.send(1111, 24); // Der 433mhz Sender versendet die Dezimalzahl „1234“
      statusSwitchSenderPhase2 = HIGH;
      delay(1000);  // Eine Sekunde Pause, danach startet der Sketch von vorne
  }
  
  statusPhase1 = digitalRead(pinSwitch); 
  if (statusPhase1 == LOW){
    Serial.println("Phase 1 activ");
    delay(1000);
    statusPhase2 = digitalRead(pinSwitch); 
  }
  //check for swtich state and trigger Actions
  if (statusPhase2 == LOW) {
    actionFire = 1;
    statusPhase2 = HIGH;
  }
  
  if (actionFire == 1) {
    fireing();
    actionFire = 0;
  }
  delay(1);

}

void fireing() {
  Serial.println("fire!");
  digitalWrite(pinFire1, HIGH);
  delay(1000);
  digitalWrite(pinFire2, HIGH);
  delay(1000);
  digitalWrite(pinFire1, LOW);
  digitalWrite(pinFire3, HIGH);
  delay(1000);
  digitalWrite(pinFire2, LOW);
  digitalWrite(pinFire4, HIGH);
  delay(1000);
  digitalWrite(pinFire3, LOW);
  delay(1000);
  digitalWrite(pinFire4, LOW);  
}
