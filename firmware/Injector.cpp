#include "Injector.h"

// --- Variáveis de Controle de Estado ---
volatile bool injectorOn = false;
volatile unsigned int injPulseTicksLatched = 0;

// --- Parâmetros de Injeção e AE ---
float Tinj_latched = 0.0;
float AE_TPS = 0.0;
float AE_TPS_max = 5.0;      // Máximo de ms extras no AE
float AE_decay_ms = 300.0;   // Tempo de decaimento do AE
float AE_decay_step = 0.0;

// --- Variáveis Internas de Processamento ---
const float TPSDOT_MIN = 35.0;     // Sensibilidade ao movimento do pedal
const float AE_RPM_LIMIT = 5500.0; // Rotação onde o AE começa a ser mínimo
float tpsPrev = 0.0;
float tpsDotFiltered = 0.0;
unsigned long tpsPrevTime = 0;
unsigned long lastAEDecay = 0;
unsigned long lastInj = 0;

/**
 * Configura o hardware para o Pino 22 (Porta PA0) e Timer 4
 */
void setupInjector() {
  // Configura o Pino 22 (PA0 no Mega) como saída
  DDRA |= (1 << DDA0); 
  PORTA &= ~(1 << PA0); // Garante bico fechado
  
  // Timer 4 em modo CTC (Clear Timer on Compare Match)
  TCCR4A = 0; 
  TCCR4B = (1 << WGM42) | (1 << CS41); // Prescaler 8 (0.5us por tick)
  
  TIMSK4 = 0; // Interrupções começam desativadas
}

/**
 * ISR: Desliga o injetor via hardware no tempo exato
 */
ISR(TIMER4_COMPA_vect) {
  PORTA &= ~(1 << PA0);     // Fecha o bico (Pino 22)
  injectorOn = false;
  TIMSK4 &= ~(1 << OCIE4A); // Desarma a interrupção
}

/**
 * Lógica Avançada de Enriquecimento por Aceleração (AE)
 */
void updateInjectorAE(float tpsPercent, unsigned int currentRpm) {
  unsigned long now = millis();
  float dt = (now - tpsPrevTime) / 1000.0;

  if (dt > 0.005) { // Executa a aprox. 200Hz
    float tpsDotRaw = (tpsPercent - tpsPrev) / dt;
    
    // Filtro Passa-Baixa (Alpha Filter) para remover ruído elétrico do TPS
    tpsDotFiltered = (tpsDotRaw * 0.6) + (tpsDotFiltered * 0.4);

    if (tpsDotFiltered > TPSDOT_MIN && currentRpm > 450) {
      // 1. Intensidade baseada na velocidade do pé (tpsDot)
      float intensity = (tpsDotFiltered - TPSDOT_MIN) / 450.0;
      intensity = constrain(intensity, 0.0, 1.0);

      // 2. RPM Taper: Menos AE em altas rotações (ar já tem inércia)
      float rpmFactor = 1.0 - (currentRpm / AE_RPM_LIMIT);
      rpmFactor = constrain(rpmFactor, 0.15, 1.0);

      // 3. Aplica novo pulso de AE se for maior que o atual
      float newAE = AE_TPS_max * intensity * rpmFactor;
      if (newAE > AE_TPS) {
        AE_TPS = newAE;
        AE_decay_step = AE_TPS / (AE_decay_ms / 10.0);
      }
    }
    tpsPrev = tpsPercent;
    tpsPrevTime = now;
  }

  // Decaimento linear do AE (roda a cada 10ms)
  if (AE_TPS > 0 && (now - lastAEDecay >= 10)) {
    AE_TPS -= AE_decay_step;
    if (AE_TPS < 0) AE_TPS = 0;
    lastAEDecay = now;
  }
}

/**
 * Função principal de execução da injeção
 */
void runInjector(unsigned int currentRpm, float tpsPercent) {
  // --- Função Flood Clear ---
  // Se TPS > 90% e motor tentando ligar (cranking), corta combustível para desalogar
  if (currentRpm < 600 && tpsPercent > 90.0) {
    Tinj_latched = 0;
    AE_TPS = 0;
  }

  // Calcula ticks (Total = Base do Mapa + Extra do AE)
  float totalPulseTime = Tinj_latched + AE_TPS;
  injPulseTicksLatched = (unsigned int)(totalPulseTime * 2000.0);

  if (!injectorOn && currentRpm > 40) {
    unsigned long interval = 60000000UL / currentRpm;
    if (micros() - lastInj >= interval) {
      lastInj = micros();
      
      if (injPulseTicksLatched > 150) { // Proteção: Tempo mínimo de pulso
        PORTA |= (1 << PA0);          // Abre o bico (Pino 22)
        injectorOn = true;
        
        TCNT4 = 0;                    // Zera contador do Timer 4
        OCR4A = injPulseTicksLatched;   // Define o alvo da interrupção
        TIFR4 |= (1 << OCF4A);        // Limpa flags pendentes
        TIMSK4 |= (1 << OCIE4A);      // Ativa interrupção de comparação
      }
    }
  }
}