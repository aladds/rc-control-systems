#include "InverterController.h"

//Program storage space variables
const String pinMap[] PROGMEM = {"R-RGC","R-RGD","R-GER1","R-GER2","R-GES","R-LTF","R-LTR","Z-BR-PLC","R-WHS","R-CSRT","R-FSH","R-CSSB","R-MIE","R-CEN","STF","STR","RM1","RH1","RM2","RH2","LEDG","LEDR"};
const byte pinCount = 22;//This is the length of pinMap
const byte pinOffset = 22;
const byte serialBufferLength = 128;
const byte thermistorOne = 0;
const byte thermistorTwo = 1;
const byte greenLED = 42;
const byte redLED = 43;

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
InverterController Inverters(36, 37, 38, 39, 40, 41);

//Thermistors
int thermistorOneReading;
int thermistorTwoReading;

//hall effect sensors, default 20,000 ~= 4m/s
unsigned long motorOnePeriod = 2857;
unsigned long motorTwoPeriod = 20000;
unsigned long motorOneLastTick = 0;
unsigned long motorTwoLastTick = 0;
float motorOneSpeed = 0;
float motorTwoSpeed = 0;

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
  Serial.begin(9600);
  Serial1.begin(9600);
  
  //Request Controler state
  Serial.println("Requesting Config from Controller");
  delay(500);
  Serial1.println('S0');//Start
  delay(500);
  if(Serial1.available() < 1)
  {
    Serial.println("Controler disconnected");
    trainPrep();
  }
  else
  {
    while(Serial1.available() > 0)
    {
      byte byteIn = Serial1.read();
      if(byteIn == '\n')
      {
        //Do some work
        processSerialCommand(controlIndex);      
        controlIndex = 0;
      }
      else
      {
        controlBuffer[controlIndex] = byteIn;
        controlIndex++;
      }
    }
  }
  Serial.println("Debug Mode");
}
//byte serialIndex = Serial.readBytesUntil(termChar, serialBuffer, serialBufferLength); 

void loop() {
  
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
    if(byteIn == '\n')
    {
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
  
  //Thermistor Reading
  thermistorOneReading = analogRead(thermistorOne);
  thermistorTwoReading = analogRead(thermistorTwo);
  
  //Speed Reading
  // 1Hz speed is 142857us period
  motorOneSpeed = 1000000/(motorOnePeriod*7); // this is in Hz
  motorTwoSpeed = 1000000/(motorTwoPeriod*7); // this is in Hz
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

//Comands from the hand held controler
void controlerCommand(byte serialIndex)
{
  //Do Some Work
}

//This is for debug commands
void debugCommand(byte serialIndex)
{
  char charCommand[serialIndex+1];
  for(int n = 0; n <= serialIndex; n ++)
  {
    charCommand[n] = serialBuffer[n];
  }
  //charCommand[serialIndex] = '\0';
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
  //Check inital relay states note that ALL inputs are inverse
  //All relays should be OPEN
  if(digitalRead(RT) && digitalRead(SBC) && digitalRead(BPS) && digitalRead(GH) && digitalRead(COMC) && digitalRead(MIC))
  {
    Serial.println("Everything is good");
    //Everything is good
  }
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
  }
}
