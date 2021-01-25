#include "OneLed.h"

//***********************************************
	OneLed::OneLed(int p): Timer(8), pin(p) { 
		pinMode(pin, OUTPUT); 
		ledcSetup(ledChannel, freq, resolution);
		ledcAttachPin(pin, ledChannel);
	}
//*************************************************
void OneLed::setDim(int l) {
	levelDim = l; 
//	Serial.println(levelDim);
}
//***********************************************
void OneLed::trigger(){
	switch (statLed) {
		case StatLed::ON:levelDim = 0; statLed = StatLed::OFF; break;
		case StatLed::BLINK:
		case StatLed::DIM:
		case StatLed::OFF:levelDim = maxLevel; statLed = StatLed::ON; break;
	}
}
//*********************************************
void OneLed::setMaxLevel(int i){
	maxLevel = i;
	if(statLed == StatLed::ON) {
		levelDim = maxLevel;
	}
}
//*********************************************
void OneLed::setStat(StatLed stat) {
	statLed = stat; 
	switch (statLed) {
		case StatLed::OFF:levelDim = 0; break;
		case StatLed::BLINK:
		case StatLed::ON:levelDim = maxLevel; break;
		case StatLed::DIM:break;
	}
}
//*************************************
void OneLed::cycle() {
	if (level != levelDim) {
		if (getTimer()) {
			setTimer();
			level += (level < levelDim? 1: -1);
		}
	}
	ledcWrite(ledChannel, level);
}

