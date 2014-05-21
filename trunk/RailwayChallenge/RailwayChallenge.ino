#include "InverterController.h"

//Program storage space variables
const String pinMap[] = {"R-RGC","R-RGD","R-GER1","R-GER2","R-GES","R-LTF","R-LTR","Z-BR-PLC","R-WHS","R-CSRT","R-FSH","R-CSSB","R-MIE","R-CEN","STF","STR","RM1","RH1","RM2","RH2","LEDG","LEDR"};
const byte pinCount = 22;//This is the length of pinMap
const byte pinOffset = 22;
const byte serialBufferLength = 128;
const byte thermistorOne = 0;
const byte thermistorTwo = 1;
const byte greenLED = 42;
const byte redLED = 43;
const int relayDelay = 200;
const unsigned long updateTime = 1777;

//input relays
const byte RT = 47;
const byte SBC = 48;
const byte BPS = 49;
const byte GH = 50;
const byte COMC = 51;
const byte MIC = 52;
//53 is connected and spare

//Dynamic memory variables
byte serialBuffer[serialBufferLength];
byte controlBuffer[serialBufferLength];
byte bufferIndex = 0;
byte controlIndex = 0;
bool connectionLost = true; //WIll stop train and request config
bool connectionTimeout = false; // for connection timeout
InverterController Inverters(36, 37, 38, 39, 40, 41);

//Thermistors
int thermistorOneReading;
int thermistorTwoReading;

//hall effect sensors, default 20,000 ~= 4m/s
unsigned long motorOnePeriod = 2857;
unsigned long motorTwoPeriod = 4000;
unsigned long motorOneLastTick = 0;
unsigned long motorTwoLastTick = 0;
//changed from float to byte as hz are only between 0 and 120 anyway
byte motorOneSpeed = 0;
byte motorTwoSpeed = 0;

//Toggle Switches 
bool genEnable = false;
bool regenEnable = false; 
bool forwardDrive = true;

void setup() {
  
  //Setup all output pins
  for(int n = 0; n < pinCount; n++)
  {
    pinMode(n+pinOffset, OUTPUT);
    digitalWrite(n+22, LOW);
  }
  
  //setup input pins
  for(int n = 47; n < 53; n++)
  {
    pinMode(n, INPUT_PULLUP);
  }
  
  //Set LEDs 
  digitalWrite(greenLED, HIGH);
  digitalWrite(redLED, HIGH);
  
  //Set interupts Due (21, 20) Mega 2, 3 both pins 20 and 21
  pinMode(21, INPUT_PULLUP);
  pinMode(20, INPUT_PULLUP);
  attachInterrupt(2, motorOneSpeedInterupt, FALLING);
  attachInterrupt(3, motorTwoSpeedInterupt, FALLING);
  
  //Start Serial
  Serial.begin(19200);
  Serial1.begin(19200);
  
  //Request Controler state
  Serial.println("Requesting Config from Controller");
  delay(500);
  Serial1.println("Q0");
  delay(500);
  if(Serial1.available() < 1)
  {
    Serial.println("Controler disconnected");
  }
  else
  {
    Serial.println("Controler Connected");
  }
  Serial.println(trainPrep());
  Serial.println("Debug Mode");
}
//byte serialIndex = Serial.readBytesUntil(termChar, serialBuffer, serialBufferLength); 
unsigned long lastUpdateTime = millis();
void loop() {
  if(millis() >= updateTime + lastUpdateTime)
  {
    lastUpdateTime = millis();
    
    if(connectionTimeout == true)
    {
      connectionLost = true;
    }
    else
    {
      connectionTimeout = true;
    }
    
    digitalWrite(greenLED, HIGH);

    //Thermistor Reading
    thermistorOneReading = analogRead(thermistorOne)/2.56;
    thermistorTwoReading = analogRead(thermistorTwo)/2.56;
    
    //Speed Reading
    // 1Hz speed is 142857us period
    motorOneSpeed = 1000000/(motorOnePeriod*7); // this is in Hz
    motorTwoSpeed = 1000000/(motorTwoPeriod*7); // this is in Hz

    serialUpdate();
    digitalWrite(greenLED, LOW);
  }
  
  //Check USB serial for debug commands
  while(Serial.available() > 0)
  {
    byte byteIn = Serial.read();
    if(byteIn == '\n' || byteIn == '\r')
    {
      //do some work
      debugCommand(bufferIndex);
      Serial.println(("Command Processed"));
      bufferIndex = 0;
    }
    else
    {
      //add to buffer
      serialBuffer[bufferIndex] = byteIn;
      bufferIndex++;
    }
  }
  
  //Check Controler serial link for commands
  while(Serial1.available() > 0)
  {
    byte byteIn = Serial1.read();
    if(byteIn == '\n' || byteIn == '\r')
    {
      //flash red LED to indicate serial coms
      //digitalWrite(redLED, !digitalRead(redLED));
      //Do some work
      controlerCommand(controlIndex);      
      controlIndex = 0;
    }
    else
    {
      controlBuffer[controlIndex] = byteIn;
      controlIndex++;
    }
  }
  
  if(connectionLost)
  {
    //Stop The Train
    Serial.println("Connection lost");
    Inverters.setSpeed(0);
    Serial1.println("Q0");
    connectionLost = false;
  }
}

