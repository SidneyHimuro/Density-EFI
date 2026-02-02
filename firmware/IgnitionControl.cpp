// IgnitionControl.cpp - COM CENTELHA PERDIDA
#include "IgnitionControl.h"
#include <Arduino.h>

// ================= VARIÁVEIS DA IGNIÇÃO =================
extern float ignAdvance;
float ignDwell = 3.5;
bool ignEnabled = true;
bool ignCut = false;
bool wastedSparkMode = true; // Padrão: centelha perdida
byte currentCylinder = 0;    // 0-3 para 4 cilindros (1-3-4-2)
byte lastToothCount = 0;

// ================= VARIÁVEIS DE TEMPORIZAÇÃO =================
unsigned long lastIgnitionTime = 0;
unsigned long ignitionInterval = 0;
unsigned long dwellStartTime = 0;
bool dwellActive = false;
bool sparkFired = false;
byte coilToFire = 0; // 0 = nenhuma, 1 = bobina A, 2 = bobina B

// ================= IMPLEMENTAÇÃO =================

void setupIgnition() {
  pinMode(IGNITION_PIN_COIL_A, OUTPUT);
  pinMode(IGNITION_PIN_COIL_B, OUTPUT);
  digitalWrite(IGNITION_PIN_COIL_A, LOW);
  digitalWrite(IGNITION_PIN_COIL_B, LOW);
  
  lastIgnitionTime = micros();
}

void setIgnitionAdvance(float degrees) {
  ignAdvance = constrain(degrees, 5.0, 40.0);
}

float getIgnitionAdvance() {
  return ignAdvance;
}

void setIgnitionDwell(float dwellMs) {
  ignDwell = constrain(dwellMs, 1.0, 5.0);
}

float getIgnitionDwell() {
  return ignDwell;
}

void enableIgnition(bool enable) {
  ignEnabled = enable;
  if (!enable) {
    digitalWrite(IGNITION_PIN_COIL_A, LOW);
    digitalWrite(IGNITION_PIN_COIL_B, LOW);
    dwellActive = false;
    sparkFired = false;
  }
}

void setIgnitionCut(bool cut) {
  ignCut = cut;
}

void setWastedSparkMode(bool wasted) {
  wastedSparkMode = wasted;
  if (!wastedSparkMode) {
    // Modo distribuidor: apenas bobina A ativa
    digitalWrite(IGNITION_PIN_COIL_B, LOW);
  }
}

bool getWastedSparkMode() {
  return wastedSparkMode;
}

void triggerCoilA() {
  if (!ignEnabled || ignCut) return;
  
  digitalWrite(IGNITION_PIN_COIL_A, HIGH);
  delayMicroseconds((unsigned long)(ignDwell * 1000));
  digitalWrite(IGNITION_PIN_COIL_A, LOW);
}

void triggerCoilB() {
  if (!ignEnabled || ignCut) return;
  
  digitalWrite(IGNITION_PIN_COIL_B, HIGH);
  delayMicroseconds((unsigned long)(ignDwell * 1000));
  digitalWrite(IGNITION_PIN_COIL_B, LOW);
}

