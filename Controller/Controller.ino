/*
**This class provides control and comunication for the control handle. It also 
**provides LCD output and calculates speed and tempreate from recived bytes over serial
*/

#include <LiquidCrystal.h>

//Setup LCD
LiquidCrystal lcd(22,23,24,25,26,27,28,29,30,31,32); 

//Constants
const byte slow = 33;
const byte fast = 34;
const byte gen = 40;
const byte fb = 41;
const byte charge = 42;
const byte drive = 43;
const byte whistle = 45;
const byte screenBack = 46;
const byte screenForward = 47;

//Serial Buffer
byte controlBuffer[128];
byte controlIndex = 0;

//persistant states
bool genState = false;
bool directionState = false;
bool regenChargeState = false;
bool regenDriveState = false;
bool whistleState = false;
bool nextScreenState = false;
bool prevScreenState = false;
bool fasterState = false;
bool slowerState = false;
bool relayStates[7] = {true, true, true, true, true, true, true};
bool ackError = false;
bool invTrip = false;

//Variables
byte temp1 = 0;
byte temp2 = 0;
float spd1 = 0;
float spd2 = 0;
byte currentSpeed = 0;
byte currentScreen = 3; //0 is main screen
bool speedHold = false;
String line1Warning = "Connecting...";
bool serialLinkUp = false;
char serialTest = 'A';

//Contrast setting
const byte con = 64;

//other constants
const byte serialTrue = 170;
const byte serialFalse = 85;//85
const byte numberOfScreens = 4;
const unsigned long screenRefreshTime = 250;
const float gearRatio = 0.1715;

//this can be used to make screen refresh more effecient. It is currently disabled.
bool updateScreen[numberOfScreens] = {true, false, false, false};

//Custom LCD chars
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
  
  //set up PWM pin for LCD contrast
  pinMode(2, OUTPUT);
  analogWrite(2, con);
  
  //Optinal Buzer
  pinMode(11, OUTPUT);
  
  //Start up serial link to train
  Serial.begin(19200);
  Serial1.begin(19200);
  
  //Set up input pins
  pinMode(slow, INPUT_PULLUP);
  pinMode(fast, INPUT_PULLUP);
  pinMode(gen, INPUT_PULLUP);
  pinMode(fb, INPUT_PULLUP);
  pinMode(charge, INPUT_PULLUP);
  pinMode(drive, INPUT_PULLUP);
  pinMode(whistle, INPUT_PULLUP);
  pinMode(screenBack, INPUT_PULLUP);
  pinMode(screenForward, INPUT_PULLUP);
  
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
  lcd.setCursor(0,2);
  for(int p = 0; p < 40; p++)
  {
    if(p % 2 == 0)
      lcd.write(byte(1));
    else
      lcd.write(byte(2));
  }
  delay(200);//to show of roundels
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

//Variables for debounce timing
unsigned long lastRefreshTime = millis();
const unsigned long debounce = 20;
unsigned long debounceTime = millis();
unsigned long slowerDebounce = millis();
unsigned long fasterDebounce = millis();
unsigned long lastAlarm = millis();
unsigned long alarmPeriod = 5;

