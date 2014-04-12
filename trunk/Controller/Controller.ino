#include <LiquidCrystal.h>

//Setup LCD
LiquidCrystal lcd(22,23,24,25,26,27,28,29,30,31,32); 

//Input array
String inputs[]  = {"F/R","RegenEnable","GenToggle","Faster","Slower","RegenDrive","RegenCharge","Whistle"};
byte inputPinOffset = 38;
byte inputCount = 8;

//Serial Buffer
byte controlBuffer[128];
byte controlIndex = 0;

byte con = 64;

byte smiley[8] = {
  B00000,
  B10001,
  B00000,
  B00000,
  B10001,
  B01110,
  B00000,
};

void setup() {
  
  pinMode(2, OUTPUT);
  pinMode(11, OUTPUT);
  analogWrite(2, con);
  
  //Start up serial link to train
  Serial.begin(9600);
  
  //intialse LCD
  lcd.begin(20,4);
  lcd.createChar(0, smiley);
  lcd.home();
  lcd.print("-RC Train Controler-");
  lcd.setCursor(0,1);
  lcd.print("Software version A1");
  lcd.setCursor(0,2);
  lcd.print("Preparing pin setup");
  lcd.setCursor(0,3);
  lcd.print("PIN:");
  
  //setup pins NOTE: some of these may have to be moved to interupts
  for(int n = inputPinOffset; n < inputCount+inputPinOffset; n++)
  {
    pinMode(n, INPUT_PULLUP);
    lcd.setCursor(5,3);
    lcd.print(n);
  }
}

void loop() {
  //analogWrite(11, 250);
  //Check for serial data from train
  while(Serial.available() > 0)
  {
    byte byteIn = Serial.read();
    if(byteIn == '\n')
    {
      char charCommand[controlIndex+1];
      for(int n = 0; n < controlIndex; n ++)
      {
        charCommand[n] = controlBuffer[n];
      }
      charCommand[controlIndex] = '\0';
      String command = String(charCommand);
      if (command == "u")
      {
        con += 10;
        analogWrite(2, con);
      }
      else if (command == "d")
      {
        con -= 10;
        analogWrite(2, con);
      }
      else
      {
      lcd.clear();
     
      lcd.print(command);
      lcd.write(byte(0));
      }
      controlIndex = 0;
    }
    else
    {
      controlBuffer[controlIndex] = byteIn;
      controlIndex++;
    }
  }
  for(int n = inputPinOffset; n < inputCount; n++)
  {
    if(digitalRead(n) == HIGH)
    {
      lcd.clear();
      lcd.print(inputs[n-inputPinOffset]);
      lcd.print(" is HIGH!");
    }
  }
}
