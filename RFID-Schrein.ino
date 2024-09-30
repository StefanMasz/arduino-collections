#include <SPI.h>
#include <MFRC522.h>
 
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;
 
void setup() 
{
  Serial.begin(9600);   // Initiate a serial communication
  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
  Serial.println("Approximate your card to the reader...");
  Serial.println();

}
void loop() 
{
  //default key for fresh card
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  //desired EE key:
  /*key.keyByte[0] = 0x36;
  key.keyByte[1] = 0x5A;
  key.keyByte[2] = 0xC4;
  key.keyByte[3] = 0x22;
  key.keyByte[4] = 0xFE;
  key.keyByte[5] = 0x35;*/

  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }

  String uuid = "";
  uuid = readUUID();

  if (uuid.substring(1) == "43 0D FE 27") //change here the UID of the card/cards that you want to give access
  {
    Serial.println("Authorized access - correct card");
  }
 
 else   {
    Serial.println("Access denied - unknown card");
    delay(2000);
    return;
  }

  byte buffer[18];
  byte block;
  byte len;

  block = 16;
  len = 18;
 
  decryptBlock(block, key);

  int counter;
  counter = readCounterValue(block, buffer, len);
  Serial.print("Counter before:"); 
  Serial.println(counter);

  //Reset to start Values
  //buffer[0] = 0;
  //buffer[1] = 0x02;
  buffer[0]++;

  writeToBlock(block, buffer);

  counter = readCounterValue(block, buffer, len);
  Serial.print("Counter after:"); 
  Serial.println(counter);

  Serial.println(F("\n**End**\n"));
 
  delay(1000);
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
} 

int readCounterValue(byte block, byte *buffer, byte len) 
{
  status = mfrc522.MIFARE_Read(block, buffer, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
 
  dump_byte_array(buffer, len);
  return (int)buffer[0];
}

void decryptBlock(byte block, MFRC522::MIFARE_Key key)
{
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
}

String readUUID()
{
    //Show UID on serial monitor
  Serial.print("UID tag :");
  String content= "";
  byte letter;

  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
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

void writeToBlock(byte block, byte buff[]) 
{
    // Write block
    status = mfrc522.MIFARE_Write(block, buff, 16);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Write() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }
    else Serial.println(F("MIFARE_Write() success!"));
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
