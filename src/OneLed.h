// OneLed.h

#ifndef _ONELED_h
#define _ONELED_h
#include "Timer.h"

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
	#include "WProgram.h"
#endif

enum class StatLed { OFF, ON, BLINK, DIM };

class OneLed:	public Timer
{
	const int pin;
	int level{ 0 };
	int levelDim{ 0 };
	int maxLevel{100};
	StatLed statLed{ StatLed::ON };
	const int ledChannel = 0;
	const int resolution = 8;
	const int freq = 100;
public:
	OneLed(int p);
	int getPin() { return pin; }
	void setStat(StatLed stat);
	void trigger();	//изменение состояния на обратное
	void setDim(int l);
	void setMaxLevel(int i);
	int getMaxLevel(){return maxLevel;}
	virtual void cycle();
};

#endif

