#include <InverterController.h>

//NORMAL MODE


//Program storage space     22     23      24        25       26     27      28       29        30       31      32      33        34     35     36    37    38    39    40    41    42     43     44   45
const String pinMap[] = {"R-RGC","R-RGD","R-GER1","R-GER2","R-GES","R-LTF","R-LTR","Z-BR-PLC","R-WHS","R-CSRT","R-FSH","R-CSSB","R-MIE","R-CEN","RH1","RM2","RH2","FWD","REV","RM1","LEDG","LEDR","RL","REN"};
const byte pinCount = 24;//This is the length of pinMap
const byte pinOffset = 22;// pins 22 - 43 and 45 pins used
const byte serialBufferLength = 128;
const byte thermistorOne = 0;
const byte thermistorTwo = 1;
const byte greenLED = 42;
const byte redLED = 43;
const byte lightsForward = 27;
const byte lightsBackward = 28;
const byte inverterPower = 34;
const byte numberOfMagnets = 1;
const int relayDelay = 200;
const unsigned long updateTime = 1777;//ms
const unsigned long speedUpdateTime = 250;
const unsigned long minimumPeriod = 1207729; //1kph
const unsigned long tripSetTime = 10000;

//input relays
const byte RT = 53;//  0
const byte SBC = 52;// 1
const byte BPS = 51;// 2
const byte GH = 50;//  3
const byte COMC = 49;//4
const byte MIC = 48;// 5
const byte INV = 47;// 6

//Common pins
const byte REN = 45;
const byte RRTC = 31;
const byte FIRE = 32;
const byte RSBC = 33;
const byte RCEN = 35;
const byte reGenCharge = 22;
const byte reGenDrive = 23;
const byte RMIE = 34;
const byte brakeRelease = 29;

//Dynamic memory variables
byte serialBuffer[serialBufferLength];
byte controlBuffer[serialBufferLength];
byte bufferIndex = 0;
byte controlIndex = 0;
bool connectionLost = true; //WIll stop train and request config
bool connectionTimeout = false; // for connection timeout
byte currentSpeed = 0;
bool regenEnabled = false;
bool tripSet = false;
unsigned long tripSetStartTime = millis();
unsigned long startBrakeTime = 0;
bool startBrakeBool = false;
bool printOnce = false;
int regenCounter = 0;
//Yes these pins are odd, I have a photo though
InverterController Inverters(44, 39, 40, 41, 36, 37, 38);

//Thermistors
const float voltRes = ((1024/5) * 3.8)/200;//(steps per volt x max voltage in) / range
byte thermistorOneReading;
byte thermistorTwoReading;

