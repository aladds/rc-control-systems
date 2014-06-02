#include <LiquidCrystal.h>

//Setup LCD
LiquidCrystal lcd(22,23,24,25,26,27,28,29,30,31,32); 

//Input array
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
bool relayStates[6] = {false, false, false, false, false, false};

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

byte con = 64;

const byte serialTrue = 170;
const byte serialFalse = 85;//85
const byte numberOfScreens = 4;
const unsigned long screenRefreshTime = 250;
const float gearRatio = 0.23;

bool updateScreen[numberOfScreens] = {true, false, false, false};

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
unsigned long lastRefreshTime = millis();
const unsigned long debounce = 20;
unsigned long debounceTime = millis();
void loop() {
  //analogWrite(11, 250);
  
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
    if(currentScreen < 3 && !digitalRead(screenForward))
    {
      Serial.println("Forward Screen");
      currentScreen++;
      updateScreen[currentScreen] = true;
    }
  }
  
  bool curPrevScreenState = !digitalRead(screenBack);
  if(curPrevScreenState != prevScreenState && debounceTime + debounce < millis())
  {
    debounceTime = millis();
    prevScreenState = curPrevScreenState;
    if(currentScreen > 0 && !digitalRead(screenBack))
    {
      Serial.println("Back screen");
      currentScreen--;
      updateScreen[currentScreen] = true;
    }
  }

  if(speedHold == false && (!digitalRead(fast) || !digitalRead(slow)))
  {
    speedHold = true;
    if(!digitalRead(fast) && !digitalRead(slow))
    {
      //Stop regen mode
      currentScreen = 0;
      updateScreen[0] = true;
      currentSpeed = 0;
    }
    else if(!digitalRead(fast))
    {
      if(currentSpeed < 4)
      {
        currentSpeed++;
      }
      sendSerialCommand('S', currentSpeed);
    }
    else if(!digitalRead(slow))
    {
      if(currentSpeed > 0)
      {
        currentSpeed--;
      }
      sendSerialCommand('S', currentSpeed);
    }
    
  }
  else if(digitalRead(slow) && digitalRead(fast))
  {
    speedHold = false;
  }
  
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
        spd1 = controlBuffer[s+1] * gearRatio;
        updateScreen[0] = true;
        updateScreen[1] = true;
        s += 1;
        break;
      case 'B': //spd2
        Serial.println("spd2");
        spd2 = controlBuffer[s+1] * gearRatio;
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
      lcd.print("Target Speed: ");
      lcd.print(currentSpeed);
      lcd.print("kph");
      lcd.setCursor(0,2);
      lcd.print("Drive Speed: ");
      lcd.print(spd1, 0);
      lcd.print("kph");
      lcd.setCursor(0,3);
      lcd.print("Resistor Temp: ");
      if(temp1 > temp2)
      {
        lcd.print(temp1, 3);
      }
      else
      {
        lcd.print(temp1, 3);//SHOULD BE TEMP2!
      }
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
      lcd.print(spd1, 0);
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
    case 3://Warning Screen
      lcd.print(" Warning Messages");
      lcd.setCursor(0,1);
      lcd.print(line1Warning);
      break;
  }
}
