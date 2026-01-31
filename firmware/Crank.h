#ifndef CRANK_H
#define CRANK_H

#include <Arduino.h>

extern const byte rpmPin;
extern byte pulsesPerRev;
extern volatile unsigned long lastPulseMicros, periodMicros, avgPeriod;
extern volatile byte toothCount;
extern volatile bool syncOK;
extern volatile unsigned long lastPmsMicros;
extern unsigned int rpm;

void setupCrank();
void rpmISR();
void updateCrankRPM();

#endif