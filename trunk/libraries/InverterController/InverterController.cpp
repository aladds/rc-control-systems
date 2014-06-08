

#include "InverterController.h"

byte forwardPin;
byte backwardPin;
byte midPinInv1;
byte highPinInv1;
byte midPinInv2;
byte highPinInv2;
byte brakeRelease = 29;
byte REN = 45;
int currentSpeed
bool forwardEnable = true;

InverterController::InverterController(byte lforwardPin, byte lbackwardPin, byte lmidPinInv1, byte lhighPinInv1, byte lmidPinInv2, byte lhighPinInv2)
{
	forwardPin = lforwardPin;
	backwardPin = lbackwardPin;
	midPinInv1 = lmidPinInv1;
	highPinInv1 = lhighPinInv1;
	midPinInv2 = lmidPinInv2;
	highPinInv2 = lhighPinInv2;
}

void InverterController::driveForward()
{
	Serial.println(F("Inverter in forward operation"));
	forwardEnable = true;
}

void InverterController::driveReverse()
{
	Serial.println(F("Inverter in reverse operation"));
	forwardEnable = false;
}

void InverterController::coast()
{
	digitalWrite(backwardPin, LOW);
	digitalWrite(forwardPin, LOW);
}

void InverterController::setSpeed(int speed)
{
    currentSpeed = speed;
    if(speed > 0 && !digitalRead(brakeRelease))
    {
        digitalWrite(brakeRelease, HIGH);
        delay(200);
    }
	if(speed == 0)
	{
        digitalWrite(REN, LOW);
        //for the moment pure REOH + DC Injection!
        digitalWrite(forwardPin, HIGH);
        digitalWrite(backwardPin, HIGH);
        //This section may be replaced with air brakes
		digitalWrite(midPinInv1, LOW);
		digitalWrite(highPinInv1, LOW);
		digitalWrite(midPinInv2, LOW);
		digitalWrite(highPinInv2, LOW);
	}
	else if(speed == 1)
	{
        digitalWrite(REN, HIGH);
        digitalWrite(brakeRelease, HIGH);
		digitalWrite(midPinInv1, LOW);
		digitalWrite(highPinInv1, LOW);
		digitalWrite(midPinInv2, LOW);
		digitalWrite(highPinInv2, LOW);
        if(forwardEnable == true)
        {
            digitalWrite(backwardPin, LOW);
            digitalWrite(forwardPin, HIGH);
        }
        else
        {
            digitalWrite(backwardPin, HIGH);
            digitalWrite(forwardPin, LOW);
        }
        }
	}
	else if(speed == 2)
	{
        digitalWrite(REN, HIGH);
        digitalWrite(brakeRelease, HIGH);
		digitalWrite(midPinInv1, HIGH);
		digitalWrite(highPinInv1, LOW);
		digitalWrite(midPinInv2, HIGH);
		digitalWrite(highPinInv2, LOW);
        if(forwardEnable == true)
        {
            digitalWrite(backwardPin, LOW);
            digitalWrite(forwardPin, HIGH);
        }
        else
        {
            digitalWrite(backwardPin, HIGH);
            digitalWrite(forwardPin, LOW);
        }
	}
	else if(speed == 3)
	{
        digitalWrite(REN, HIGH);
        digitalWrite(brakeRelease, HIGH);
		digitalWrite(midPinInv1, LOW);
		digitalWrite(highPinInv1, HIGH);
		digitalWrite(midPinInv2, LOW);
		digitalWrite(highPinInv2, HIGH);
        if(forwardEnable == true)
        {
            digitalWrite(backwardPin, LOW);
            digitalWrite(forwardPin, HIGH);
        }
        else
        {
            digitalWrite(backwardPin, HIGH);
            digitalWrite(forwardPin, LOW);
        }
	}
	else if(speed == 4)
	{
        digitalWrite(REN, HIGH);
        digitalWrite(brakeRelease, HIGH);
		digitalWrite(midPinInv1, HIGH);
		digitalWrite(highPinInv1, HIGH);
		digitalWrite(midPinInv2, HIGH);
		digitalWrite(highPinInv2, HIGH);
        if(forwardEnable == true)
        {
            digitalWrite(backwardPin, LOW);
            digitalWrite(forwardPin, HIGH);
        }
        else
        {
            digitalWrite(backwardPin, HIGH);
            digitalWrite(forwardPin, LOW);
        }
	}
	else
	{
		coast();
	}
}

int InverterController::getSpeed()
{
    return speed;
}