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

//persistant states
bool genState = false;
bool directionState = false;
bool regenChargeState = false;
bool regenDriveState = false;
bool whistleState = false;
bool relayStates[6] = {false, false, false, false, false, false};

//Variables
byte temp1 = 0;
byte temp2 = 0;
float spd1 = 0;
float spd2 = 0;
byte currentSpeed = 0;
byte currentScreen = 0; //0 is main screen
bool speedHold = false;

byte con = 64;

const byte serialTrue = 170;
const byte serialFalse = 85;//85
const byte numberOfScreens = 4;
const unsigned long screenRefreshTime = 1000;
const byte fasterPin = 33;
const byte slowerPin = 34;

bool updateScreen[numberOfScreens];

byte smiley[8] = {
  B00000,
  B10001,
  B00000,
  B00000,
  B10001,
  B01110,
  B00000,
};

byte roundelLeft[8] = {
	B00000,
	B00011,
	B00110,
	B01100,
	B11111,
	B01100,
	B00110,
	B00011
};

byte roundelRight[8] = {
	B00000,
	B11000,
	B01100,
	B00110,
	B11111,
	B00110,
	B01100,
	B11000
};

void setup() {
  
  pinMode(2, OUTPUT);
  pinMode(11, OUTPUT);
  analogWrite(2, con);
  
  //Start up serial link to train
  Serial.begin(19200);
  Serial1.begin(19200);
  
  Serial.println(true);
  
  //intialse LCD
  lcd.begin(20,4);
  lcd.createChar(0, smiley);
  lcd.createChar(1, roundelLeft);
  lcd.createChar(2, roundelRight);
  lcd.home();
  lcd.print("-RC Train Controler-");
  lcd.setCursor(0,1);
  lcd.print("Connecting");
  unsigned int dotCount = 0;
  while(Serial.available() < 1)
  {
    if(dotCount == 60000)
    {
      lcd.print(".");
      dotCount = 0;
    }
    else
    {
      dotCount++;
    }
  }
  lcd.setCursor(0,2);
  for(int p = 0; p < 40; p++)
  {
    if(p % 2 == 0)
      lcd.write(byte(1));
    else
      lcd.write(byte(2));
  }
  
  //setup pins NOTE: some of these may have to be moved to interupts
  for(int n = inputPinOffset; n < inputCount+inputPinOffset; n++)
  {
    pinMode(n, INPUT_PULLUP);
  }
}

byte serialBoolConverter(bool toConvert)
{
  if(toConvert)
  {
    return serialTrue;
  }
  else
  {
    return serialFalse;
  }
}
unsigned long lastRefreshTime = millis();
void loop() {
  //analogWrite(11, 250);
  
  bool curDir = !digitalRead(38);
  if(curDir != directionState)
  {
    directionState = curDir;
    sendSerialCommand('D', serialBoolConverter(directionState));
  }
  
  bool curGen = !digitalRead(40);
  if(curGen != genState)
  {
    genState = curGen;
    sendSerialCommand('G', serialBoolConverter(genState));
  }
  
  bool curRegenDrive = !digitalRead(40);
  if(curRegenDrive != regenDriveState)
  {
    regenDriveState = curRegenDrive;
    sendSerialCommand('Z', serialBoolConverter(regenDriveState));
  }
  
  bool curRegenChargeState = !digitalRead(40);
  if(curRegenChargeState != regenChargeState)
  {
    regenChargeState = curRegenChargeState;
    sendSerialCommand('C', serialBoolConverter(regenChargeState));
  }
  
  bool curWhistleState = !digitalRead(40);
  if(curWhistleState != whistleState)
  {
    whistleState = curWhistleState;
    sendSerialCommand('W', serialBoolConverter(whistleState));
  }
  

  if(speedHold == false && (!digitalRead(fasterPin) || !digitalRead(slowerPin)))
  {
    speedHold = true;
    if(!digitalRead(fasterPin))
    {
      if(currentSpeed < 4)
      {
        currentSpeed++;
      }
      sendSerialCommand('S', currentSpeed);
    }
    else if(!digitalRead(slowerPin))
    {
      if(currentSpeed > 0)
      {
        currentSpeed--;
      }
      sendSerialCommand('S', currentSpeed);
    }
  }
  else if(digitalRead(slowerPin) && digitalRead(fasterPin))
  {
    speedHold = false;
  }
  
  //Check for serial data from train
  while(Serial.available() > 0)
  {
    byte byteIn = Serial.read();
    if(byteIn == '\n')
    {
      controlerCommand(controlIndex);
      controlIndex = 0;
    }
    else
    {
      controlBuffer[controlIndex] = byteIn;
      controlIndex++;
    }
  }
  
  if(millis() >= screenRefreshTime + lastRefreshTime)
  {
    lastRefreshTime = millis();
    sendSerialCommand('Q', 0);
    updateLCD();
    Serial.println("UPDATE LCD");
  }
}