void serialUpdate()
{
  Serial1.write('T');
  Serial1.write(thermistorOneReading);
  Serial1.write('Y');
  Serial1.write(thermistorTwoReading);
  Serial1.write('V');
  Serial1.write(motorOneSpeed);
  Serial1.write('B');
  Serial1.write(motorTwoSpeed);
  Serial1.write('R');
  byte states = 0;
    //note: MIC is the last input pin, RT is the first
  for(byte b = 0; b < MIC - RT; b++)
  {
    states |= (digitalRead(RT+b) << b);
  }
  Serial1.write(states);
  Serial1.write('\n');
}



//Comands from the hand held controler
void controlerCommand(byte serialIndex)
{
  connectionTimeout = false;
    connectionLost = false;
  //Do Some Work
  for(int s = 0; s < serialIndex; s++)
  {

    //checksum
    if(s + 2 < serialIndex && controlBuffer[s+1] == controlBuffer[s+2])
    {
      switch (controlBuffer[s])
      {
        case 'S': //Speed
          Inverters.setSpeed(controlBuffer[s+1]);
          s += 2;
          break;
        case 'D': //Direction
          if(commandState(controlBuffer[s+1]))
          {
            Inverters.driveForward();
          }
          else
          {
            Inverters.driveReverse();
          }
          s += 2;
          break;
        case 'G': //Genorator
          if(commandState(controlBuffer[s+1]))
          {
            startGenorator();
          }
          else
          {
            stopGenorator();
          }
          s += 2;
          break;
        case 'W': //Whistle
          Serial.println("Whistle");
          if(commandState(controlBuffer[s+1]))
          {
            digitalWrite(30, HIGH);
          }
          else
          {
            digitalWrite(30, LOW);
          }
          s += 2;
          break;
        case 'C': // Charge Regen
          if(commandState(controlBuffer[s+1]))
          {
            //charge regen
            digitalWrite(22, HIGH);
          }
          else
          {
            digitalWrite(22, LOW);
          }
          s += 2;
          break;
        case 'Z': //Drive Regen
          if(!commandState(controlBuffer[s+1]))
          {
            //Drive Regen
            digitalWrite(23, HIGH);
          }
          else
          {
            digitalWrite(23, LOW);
          
          }
          s += 2;
          break;
      }
    }
  }
}

bool commandState(byte state)
{
  //170 = 10101010 = true
  if(state == 170)
  {
    return true;
  }
  //85 = 01010101 = false
  else if(state == 85)
  {
    return false;
  }
}

//This is for debug commands
void debugCommand(byte serialIndex)
{
  char charCommand[serialIndex+1];
  for(int n = 0; n < serialIndex; n ++)
  {
    charCommand[n] = serialBuffer[n];
  }
  charCommand[serialIndex] = '\0';
  String command = String(charCommand);
  if(command == "list")
  {
    for(byte n = 0; n < pinCount; n++)
    {
      //Serial.println("Printing");
      Serial.print(pinMap[n]);
      Serial.print((" (Pin "));
      Serial.print(n+pinOffset);
      Serial.print((") State = "));
      Serial.println(digitalRead(pinOffset+n));
    }
  }
  else if(command == "speed1")
  {
    Inverters.setSpeed(1);
  }
  else if(command == "speed2")
  {
    Inverters.setSpeed(2);
  }
  else if(command == "speed3")
  {
    Inverters.setSpeed(3);
  }
  else if(command == "speed4")
  {
    Inverters.setSpeed(4);
  }
  else if(command == "forward")
  {
    Inverters.driveForward();
  }
  else if(command == "coast")
  {
    Inverters.coast();
  }
  else if(command == "startgen")
  {
    if(startGenorator())
      Serial.println("Genorator Online");
    else
      Serial.println("Genorator Failed To Start");
  }
  else if(command == "stopgen")
  {
    stopGenorator();
  }
  else if(command == "inputs")
  {
    Serial.print("RT: ");
    Serial.println(!digitalRead(RT));
    Serial.print("SBC: ");
    Serial.println(!digitalRead(SBC));
    Serial.print("BPS: ");
    Serial.println(!digitalRead(BPS));
    Serial.print("GH: ");
    Serial.println(!digitalRead(GH));
    Serial.print("COMC: ");
    Serial.println(!digitalRead(COMC));
    Serial.print("MIC: ");
    Serial.println(!digitalRead(MIC));
  }
  else if(command == "flick")
  {
    flick();
  }
  else if(command == "sensors")
  {
    Serial.print(("Thermistor One: "));
    Serial.println(thermistorOneReading);
    Serial.print(("Thermistor Two: "));
    Serial.println(thermistorTwoReading);
    Serial.print("Motor One Speed: ");
    Serial.println(motorOneSpeed);
    Serial.print("Motor Two Speed: ");
    Serial.println(motorTwoSpeed);
  }
  else
  {
    int pin = command.toInt();
    if(pin == 0)
    {
      pin = getPinNumber(command);
    }
    if (pin != -1)
    {
      Serial.print(("Toggeling pin "));
      Serial.println(pin);
      togglePin(pin);
    }
    else
    {
      Serial.print(("No such pin name: "));
      Serial.println(command);
    }
  }
}

