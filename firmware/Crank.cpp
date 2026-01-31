#include "Crank.h"

const byte rpmPin = 21;
byte pulsesPerRev = 60;
volatile unsigned long lastPulseMicros = 0, periodMicros = 0, avgPeriod = 0;
volatile byte toothCount = 0;
volatile bool syncOK = false;
volatile unsigned long lastPmsMicros = 0;
unsigned int rpm = 0;

void setupCrank() {
  pinMode(rpmPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(rpmPin), rpmISR, RISING);
}

void rpmISR() {
  unsigned long now = micros();
  unsigned long p = now - lastPulseMicros;
  lastPulseMicros = now;
  if (pulsesPerRev == 60) {
    if (avgPeriod == 0) avgPeriod = p;
    else avgPeriod = (avgPeriod * 7 + p) / 8;
    if (p > avgPeriod * 1.8) { toothCount = 0; syncOK = true; }
    else if (syncOK) { toothCount++; if (toothCount >= pulsesPerRev) toothCount = 0; }
  } else {
    syncOK = true;
  }
  periodMicros = p;
}

void updateCrankRPM() {
  unsigned long p;
  noInterrupts(); p = periodMicros; interrupts();
  if (p > 0 && (micros() - lastPulseMicros) < 300000) {
      if(pulsesPerRev == 60) rpm = 60000000UL / (p * pulsesPerRev);
      else rpm = 60000000UL / (p * 2);
  }
  else { rpm = 0; syncOK = false; }
}