void controlerCommand(byte serialIndex)
{
  //Do Some Work
  if( serialIndex < 2)
  {
    return;
  }
  
  for(int s = 0; s < serialIndex; s++)
  {
    //checksum
    switch (controlBuffer[s])
    {
      case 'Q': //Start
        //The Gen and Dir states must be sent as they are toggle switches
        Serial.println("Start");
        Serial1.write('D');
        Serial1.write(serialBoolConverter(directionState));
        Serial1.write(serialBoolConverter(directionState));
        Serial1.write('G');
        Serial1.write(serialBoolConverter(genState));
        Serial1.write(serialBoolConverter(genState));
        Serial1.write('\n');
        s += 1;
        break;
      case 'T': //Therm1
        Serial.println("Therm1");
        temp1 = controlBuffer[s+1];
        updateScreen[3] = true;
        s += 1;
        break;
      case 'Y': //Therm2
        Serial.println("THerm2");
        temp2 = controlBuffer[s+1];
        updateScreen[0] = true;
        updateScreen[2] = true;
        s += 1;
        break;
      case 'V': //spd1
        Serial.println("spd1");
        //Convert from Hz to KPH
        updateScreen[0] = true;
        updateScreen[1] = true;
        s += 1;
        break;
      case 'B': //spd2
        Serial.println("spd2");
        s += 1;
        break;
      case 'R': //Relay States
        
        byte states = controlBuffer[s+1];
        Serial.print("RelayStates ");
        for(int k = 0; k < 6; k++)
        {
          relayStates[k] = (states >> k) & B00000001;
          Serial.print(relayStates[k], BIN);
        }
        Serial.print("\n");
        
        updateScreen[2] = true;
        s += 1;
        break;
    }    
  }
}

void sendSerialCommand(char type, byte value)
{
  Serial1.write(type);
  Serial1.write(value);
  Serial1.write(value);
  Serial1.write('\n');
}

//LCD Functions
void updateLCD()
{
  if(!updateScreen[currentScreen])
  {
    return;
  }
  else
  {
    updateScreen[currentScreen] = false;
  }
  
  lcd.clear();
  lcd.print(currentScreen);
  lcd.print("/");
  lcd.print(numberOfScreens);
  
  switch (currentScreen) 
  {
    case 0: //Main screen
      lcd.print(" Main Screen");
      lcd.setCursor(0,1);
      lcd.print("Target Speed: ");
      lcd.print(currentSpeed);
      lcd.print("kph");
      lcd.setCursor(0,2);
      lcd.print("Current Speed: ");
      lcd.print(spd1, 2);
      lcd.print("kph");
      lcd.setCursor(0,3);
      lcd.print("Resistor Temp: ");
      lcd.print(temp2, 3);
      lcd.print("C");
      break;
    case 1://Regen screen
      lcd.print(" Regen Screen");
      lcd.setCursor(0,1);
      lcd.print("Regen State: ");
      if(regenChargeState)
        lcd.print("CHARGE");
      else if(regenDriveState)
        lcd.print("DRIVE");
      else
        lcd.print("IDLE");
      lcd.setCursor(0,2);
      lcd.print("Current Speed: ");
      lcd.print(spd1, 2);
      lcd.print("kph");
      break;
    case 2://Relay states
      lcd.print(" Relay States");
      lcd.setCursor(0,1);
      lcd.print("Rnd Trn:");
      lcd.print(relayStates[0]);
      lcd.print("|Brake PS:");
      lcd.print(relayStates[3]);
      lcd.setCursor(0,2);
      lcd.print("    SBC:");
      lcd.print(relayStates[1]);
      lcd.print("|    COMC:");
      lcd.print(relayStates[4]);
      lcd.setCursor(0,3);
      lcd.print("  Gen H:");
      lcd.print(relayStates[2]);
      lcd.print("|     MIC:");
      lcd.print(relayStates[5]);
      break;
  }
}
