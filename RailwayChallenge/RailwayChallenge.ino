#include "InverterController.h"

//Program storage space variables
const String pinMap[] PROGMEM = {"R-RGC","R-RGD","R-GER1","R-GER2","R-GES","R-LTF","R-LTR","0VA","Z-BR-PLC","R-WHS","R-CSRT","R-FSH","R-CSSB","R-MIE","R-CEN","0VB","24V","0VC","STF","STR","RM1","RH1","RM2","RH2"};
const byte pinCount PROGMEM = 24;//This is the length of pinMap
const byte pinOffset PROGMEM = 22;
const byte serialBufferLength PROGMEM = 128;
const byte thermistorOne PROGMEM = 0;
const byte thermistorTwo PROGMEM = 1;

//Dynamic memory variables
byte serialBuffer[serialBufferLength];
byte controlBuffer[serialBufferLength];
byte bufferIndex = 0;
byte controlIndex = 0;
InverterController Inverters(40, 41, 42, 43, 44, 45);
//Thermistors
int thermistorOneReading;
int thermistorTwoReading;
//Read relays, default 20,000 ~= 4m/s
unsigned long motorOnePeriod = 2857;
unsigned long motorTwoPeriod = 20000;
unsigned long motorOneLastTick = 0;
unsigned long motorTwoLastTick = 0;
float motorOneSpeed = 0;
float motorTwoSpeed = 0;

void setup() {
  
  //Setup all pins
  for(int n = 0; n < pinCount; n++)
  {
    pinMode(n+pinOffset, INPUT);
    pinMode(n+pinOffset, OUTPUT);
    digitalWrite(n+22, LOW);
  }
  
  //Set interupts Due (21, 20) Mega 2, 3 both pins 20 and 21
  attachInterrupt(21, motorOneSpeedInterupt, RISING);
  attachInterrupt(20, motorTwoSpeedInterupt, RISING);
  
  //Set up Inverters
  //inverterOne = new InverterController(40, 41, 42, 43);
  
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial.println(("Waiting for user responce to begin in DEBUG mode"));
  while(Serial.available() < 1)
  {
    delay(100);
  }
  while(Serial.available() > 0)
  {
    Serial.read();
  }
  Serial.println(("Thank you, Starting now..."));
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
  motorTwoSpeed = 1000000/(motorOnePeriod*7); // this is in Hz
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