//hall effect sensors, default 20,000 ~= 4m/s
unsigned long motorOnePeriod = 0;
unsigned long motorTwoPeriod = 0;
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
    digitalWrite(n+pinOffset, LOW);
  }
  
  //setup input pins
  for(int n = 47; n < 54; n++)
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
  Serial1.write('Q');
  Serial1.write(0);
  Serial1.write('\n');
  delay(500);
  if(Serial1.available() < 1)
  {
    Serial.println("Controler disconnected");
    digitalWrite(FIRE, HIGH);
  }
  else
  {
    Serial.println("Controler Connected");
    //Enable SBC and round train as we have a controler anyway
    digitalWrite(RSBC, HIGH);
    digitalWrite(RRTC, HIGH);
    //We are probs not on fire 
    digitalWrite(FIRE, HIGH);
    //and some break resistors wouldnt go a miss
    //digitalWrite(REN, HIGH);
    //also i would like the compressor
    digitalWrite(RCEN, HIGH);
    
    digitalWrite(redLED, LOW);
  }
  //Serial.println(trainPrep());
  Serial.println("Debug Mode");
}
//byte serialIndex = Serial.readBytesUntil(termChar, serialBuffer, serialBufferLength); 
unsigned long lastUpdateTime = millis();
unsigned long lastSpeedUpdateTime = millis();
void loop() {
  // Break Rate Debug ---
  if(!digitalRead(BPS) && !startBrakeBool)
  {
     startBrakeBool = true;
     startBrakeTime = millis();
  }
  else if(!digitalRead(BPS) && digitalRead(SBC))
  {
    Serial.print("Stop time = ");
    Serial.println(millis() - startBrakeTime);
    printOnce = true;
  }
  else if(digitalRead(BPS))
  {
    startBrakeBool = false;
    printOnce = false;
  }
  // Brake Rate Debug End --- 
  
  
  //REGEN
  if(digitalRead(reGenDrive) && regenCounter < 0)
  {
    digitalWrite(reGenDrive, LOW);
  }
  
  if(millis() >= updateTime + lastUpdateTime)
  {
    lastUpdateTime = millis();
    
    if(connectionTimeout == true)
    {
      connectionLost = true;
      Serial.println("Connection Lost");
    }
    else
    {
      connectionTimeout = true;
      //Serial.println("Connected");
    }
    
    digitalWrite(greenLED, HIGH);

    //Thermistor Reading
    thermistorOneReading = round(analogRead(thermistorOne) / voltRes);
    thermistorTwoReading = round(analogRead(thermistorTwo) / voltRes);

    if(!connectionLost)
    {
      serialUpdate();
    }
    digitalWrite(greenLED, LOW);
  }
  //Speed Update
  if(millis() > speedUpdateTime + lastSpeedUpdateTime)
  {
    lastSpeedUpdateTime = millis();
    //Speed Reading
    // 1Hz speed is 142857us period
    if(micros() - motorOneLastTick > minimumPeriod) //slower than 1kph
    {
      motorOneSpeed = 0;
    }
    else
    {
      motorOneSpeed = round((1000000/(motorOnePeriod*numberOfMagnets))*8); // this is in Hz
    }
    if(micros() - motorTwoLastTick > minimumPeriod) //slower than 1kph
    {
      motorTwoSpeed = 0;
    }
    else
    {
      motorTwoSpeed = round((1000000/(motorTwoPeriod*numberOfMagnets))*8); // this is in Hz
    }
    Serial1.write('V');
    Serial1.write(motorOneSpeed);
    Serial1.write('B');
    Serial1.write(motorTwoSpeed);
    Serial1.write('\n');
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
  
  //Controled Service stop
  if(currentSpeed == 0 && regenEnable == false)
  {
    if(motorOneSpeed < 2.5)
    {
      //Start Hack -- Applies Brakes
      digitalWrite(39, LOW);
      digitalWrite(40, LOW);
      //End Hack --
      digitalWrite(brakeRelease, LOW);
    }
  }
  
  if(tripSet == true)
  {
    if(tripSetTime + tripSetStartTime > millis())
    {
      //DISABLE ALL REGEN
      digitalWrite(reGenCharge, LOW);
      digitalWrite(reGenDrive, LOW);
      //SET STOP SPEED
      currentSpeed = 0;
      Inverters.setSpeed(0);
      //RESTROKE INVERTERS
      digitalWrite(RMIE, LOW);
    }
    else
    {  
      tripSet = false;
      regenCounter = 0;
      digitalWrite(RMIE, HIGH);
      //REGEN IS NOW DISABLED
      regenEnabled = false;
    }
  }
  
  if(connectionLost)
  {
    digitalWrite(redLED, HIGH);
    //Stop The Train
    //Serial.println("Connection lost");
    digitalWrite(RSBC, LOW); // kill the saftey brake circuit
    //Inverters.setSpeed(0);
    Serial1.write('Q');
    Serial1.write(0);
    Serial1.write('\n');
    //connectionLost = false; Disabled may break the reconnect!
  }
}

void serialUpdate()
{
  //Thermistor readings must be converted to byte values
  byte thrm1 = round(thermistorOneReading);
  byte thrm2 = round(thermistorTwoReading);
  Serial1.write('T');
  Serial1.write(thrm1);
  Serial1.write('Y');
  Serial1.write(thrm2);
  Serial1.write('R');
  byte states = 0;
    //note: MIC is the last input pin, RT is the first
  for(byte b = 0; b < 7; b++)//7 inputs to store
  {
    states = states << 1;
    if(digitalRead(47+b))
    {
      states |= B00000001;
    }
    
  }
  Serial1.write(states);
  Serial1.write('S');
  Serial1.write(currentSpeed);
  Serial1.write('\n');
}

//Comands from the hand held controler
void controlerCommand(byte serialIndex)
{
  //The 'U' command will get this far
  connectionTimeout = false;
  //Do Some Work
  for(int s = 0; s < serialIndex; s++)
  {
    if(controlBuffer[s] == 'U')
    {
      //Serial.println("U");
      return;
    }
    //checksum
    if(s + 2 < serialIndex && controlBuffer[s+1] == controlBuffer[s+2])
    {
      //Only a valid command can revive from conection lost.
      if(connectionLost == true)
      {
        Serial.println("Controler Connected");
        //Enable SBC and round train as we have a controler anyway
        digitalWrite(RSBC, HIGH);
        digitalWrite(RRTC, HIGH);
        //We are probs not on fire 
        digitalWrite(FIRE, HIGH);
        //and some break resistors wouldnt go a miss
        //digitalWrite(REN, HIGH);
        //also i would like the compressor
        digitalWrite(RCEN, HIGH);
        
        digitalWrite(redLED, LOW);
      }
      connectionLost = false;
      switch (controlBuffer[s])
      {
        case 'S': //Speed
          if(regenEnabled == false)
          {
            currentSpeed = controlBuffer[s+1];
            Inverters.setSpeed(controlBuffer[s+1]);
          }
          s += 2;
          break;
        case 'D': //Direction
          if(commandState(controlBuffer[s+1]))
          {
            Inverters.driveForward();
            digitalWrite(lightsForward, HIGH);
            digitalWrite(lightsBackward, LOW);
          }
          else
          {
            Inverters.driveReverse();
            digitalWrite(lightsForward, LOW);
            digitalWrite(lightsBackward, HIGH);
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
          digitalWrite(RMIE, LOW);
          regenEnabled = true;
          if(commandState(controlBuffer[s+1]))
          {
            //charge regen
            digitalWrite(reGenCharge, HIGH);
          }
          else
          {
            digitalWrite(reGenCharge, LOW);
          }
          s += 2;
          break;
        case 'Z': //Drive Regen
          digitalWrite(RMIE, LOW);
          regenEnabled = true;
          regenCounter--;
          if(commandState(controlBuffer[s+1]))
          {
            //Drive Regen
            digitalWrite(brakeRelease, HIGH);
            while(digitalRead(BPS))
            {
              delay(10);
            }
            digitalWrite(reGenDrive, HIGH);
          }
          else
          {
            digitalWrite(reGenDrive, LOW);
          }
          s += 2;
          break;
        case 'L': //Restroke
          if(commandState(controlBuffer[s+1]))
          {
            tripSet = true;
            tripSetStartTime= millis();
          }
          s += 2;
          break;
      }
    }
    else
    {
      Serial.println("junk");
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
    Serial.print("INV: ");
    Serial.println(!digitalRead(INV));
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
  if(micros() - motorOneLastTick > (30000)) // /numberOfMagnets)
  {
    motorOnePeriod = micros() - motorOneLastTick;
    if(regenEnabled == true && digitalRead(reGenCharge))
    {
      regenCounter++;
    }
    if(regenEnabled == true && digitalRead(reGenDrive))
    {
      regenCounter--;
    }
  }
  motorOneLastTick = micros();
}

void motorTwoSpeedInterupt()
{
  if(micros() - motorTwoLastTick > (30000)) // /numberOfMagnets)
  {
    motorTwoPeriod = micros() - motorTwoLastTick;  
  }
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
  digitalWrite(24, HIGH); //GER1
  delay(relayDelay);
  digitalWrite(25, HIGH); //GER2
  delay(relayDelay);
  digitalWrite(26, HIGH); //GES
//  while(digitalRead(GH))//While not genorator healthy
//  {
//    delay(100);
//    if(startTime + 4000 < millis())
//    {
//      digitalWrite(24, LOW);
//      digitalWrite(25, LOW);
//      digitalWrite(26, LOW);
//      return false;
//    }
//  }
  delay(4000);
  digitalWrite(26, LOW);
  delay(1000);
  digitalWrite(inverterPower, HIGH); //Enable the motor inverters, yay!
  return true;
}

void stopGenorator()
{
  digitalWrite(inverterPower, LOW);
  delay(1000);
  digitalWrite(24, LOW);
  digitalWrite(25, LOW);
}

void flick()
{
  Serial.println("Disabled");
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
  
