#ifndef INJECTOR_H
#define INJECTOR_H

#include <Arduino.h>

// Pino 22 corresponde ao Porto PA0 no ATmega2560
#define INJ_PIN 22 

// Variáveis globais compartilhadas
extern volatile bool injectorOn;
extern volatile unsigned int injPulseTicksLatched;
extern float Tinj_latched;
extern float AE_TPS;
extern float AE_TPS_max;
extern float AE_decay_ms;
extern float AE_decay_step;

// Protótipos das funções
void setupInjector();
void updateInjectorAE(float tpsPercent, unsigned int currentRpm);
// AQUI ESTAVA O ERRO: Adicionamos o parâmetro tpsPercent
void runInjector(unsigned int currentRpm, float tpsPercent); 

#endif