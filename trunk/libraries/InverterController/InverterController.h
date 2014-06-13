#ifndef InverterController_H
#define InverterController_H

#include <Arduino.h>

class InverterController
{
public:
	InverterController(byte lowPin, byte forwardPin, byte reversePin, byte midPin, byte highPin, byte midPin2, byte highPin2);
	void driveForward();
	void driveReverse();
	void coast();
	void setSpeed(int speed);
    int getSpeed();
private:
    void setDirection();
};

#endif