String printPinState(byte index)
{
  Serial.print(pinMap[index]);
  Serial.print((" (Pin "));
  Serial.print(index+pinOffset);
  Serial.print((") State = "));
  Serial.println(digitalRead(pinOffset+index));
}

void togglePin(byte pinNumber)
{
  digitalWrite(pinNumber, !digitalRead(pinNumber));
}


void motorOneSpeedInterupt()
{
  motorOnePeriod = micros() - motorOneLastTick;
  motorOneLastTick = micros();
}

void motorTwoSpeedInterupt()
{
  motorTwoPeriod = micros() - motorTwoLastTick;
  motorTwoLastTick = micros();
}

String trainPrep()
{
  Serial.println("Starting Train Prep");
  
  String returnString = "FAIL: ";
  
  //Check inital relay states note that ALL inputs are inverse
  //All relays should be OPEN
  if(!(digitalRead(RT) && digitalRead(SBC) && digitalRead(BPS) && digitalRead(GH) && digitalRead(COMC) && digitalRead(MIC)))
  {
    returnString += "INITAL RELAY STATES WRONG, ";
  }
  //Next roundtrain tests
  digitalWrite(31, HIGH); //RTC RELAY
  digitalWrite(32, HIGH); //Fire relay
  delay(relayDelay);
  if(digitalRead(RT))// if RT not on
  {
    returnString += "RTC DID NOT COME UP, ";
  }
  digitalWrite(31, LOW);
  delay(relayDelay);
  if(!digitalRead(RT))
  {
    returnString += "RTC UP AFTER RTC OPEN, ";
  }
  digitalWrite(31, HIGH);
  digitalWrite(32, LOW);
  delay(relayDelay);
  if(!digitalRead(RT))
  {
    returnString += "RTC UP AFTER FIRE OPEN, ";
  }
  digitalWrite(31, LOW);
  digitalWrite(33, HIGH);
  delay(relayDelay);
  if(digitalRead(SBC))
  {
    returnString += "SBC DID NOT COME UP, ";
  }
  //Perform brake test
  digitalWrite(31, HIGH); // Enable RTC
  digitalWrite(32, HIGH); // Fire Systems Healthy
  if(forwardDrive)
    digitalWrite(27, HIGH);
  else
    digitalWrite(28, HIGH);
    
  if(genEnable)
  {
    if(!startGenorator())
    {
      returnString += "GEN WILL NOT START, ";
    }
  }   
  
  digitalWrite(29, HIGH); // Release Brakes
  delay(3000); // 3 seconds for brakes to disengage

  if(digitalRead(BPS))
  {
    returnString += "BRAKES WILL NOT RELEASE, ";
  }
  digitalWrite(29, LOW);
  
  if(returnString == "FAIL: ")
  {
    returnString = "ALL TESTS PASSED";
    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, HIGH);
  }
  else
  {
    digitalWrite(redLED, HIGH);
    digitalWrite(greenLED, LOW); 
  }
  
  return returnString;
}

void processSerialCommand(byte index)
{
  byte bytes[2];
  for(int n = 0; n <= index; n += 2)
  {
    bytes[0] = controlBuffer[n];
    bytes[1] = controlBuffer[n+1];
    if(bytes[0] == 'R')
    {
      if(bytes[1] == 1)
      {
        regenEnable = true;
      }
      else
      {
        regenEnable = false;
      }
    }
    else if(bytes[0] == 'G')
    {
      if(bytes[1] == 1)
      {
        genEnable = true;
      }
      else
      {
        genEnable = false;
      }
    }
    else if(bytes[0] == 'F')
    {
      if(bytes[1] == 1)
        {
          genEnable = true;
        }
        else
        {
          genEnable = false;
        }
    }
    else if (bytes[0] == 'P')
    {
      Inverters.setSpeed(bytes[1]);
    }
  }
}

bool startGenorator()
{
  unsigned long startTime = millis();
  digitalWrite(24, HIGH);
  delay(relayDelay);
  digitalWrite(25, HIGH);
  delay(relayDelay);
  digitalWrite(26, HIGH);
  while(digitalRead(GH))
  {
    delay(100);
    if(startTime + 4000 < millis())
    {
      digitalWrite(24, LOW);
      digitalWrite(25, LOW);
      digitalWrite(26, LOW);
      return false;
    }
  }
  digitalWrite(26, LOW);
  return true;
}

void stopGenorator()
{
  digitalWrite(24, LOW);
  digitalWrite(25, LOW);
}

void flick()
{
  for(int n = pinOffset; n < pinCount+pinOffset; n++)
  {
    togglePin(n);
    delay(relayDelay);
    togglePin(n);
    delay(relayDelay);
  }
}

int getPinNumber(String pinName)
{
  for(int n = 0; n < pinCount; n++)
  {
    if(pinMap[n] == (pinName))
    {
      return n + pinOffset;
    }
  }
  return -1;
}
  
