

#include "InverterController.h"

byte forwardPin;
byte backwardPin;
byte lowPin;
byte midPinInv1;
byte highPinInv1;
byte midPinInv2;
byte highPinInv2;
byte brakeRelease;
byte REN;
int inverterSpeed;
bool forwardEnable;

InverterController::InverterController(byte llowPin, byte lforwardPin, byte lbackwardPin, byte lmidPinInv1, byte lhighPinInv1, byte lmidPinInv2, byte lhighPinInv2)
{
    lowPin = llowPin;
	forwardPin = lforwardPin;
	backwardPin = lbackwardPin;
	midPinInv1 = lmidPinInv1;
	highPinInv1 = lhighPinInv1;
	midPinInv2 = lmidPinInv2;
	highPinInv2 = lhighPinInv2;
    REN = 45;
    brakeRelease = 29;
    forwardEnable = true;
    inverterSpeed = 0;
}

void InverterController::driveForward()
{
	Serial.println(F("Inverter in forward operation"));
	forwardEnable = true;
    digitalWrite(backwardPin, LOW);
    digitalWrite(forwardPin, HIGH);
}

void InverterController::driveReverse()
{
	Serial.println(F("Inverter in reverse operation"));
	forwardEnable = false;
    digitalWrite(backwardPin, HIGH);
    digitalWrite(forwardPin, LOW);
}

void InverterController::coast()
{
	digitalWrite(backwardPin, HIGH);
	digitalWrite(forwardPin, HIGH);
	digitalWrite(brakeRelease, HIGH);
}

void InverterController::setSpeed(int speed)
{
    inverterSpeed = speed;
    digitalWrite(REN, HIGH);
    if(speed > 0 && !digitalRead(brakeRelease))
    {
        digitalWrite(brakeRelease, HIGH);
    }
	if(speed == 0)
	{
        //for the moment pure REOH + DC Injection!
        //digitalWrite(forwardPin, HIGH);
        //digitalWrite(backwardPin, HIGH);
        //This section may be replaced with air brakes
        digitalWrite(lowPin, HIGH);
		digitalWrite(midPinInv1, LOW);
		digitalWrite(highPinInv1, LOW);
		digitalWrite(midPinInv2, LOW);
		digitalWrite(highPinInv2, LOW);
        return;
	}
    else
    {
        //Set Direction
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
        //Release Brakes
        digitalWrite(brakeRelease, HIGH);
    }
	if(speed == 1)
	{
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
        digitalWrite(lowPin, LOW);
		digitalWrite(midPinInv1, HIGH);
		digitalWrite(midPinInv2, HIGH);
		digitalWrite(highPinInv2, LOW);
        digitalWrite(highPinInv1, LOW);
	}
	else if(speed == 2)
	{
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
        digitalWrite(lowPin, LOW);
		digitalWrite(midPinInv1, LOW);
		digitalWrite(midPinInv2, LOW);
		digitalWrite(highPinInv2, HIGH);
        digitalWrite(highPinInv1, HIGH);
	}
	else if(speed == 3)
	{
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
        digitalWrite(lowPin, HIGH);
		digitalWrite(midPinInv1, HIGH);
		digitalWrite(midPinInv2, HIGH);
		digitalWrite(highPinInv2, LOW);
        digitalWrite(highPinInv1, LOW);
	}
    else if(speed == 4)
	{
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
        digitalWrite(lowPin, HIGH);
		digitalWrite(midPinInv1, LOW);
		digitalWrite(midPinInv2, LOW);
		digitalWrite(highPinInv2, HIGH);
        digitalWrite(highPinInv1, HIGH);
	}
    else if(speed == 5)
	{
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
        digitalWrite(lowPin, LOW);
		digitalWrite(midPinInv1, HIGH);
		digitalWrite(midPinInv2, HIGH);
		digitalWrite(highPinInv2, HIGH);
        digitalWrite(highPinInv1, HIGH);
	}
    else if(speed == 6)
	{
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
        digitalWrite(lowPin, HIGH);
		digitalWrite(midPinInv1, HIGH);
		digitalWrite(midPinInv2, HIGH);
		digitalWrite(highPinInv2, HIGH);
        digitalWrite(highPinInv1, HIGH);
	}
	else
	{
		coast();
	}
}

void InverterController::setDirection()
{
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

int InverterController::getSpeed()
{
    return inverterSpeed;
}