void loop() {
  if(relayStates[0] == true)
  {
    analogWrite(11, 800);
    analogWrite(11, 500);
  }
  else
  {
    analogWrite(11, 0);
  }
  
  bool curDir = !digitalRead(fb);
  if(curDir != directionState)
  {
    //toggle
    delay(20);
    directionState = curDir;
    sendSerialCommand('D', serialBoolConverter(directionState));
  }
  
  bool curGen = !digitalRead(gen);
  if(curGen != genState)
  {
    //this is a toggle switch
    delay(20);
    Serial.println(genState);
    genState = curGen;
    sendSerialCommand('G', serialBoolConverter(genState));
  }
  
  //REGEN STUFF -------------------------------------------------------
  bool curRegenDrive = !digitalRead(drive);
  if(curRegenDrive != regenDriveState)
  {
    regenDriveState = curRegenDrive;
    sendSerialCommand('Z', serialBoolConverter(regenDriveState));
    updateScreen[1] = true;
    currentScreen = 1;
  }
  
  bool curRegenChargeState = !digitalRead(charge);
  if(curRegenChargeState != regenChargeState)
  {
    regenChargeState = curRegenChargeState;
    sendSerialCommand('C', serialBoolConverter(regenChargeState));
    updateScreen[1] = true;
    currentScreen = 1;
  }
  //REGEN END ---------------------------------------------------------
  
  bool curWhistleState = !digitalRead(whistle);
  if(curWhistleState != whistleState)
  {
    whistleState = curWhistleState;
    sendSerialCommand('W', serialBoolConverter(whistleState));
  }
  
  bool curNextScreenState = !digitalRead(screenForward);
  if(curNextScreenState != nextScreenState  && (debounceTime + debounce) < millis())
  {
    debounceTime = millis();
    nextScreenState = curNextScreenState;
    if(!digitalRead(screenForward))
    {
      ackError = true;
      if(currentScreen < 3)
      {
        currentScreen++;
      }
      else
      {
        currentScreen = 0;
      }
      updateScreen[currentScreen] = true;
    }

  }
  //This is now RESTROKE ---------------------------------------------------------
  bool curPrevScreenState = !digitalRead(screenBack);
  if(curPrevScreenState != prevScreenState && debounceTime + debounce < millis())
  {
    debounceTime = millis();
    prevScreenState = curPrevScreenState;
    if(!digitalRead(screenBack))
    {
      currentSpeed = 0;
      currentScreen = 0;
      sendSerialCommand('L', serialBoolConverter(true));
    }
  }
  //END RESTROKE ------------------------------------------------------------------
  
  //SPEED BUTTONS -----------------------------------------------------------------
  if(!(curRegenDrive || regenDriveState || curRegenChargeState || regenChargeState))
  {
    bool curFasterState = !digitalRead(fast);
    if(curFasterState != fasterState && (fasterDebounce + debounce) < millis())
    {
      fasterState = curFasterState;
      fasterDebounce = millis();
      if(curFasterState == true)
      {
        if(currentSpeed < 6)
          {
            currentSpeed++;
          }
          sendSerialCommand('S', currentSpeed);
      }
    }
    
    bool curSlowerState = !digitalRead(slow);
    if(curSlowerState != slowerState && (slowerDebounce + debounce) < millis())
    {
      slowerState = curSlowerState;
      slowerDebounce = millis();
      if(curSlowerState == true)
      {
        if(currentSpeed > 0)
          {
            currentSpeed--;
          }
          sendSerialCommand('S', currentSpeed);
      }
    }
  }
  //END SPEED BUTTONS -------------------------------------------------------------
  
  //Check for serial data from train
  while(Serial1.available() > 0)
  {
    byte byteIn = Serial1.read();
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
    sendSerialCommand('U', 0);
    sendSerialCommand('S', currentSpeed);
    updateLCD();
  }
}

bool activtyToggle = false;

