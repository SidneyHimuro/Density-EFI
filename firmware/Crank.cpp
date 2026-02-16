#include "Crank.h"

const byte rpmPin = 21;
byte pulsesPerRev = 60;
volatile unsigned long lastPulseMicros = 0, periodMicros = 0, avgPeriod = 0;
volatile byte toothCount = 0;
bool syncOK = false;
volatile unsigned long lastPmsMicros = 0;
unsigned int rpm = 0;

// Variável para o dente de sincronismo (vindo do menu)
extern byte denteSincro;  // Conecta com a variável do menu

void setupCrank() {
  pinMode(rpmPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(rpmPin), rpmISR, RISING);
}

void rpmISR() {
  unsigned long now = micros();
  unsigned long p = now - lastPulseMicros;
  lastPulseMicros = now;
  
  if (pulsesPerRev == 60) {
    // Média móvel para o período
    if (avgPeriod == 0) avgPeriod = p;
    else avgPeriod = (avgPeriod * 7 + p) / 8;
    
    // Detecção do dente faltante (gap)
    // O gap é aproximadamente 2x o período normal
    if (p > avgPeriod * 1.8) { 
      toothCount = 0;  // Reseta contagem no gap
      syncOK = true;    // Sincronismo estabelecido
    }
    else if (syncOK) { 
      toothCount++; 
      
      // Verifica se atingiu o dente de sincronismo configurado
      if (toothCount == denteSincro) {
        // Atingiu o dente de referência! 
        // Aqui você pode acionar eventos, resetar ângulos, etc.
        // Por enquanto, só marcamos
      }
      
      if (toothCount >= pulsesPerRev) {
        toothCount = 0; // Reinicia se passar da roda
      }
    }
  } else {
    // Modo distribuidor - sem dentes
    syncOK = true;
  }
  
  periodMicros = p;
}

void updateCrankRPM() {
  unsigned long p;
  noInterrupts(); 
  p = periodMicros; 
  interrupts();
  
  // Calcula RPM apenas se houver pulsos recentes (últimos 300ms)
  if (p > 0 && (micros() - lastPulseMicros) < 300000) {
    if(pulsesPerRev == 60) {
      rpm = 60000000UL / (p * pulsesPerRev);
    } else {
      // Para distribuidor: 2 pulsos por revolução (um por cilindro?)
      rpm = 60000000UL / (p * 2);
    }
  } else { 
    rpm = 0; 
    syncOK = false; 
  }
}