// FUNÇÃO PRINCIPAL - RECEBE rpm, syncOK E pulsesPerRev
void updateIgnition(unsigned int currentRPM, bool currentSyncOK, byte pulsesPerRev) {
  if (!ignEnabled || ignCut || !currentSyncOK || currentRPM == 0) {
    digitalWrite(IGNITION_PIN_COIL_A, LOW);
    digitalWrite(IGNITION_PIN_COIL_B, LOW);
    dwellActive = false;
    return;
  }
  
  // Limitar RPM máxima
  if (currentRPM > 8000) {
    digitalWrite(IGNITION_PIN_COIL_A, LOW);
    digitalWrite(IGNITION_PIN_COIL_B, LOW);
    return;
  }
  
  unsigned long currentTime = micros();
  
  // Calcular intervalo
  ignitionInterval = 60000000UL / currentRPM;
  
  // Para centelha perdida, o intervalo é metade (720° / 2 = 360°)
  unsigned long sparkInterval = wastedSparkMode ? (ignitionInterval / 2) : ignitionInterval;
  
  // Calcular avanço
  unsigned long advanceMicros = (unsigned long)((ignAdvance * ignitionInterval) / 360.0);
  unsigned long dwellStartOffset = sparkInterval - advanceMicros;
  
  // Iniciar dwell
  if (!dwellActive) {
    if (currentTime - lastIgnitionTime >= dwellStartOffset) {
      // Determinar qual bobina disparar
      if (wastedSparkMode) {
        // Modo centelha perdida
        if (coilToFire == 0) {
          coilToFire = 1; // Começa com bobina A
        } else {
          coilToFire = (coilToFire == 1) ? 2 : 1; // Alterna entre A e B
        }
      } else {
        // Modo distribuidor: sempre bobina A
        coilToFire = 1;
      }
      
      // Ativar a bobina selecionada
      if (coilToFire == 1) {
        digitalWrite(IGNITION_PIN_COIL_A, HIGH);
      } else if (coilToFire == 2) {
        digitalWrite(IGNITION_PIN_COIL_B, HIGH);
      }
      
      dwellStartTime = currentTime;
      dwellActive = true;
      sparkFired = false;
    }
  }
  
  // Finalizar dwell e disparar
  if (dwellActive && !sparkFired) {
    if (currentTime - dwellStartTime >= (unsigned long)(ignDwell * 1000)) {
      // Desligar a bobina para gerar centelha
      if (coilToFire == 1) {
        digitalWrite(IGNITION_PIN_COIL_A, LOW);
      } else if (coilToFire == 2) {
        digitalWrite(IGNITION_PIN_COIL_B, LOW);
      }
      sparkFired = true;
    }
  }
  
  // Resetar ciclo
  if (currentTime - lastIgnitionTime >= sparkInterval) {
    lastIgnitionTime = currentTime;
    dwellActive = false;
    sparkFired = false;
  }
}

// Função para disparo baseado no dente do sensor (para sensor 60-2)
void updateIgnitionFromTeeth(byte toothCount, bool currentSyncOK) {
  if (!ignEnabled || ignCut || !currentSyncOK) return;
  
  // Para sensor 60-2 (60 pulsos por revolução, com 2 dentes faltando)
  // Cada dente = 6° (360° / 60)
  // A cada 30 dentes = 180° = tempo para próxima centelha
  
  if (toothCount == 0) {
    // Dente de sincronização (primeiro após o gap)
    currentCylinder = 0;
  }
  
  // Determinar quando disparar baseado no dente
  // Para centelha perdida, disparar a cada 180° (30 dentes)
  if (toothCount % 30 == 0) {
    // Calcular qual bobina disparar baseado no cilindro
    if (wastedSparkMode) {
      // Sequência 1-3-4-2 para 4 cilindros
      switch (currentCylinder) {
        case 0: // Cilindro 1 (e 4)
          triggerCoilA();
          break;
        case 1: // Cilindro 3 (e 2)
          triggerCoilB();
          break;
        case 2: // Cilindro 4 (e 1)
          triggerCoilA();
          break;
        case 3: // Cilindro 2 (e 3)
          triggerCoilB();
          break;
      }
      currentCylinder = (currentCylinder + 1) % 4;
    } else {
      // Modo distribuidor: sempre bobina A
      triggerCoilA();
    }
  }
}

void fireIgnitionTest() {
  if (!ignEnabled) return;
  
  if (wastedSparkMode) {
    // Teste ambas as bobinas
    triggerCoilA();
    delay(100);
    triggerCoilB();
  } else {
    // Teste apenas bobina A
    triggerCoilA();
  }
}

bool isIgnitionReady() {
  return (ignEnabled && !ignCut);
}

unsigned long getTimeToNextIgnition() {
  return ignitionInterval;
}