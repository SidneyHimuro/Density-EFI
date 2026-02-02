// IgnitionControl.h - COM CENTELHA PERDIDA
#ifndef IGNITION_CONTROL_H
#define IGNITION_CONTROL_H

#include <Arduino.h>

// ================= VARIÁVEIS DA IGNIÇÃO =================
extern float ignAdvance;     // Avanço atual em graus
extern float ignDwell;       // Tempo de carga da bobina (ms)
extern bool ignEnabled;      // Sistema habilitado
extern bool ignCut;          // Corte de ignição
extern bool wastedSparkMode; // Modo centelha perdida (true) ou distribuidor (false)
extern byte currentCylinder; // Cilindro atual (0-3 para 4 cilindros)

// ================= PINS =================
#define IGNITION_PIN_COIL_A 40  // Cilindros 1 e 4 (na ordem 1-3-4-2)
#define IGNITION_PIN_COIL_B 38  // Cilindros 2 e 3 (na ordem 1-3-4-2)

// ================= PROTÓTIPOS =================
void setupIgnition();
void setIgnitionAdvance(float degrees);
float getIgnitionAdvance();
void setIgnitionDwell(float dwellMs);
float getIgnitionDwell();
void enableIgnition(bool enable);
void setIgnitionCut(bool cut);
void setWastedSparkMode(bool wasted); // Nova função
bool getWastedSparkMode(); // Nova função
void updateIgnition(unsigned int currentRPM, bool currentSyncOK, byte pulsesPerRev = 60);
void updateIgnitionFromTeeth(byte toothCount, bool currentSyncOK); // Adicione esta linha
void fireIgnitionTest();
bool isIgnitionReady();
unsigned long getTimeToNextIgnition();
void triggerCoilA(); // Dispara bobina A (cilindros 1 e 4)
void triggerCoilB(); // Dispara bobina B (cilindros 2 e 3)

#endif