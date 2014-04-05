

#include "InverterController.h"

byte forwardPin;
byte backwardPin;
byte midPinInv1;
byte highPinInv1;
byte midPinInv2;
byte highPinInv2; 

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
	digitalWrite(backwardPin, LOW);
	digitalWrite(forwardPin, HIGH);
}

void InverterController::driveReverse()
{
	Serial.println(F("Inverter in reverse operation"));
	digitalWrite(backwardPin, HIGH);
	digitalWrite(forwardPin, LOW);
}

void InverterController::coast()
{
	digitalWrite(backwardPin, LOW);
	digitalWrite(forwardPin, LOW);
}

void InverterController::setSpeed(int speed)
{
	if(speed == 0)
	{
		coast();
	}
	else if(speed == 1)
	{
		digitalWrite(midPinInv1, LOW);
		digitalWrite(highPinInv1, LOW);
		digitalWrite(midPinInv2, LOW);
		digitalWrite(highPinInv2, LOW);
	}
	else if(speed == 2)
	{
		digitalWrite(midPinInv1, HIGH);
		digitalWrite(highPinInv1, LOW);
		digitalWrite(midPinInv2, HIGH);
		digitalWrite(highPinInv2, LOW);
	}
	else if(speed == 3)
	{
		digitalWrite(midPinInv1, LOW);
		digitalWrite(highPinInv1, HIGH);
		digitalWrite(midPinInv2, LOW);
		digitalWrite(highPinInv2, HIGH);
	}
	else if(speed == 4)
	{
		digitalWrite(midPinInv1, HIGH);
		digitalWrite(highPinInv1, HIGH);
		digitalWrite(midPinInv2, HIGH);
		digitalWrite(highPinInv2, HIGH);
	}
	else
	{
		coast();
	}
}