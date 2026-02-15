// IgnitionControl.h - Controle de ignição não-bloqueante com avanço variável
#ifndef IGNITION_CONTROL_H
#define IGNITION_CONTROL_H

#include <Arduino.h>

// ================= Pinos das Bobinas =================
#define IGNITION_PIN_COIL_A 40  // Cilindros 1 e 4 (centelha perdida) ou único (distribuidor)
#define IGNITION_PIN_COIL_B 38  // Cilindros 2 e 3 (apenas centelha perdida)

// ================= Variáveis Globais =================
extern float ignAdvance;          // Avanço atual (graus)
extern float ignDwell;            // Tempo de carga (ms)
extern bool ignEnabled;           // Sistema habilitado
extern bool ignCut;               // Corte de ignição
extern bool wastedSparkMode;      // true = centelha perdida, false = distribuidor

// ================= Funções Públicas =================
void setupIgnition();
void setIgnitionAdvance(float degrees);
float getIgnitionAdvance();
void setIgnitionDwell(float dwellMs);
float getIgnitionDwell();
void enableIgnition(bool enable);
void setIgnitionCut(bool cut);
void setWastedSparkMode(bool wasted);
bool getWastedSparkMode();

// Função principal de atualização (chamada no loop)
void updateIgnition(unsigned int currentRPM, bool syncOK, byte pulsesPerRev, byte toothCount = 0);

// Função de teste
void fireIgnitionTest();

#endif