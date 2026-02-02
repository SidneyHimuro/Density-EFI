#ifndef CRANK_H
#define CRANK_H

#include <Arduino.h>

extern const byte rpmPin;
extern byte pulsesPerRev;
extern volatile unsigned long lastPulseMicros, periodMicros, avgPeriod;
extern volatile byte toothCount;
extern volatile unsigned long lastPmsMicros;


// Variáveis compartilhadas
extern unsigned int rpm;    // ADICIONAR 'volatile'
extern bool syncOK;         // ADICIONAR 'volatile'

void setupCrank();
void rpmISR();
void updateCrankRPM();

#endif