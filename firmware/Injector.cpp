#include "Injector.h"

// --- Eixos da Tabela (Devem coincidir com o .ino se possível) ---
const int rpmAxisTable[16] = {500, 800, 1200, 1600, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500};
const float mapAxisTable[16] = {0.0, -0.07, -0.14, -0.21, -0.28, -0.35, -0.42, -0.49, -0.56, -0.63, -0.70, -0.77, -0.84, -0.91, -0.96, -1.0};

// Variáveis Globais
volatile bool injectorOn = false;
volatile unsigned int injPulseTicksLatched = 0;
float Tinj_latched = 0.0;
float AE_TPS = 0.0;
float AE_TPS_max = 5.0;
float AE_decay_ms = 300.0;
float AE_decay_step = 0.0;

// Internas
float tpsPrev = 0.0;
float tpsDotFiltered = 0.0;
unsigned long tpsPrevTime = 0;
unsigned long lastAEDecay = 0;
unsigned long lastInj = 0;

// Função de Interpolação Bilinear 2D
float calculateTinj(int rpmVal, float mapVal) {
    byte iR = 0, iM = 0;
    while (iR < 14 && rpmVal > rpmAxisTable[iR + 1]) iR++;
    while (iM < 14 && mapVal < mapAxisTable[iM + 1]) iM++; // Invertido para eixos negativos

    float fr = (float)(rpmVal - rpmAxisTable[iR]) / (rpmAxisTable[iR + 1] - rpmAxisTable[iR]);
    float fm = (float)(mapVal - mapAxisTable[iM]) / (mapAxisTable[iM + 1] - mapAxisTable[iM]);

    // O acesso à injTable aqui requer que ela seja externa ou definida aqui. 
    // Para simplificar, usamos a global injTable definida no seu .ino
    extern float injTable[16][16]; 

    float a = injTable[iR][iM] + fr * (injTable[iR + 1][iM] - injTable[iR][iM]);
    float b = injTable[iR][iM + 1] + fr * (injTable[iR + 1][iM + 1] - injTable[iR][iM + 1]);
    return a + fm * (b - a);
}

void setupInjector() {
    DDRA |= (1 << DDA0); 
    PORTA &= ~(1 << PA0); 
    TCCR4A = 0; 
    TCCR4B = (1 << WGM42) | (1 << CS41); // CTC, Prescaler 8
    TIMSK4 = 0; 
}

ISR(TIMER4_COMPA_vect) {
    PORTA &= ~(1 << PA0);
    injectorOn = false;
    TIMSK4 &= ~(1 << OCIE4A);
}

void updateInjectorAE(float tpsPercent, unsigned int currentRpm) {
    unsigned long now = millis();
    float dt = (now - tpsPrevTime) / 1000.0;
    
    if (dt > 0.005) {
        float tpsDotRaw = (tpsPercent - tpsPrev) / dt;
        tpsDotFiltered = (tpsDotRaw * 0.6) + (tpsDotFiltered * 0.4);
        
        if (tpsDotFiltered > 35.0 && currentRpm > 450) {
            float intensity = constrain((tpsDotFiltered - 35.0) / 450.0, 0.0, 1.0);
            float rpmFactor = constrain(1.0 - (currentRpm / 5500.0), 0.15, 1.0);
            float newAE = AE_TPS_max * intensity * rpmFactor;
            
            if (newAE > AE_TPS) {
                AE_TPS = newAE;
                AE_decay_step = AE_TPS / (AE_decay_ms / 10.0);
            }
        }
        tpsPrev = tpsPercent;
        tpsPrevTime = now;
    }

    if (AE_TPS > 0 && (now - lastAEDecay >= 10)) {
        AE_TPS -= AE_decay_step;
        if (AE_TPS < 0) AE_TPS = 0;
        lastAEDecay = now;
    }
}

void runInjector(unsigned int currentRpm, float mapVal, float tpsPercent) {
    if (currentRpm > 40) {
        Tinj_latched = calculateTinj(currentRpm, mapVal);
        
        // Flood Clear
        if (currentRpm < 600 && tpsPercent > 90.0) {
            Tinj_latched = 0;
            AE_TPS = 0;
        }

        float totalPulseTime = Tinj_latched + AE_TPS;
        injPulseTicksLatched = (unsigned int)(totalPulseTime * 2000.0); // 1ms = 2000 ticks @8MHz/8

        if (!injectorOn) {
            unsigned long interval = 60000000UL / currentRpm;
            if (micros() - lastInj >= interval) {
                lastInj = micros();
                if (injPulseTicksLatched > 100) {
                    PORTA |= (1 << PA0);
                    injectorOn = true;
                    TCNT4 = 0;
                    OCR4A = injPulseTicksLatched;
                    TIFR4 |= (1 << OCF4A);
                    TIMSK4 |= (1 << OCIE4A);
                }
            }
        }
    }
}