void controlerCommand(byte serialIndex)
{
  //Do Some Work
  if( serialIndex < 2)
  {
    return;
  }
  
  lcd.setCursor(19,0);
  if(activtyToggle)
  {
    serialTest = 'A';
    
  }
  else
  {
    serialTest = 'B';
  }
  activtyToggle = !activtyToggle;
  
  for(int s = 0; s < serialIndex; s++)
  {
    //checksum
    switch (controlBuffer[s])
    { 
      case 'Q': //Start
        //The Gen and Dir states must be sent as they are toggle switches
        serialLinkUp = true;
        Serial.println("Start");
        Serial1.write('D');
        Serial1.write(serialBoolConverter(directionState));
        Serial1.write(serialBoolConverter(directionState));
        Serial1.write('G');
        Serial1.write(serialBoolConverter(genState));
        Serial1.write(serialBoolConverter(genState));
        Serial1.write('\n');
        currentScreen = 0;
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
        spd1 = ((controlBuffer[s+1] * 3.6) * gearRatio)/8;
        updateScreen[0] = true;
        updateScreen[1] = true;
        s += 1;
        break;
      case 'B': //spd2
        Serial.println("spd2");
        spd2 = ((controlBuffer[s+1] * 3.6) * gearRatio)/8;
        updateScreen[0] = true;
        updateScreen[1] = true;
        s += 1;
        break;
      case 'M':
        updateScreen[3] = true;
        currentScreen = 3;
        s+= 1;
        break;
      case 'S':
        currentSpeed = controlBuffer[s+1];
        s+=1;
        break;
      case 'R': //Relay States
        byte states = controlBuffer[s+1];
        for(int k = 6; k >= 0; k--)
        {
          relayStates[k] = (states >> k) & B00000001;
        }   
        if(relayStates[6] == true)
        {
          if(ackError == false)
          {
            line1Warning = "Inverter Trip!";
            //currentScreen =   3;
          }
        }
        else
        {
          ackError = false;
        }
        updateScreen[2] = true;
        s += 1;
        break;
      
    }    
  }
}

void sendSerialCommand(char type, byte value)
{
  if(serialLinkUp)
  {
    Serial1.write(type);
    Serial1.write(value);
    Serial1.write(value);
    Serial1.write('\n');
  }
}

//LCD Functions
void updateLCD()
{
  if(currentScreen > 3)
  {
    //currentScreen = 0;
  }
  
  if(!updateScreen[currentScreen])
  {
    //return;
  }
  else
  {
    updateScreen[currentScreen] = false;
  }
  
  lcd.clear();
  lcd.setCursor(19,0);
  lcd.print(serialTest);
  lcd.home();
  lcd.print(currentScreen+1);
  lcd.print("/");
  lcd.print(numberOfScreens);
  
  switch (currentScreen) 
  {
    case 0: //Main screen
      lcd.print(" Main Screen");
      lcd.setCursor(0,1);
      lcd.print("Speed Level: ");
      lcd.print(currentSpeed);
      //lcd.print("kph");
      lcd.setCursor(0,2);
      lcd.print("Drive Speed: ");
      lcd.print(spd1, 1);
      lcd.print("kph");//change to kph
      lcd.setCursor(0,3);
      lcd.print("Inverter State: ");
      if(relayStates[5] == true)
      {
        lcd.print("Off");
      }
      else if(relayStates[6] == true)
      {
        lcd.print("Trip");
      }
      else
      {
        lcd.print("Set");
      }
      break;
    case 1://Regen screen
      lcd.print(" Regen Screen");
      lcd.setCursor(0,1);
      lcd.print("Regen State: ");
      if(regenChargeState && regenDriveState)
        lcd.print("BOTH");
      else if(regenDriveState)
        lcd.print("DRIVE");
      else if(regenChargeState)
        lcd.print("CHARGE");
      else
        lcd.print("IDLE");
      lcd.setCursor(0,2);
      lcd.print("Current Speed: ");
      lcd.print(spd1, 1);
      lcd.print("kph");
      break;
    case 2://Relay states
      lcd.print(" Relay States");
      lcd.setCursor(0,1);
      lcd.print("Rnd Trn:");
      lcd.print(!relayStates[0]);
      lcd.print("|Brake PS:");
      lcd.print(!relayStates[2]);
      lcd.setCursor(0,2);
      lcd.print("    SBC:");
      lcd.print(!relayStates[1]);
      lcd.print("|    COMC:");
      lcd.print(!relayStates[4]);
      lcd.setCursor(0,3);
      lcd.print("  Gen H:");
      lcd.print(!relayStates[3]);
      lcd.print("|     MIC:");
      lcd.print(!relayStates[5]);
      break;
    case 3://Warning Screen
      lcd.print(" Warning Messages");
      lcd.setCursor(0,1);
      lcd.print(line1Warning);
      break;
  }
}
