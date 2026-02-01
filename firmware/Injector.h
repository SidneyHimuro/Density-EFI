#ifndef INJECTOR_H
#define INJECTOR_H

#include <Arduino.h>

#define INJ_PIN 22 

extern volatile bool injectorOn;
extern volatile unsigned int injPulseTicksLatched;
extern float Tinj_latched;
extern float AE_TPS;
extern float AE_TPS_max;
extern float AE_decay_ms;

void setupInjector();
void updateInjectorAE(float tpsPercent, unsigned int currentRpm);
void runInjector(unsigned int currentRpm, float mapVal, float tpsPercent);

#endif