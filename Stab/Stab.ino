
const int pinSwitch = 10; //Pin Reed
const int pinFire1 = 2; //Pin Rohr 1
const int pinFire2 = 3; //Pin Rohr 2
const int pinFire3 = 4; //Pin Rohr 3
const int pinFire4 = 5; //Pin Rohr 4

int actionFire = 0;
int statusPhase1 = HIGH;
int statusPhase2 = HIGH;

void setup()
{
  pinMode(pinFire1, OUTPUT);
  pinMode(pinFire2, OUTPUT);
  pinMode(pinFire3, OUTPUT);
  pinMode(pinFire4, OUTPUT); 
  pinMode(pinSwitch, INPUT_PULLUP);
  Serial.begin(9600);
}  
  
void loop()
{
  statusPhase1 = digitalRead(pinSwitch); 
  if (statusPhase1 == LOW){
    Serial.println("Phase 1 activ");
    delay(1500);
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
