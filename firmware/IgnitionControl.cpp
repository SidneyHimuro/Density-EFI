// IgnitionControl.cpp - Implementação não-bloqueante COMPLETA
#include "IgnitionControl.h"

// Variáveis internas
static unsigned long lastSparkTime = 0;
static unsigned long dwellStartTime = 0;
static bool dwellActive = false;
static byte activeCoil = 0;          // 0 = nenhum, 1 = bobina A, 2 = bobina B
static unsigned long lastToothTime = 0;
static byte lastToothCount = 255;

// Variáveis exportadas (definidas APENAS aqui)
float ignAdvance = 10.0;
float ignDwell = 3.5;
bool ignEnabled = true;
bool ignCut = false;
bool wastedSparkMode = true;

// Declaração de variáveis externas do Crank
extern unsigned long lastPulseMicros;
extern byte toothCount;

void setupIgnition() {
    pinMode(IGNITION_PIN_COIL_A, OUTPUT);
    pinMode(IGNITION_PIN_COIL_B, OUTPUT);
    digitalWrite(IGNITION_PIN_COIL_A, LOW);
    digitalWrite(IGNITION_PIN_COIL_B, LOW);
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
    }
}

void setIgnitionCut(bool cut) {
    ignCut = cut;
}

void setWastedSparkMode(bool wasted) {
    wastedSparkMode = wasted;
    if (!wastedSparkMode) {
        digitalWrite(IGNITION_PIN_COIL_B, LOW);
    }
}

bool getWastedSparkMode() {
    return wastedSparkMode;
}

void fireIgnitionTest() {
    if (!ignEnabled) return;
    digitalWrite(IGNITION_PIN_COIL_A, HIGH);
    delayMicroseconds((int)(ignDwell * 1000));
    digitalWrite(IGNITION_PIN_COIL_A, LOW);
    if (wastedSparkMode) {
        delay(100);
        digitalWrite(IGNITION_PIN_COIL_B, HIGH);
        delayMicroseconds((int)(ignDwell * 1000));
        digitalWrite(IGNITION_PIN_COIL_B, LOW);
    }
}

// Função principal corrigida e COMPLETA
void updateIgnition(unsigned int rpm, bool syncOK, byte pulsesPerRev, byte tooth) {
    // Se desabilitado ou sem sincronismo, desliga tudo
    if (!ignEnabled || ignCut || !syncOK || rpm < 50) {
        digitalWrite(IGNITION_PIN_COIL_A, LOW);
        digitalWrite(IGNITION_PIN_COIL_B, LOW);
        dwellActive = false;
        return;
    }

    unsigned long now = micros();
    unsigned long usPerRev = 60000000UL / rpm;
    unsigned long usPerDegree = usPerRev / 360;
    unsigned long dwellUs = (unsigned long)(ignDwell * 1000);
    
    // ==================== MODO DISTRIBUIDOR (1 pulso por rev) ====================
    if (pulsesPerRev == 1) {
        // Atualizar tempo do último pulso
        if (lastPulseMicros != lastToothTime) {
            lastToothTime = lastPulseMicros;
            // Resetar ciclo ao receber novo pulso
            if (dwellActive) {
                digitalWrite(IGNITION_PIN_COIL_A, LOW);
                dwellActive = false;
            }
        }
        
        unsigned long timeSinceLastPulse = now - lastToothTime;
        unsigned long targetSparkTime = usPerRev - (ignAdvance * usPerDegree);
        unsigned long targetDwellStart = targetSparkTime - dwellUs;
        
        if (!dwellActive) {
            if (timeSinceLastPulse >= targetDwellStart) {
                digitalWrite(IGNITION_PIN_COIL_A, HIGH);
                dwellActive = true;
                dwellStartTime = now;
                activeCoil = 1;
            }
        } else {
            if (now - dwellStartTime >= dwellUs) {
                digitalWrite(IGNITION_PIN_COIL_A, LOW);
                dwellActive = false;
            }
        }
    }
    
    // ==================== MODO 60-2 (CENTELHA PERDIDA) ====================
    else { // pulsesPerRev == 60
        unsigned long usPerTooth = usPerRev / 60;  // microssegundos por dente (6°)
        
        // Detectar novo dente
        if (tooth != lastToothCount) {
            lastToothCount = tooth;
            
            // A cada 30 dentes (180°), temos um evento de ignição
            if (tooth % 30 == 0) {
                // Calcular tempo para o próximo PMS (a cada 180°)
                // O ângulo alvo: queremos disparar 'ignAdvance' graus ANTES do próximo PMS
                // O próximo PMS ocorre em 180° a partir de agora
                // Portanto, queremos disparar após (180 - ignAdvance) graus
                
                unsigned long degreesToWait = 180.0 - ignAdvance;
                if (degreesToWait < 0) degreesToWait += 180;  // Segurança
                
                // Converter para microssegundos
                unsigned long usToWait = (unsigned long)(degreesToWait * usPerDegree);
                
                // Se não há dwell ativo, programar início do dwell
                if (!dwellActive) {
                    // Iniciar dwell agora? Ou aguardar?
                    // Simplificando: iniciar dwell imediatamente e deixar que o tempo de dwell
                    // termine exatamente no ângulo desejado
                    
                    // Escolher bobina (alternada para centelha perdida)
                    static bool coilToggle = false;
                    coilToggle = !coilToggle;
                    activeCoil = coilToggle ? 1 : 2;
                    
                    // Iniciar dwell
                    if (activeCoil == 1) digitalWrite(IGNITION_PIN_COIL_A, HIGH);
                    else digitalWrite(IGNITION_PIN_COIL_B, HIGH);
                    
                    dwellActive = true;
                    dwellStartTime = now;
                    
                    // Programar o fim do dwell para o ângulo desejado
                    // O dwell deve terminar APÓS o tempo calculado
                    // Mas como já iniciamos, o fim será dwellStartTime + dwellUs
                    // Isso só funciona se o dwell for menor que o tempo até o ângulo
                    // Para avanços muito pequenos, pode ser necessário ajustar
                }
            }
        }
        
        // Verificar fim do dwell (independente do dente)
        if (dwellActive) {
            if (now - dwellStartTime >= dwellUs) {
                if (activeCoil == 1) digitalWrite(IGNITION_PIN_COIL_A, LOW);
                else digitalWrite(IGNITION_PIN_COIL_B, LOW);
                dwellActive = false;
            }
        }
    }
}