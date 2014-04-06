#include <LiquidCrystal.h>

//Setup LCD
LiquidCrystal lcd(22,23,24,25,26,27,28,29,30,31,32); 

//Input array
String inputs[] PROGMEM = {"F/R","RegenEnable","GenToggle","Faster","Slower","RegenDrive","RegenCharge","Whistle"};
byte inputPinOffset = 38;
byte inputCount = 8;

//Serial Buffer
byte controlBuffer[128];
byte controlIndex = 0;

void setup() {
  
  //Start up serial link to train
  Serial1.begin(9600);
  
  //intialse LCD
  lcd.begin(20,4);
  lcd.print("-RC Train Controler-");
  lcd.setCursor(0,1);
  lcd.print("Software version A1");
  lcd.setCursor(0,2);
  lcd.print("Prepairing pin setup");
  lcd.setCursor(0,3);
  lcd.print("PIN:");
  
  //setup pins NOTE: some of these may have to be moved to interupts
  for(int n = inputPinOffset; n < inputCount; n++)
  {
    pinMode(n, INPUT_PULLUP);
    lcd.setCursor(5,3);
    lcd.print(n);
    delay(500);
  }
}

void loop() {
  
  //Check for serial data from train
  while(Serial1.available() > 0)
  {
    byte byteIn = Serial1.read();
    if(byteIn == '\n')
    {
      char charCommand[controlIndex+1];
      for(int n = 0; n < controlIndex; n ++)
      {
        charCommand[n] = controlBuffer[n];
      }
      charCommand[controlIndex] = '\0';
      String command = String(charCommand);
      lcd.clear();
      lcd.print(command);
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
