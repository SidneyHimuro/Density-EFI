#include <LiquidCrystal.h>

// ================= LCD =================
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// ================= RPM =================
const byte rpmPin = 21;       // INT2
const byte pulsesPerRev = 60; // RODA FÔNICA 60-2

volatile unsigned long lastPulseMicros = 0;
volatile unsigned long periodMicros = 0;
unsigned int rpm = 0;

// ================= MAP =================
const byte mapPin = A4;
const int adc_1V = 205;
const int adc_5V = 1023;

float mapBar = 0.0;

// Filtro MAP
const byte MAP_SAMPLES = 8;
int mapBuffer[MAP_SAMPLES];
byte mapIndex = 0;

// ================= INJ =================
float Tinj = 0.0;

// ================= EIXOS =================
const int rpmAxis[16] = {
   500,  800, 1200, 1600,
  2000, 2500, 3000, 3500,
  4000, 4500, 5000, 5500,
  6000, 6500, 7000, 7500
};

const float mapAxis[16] = {
   0.0, -0.07, -0.14, -0.21,
  -0.28, -0.35, -0.42, -0.49,
  -0.56, -0.63, -0.70, -0.77,
  -0.84, -0.91, -0.96, -1.0
};

// ================= TABELA 16x16 (ms) =================
float injTable[16][16] = {
 {1.2,1.1,1.0,0.9,0.8,0.8,0.7,0.7,0.6,0.6,0.6,0.5,0.5,0.5,0.5,0.5},
 {1.4,1.3,1.2,1.1,1.0,0.9,0.9,0.8,0.8,0.7,0.7,0.6,0.6,0.6,0.6,0.6},
 {1.8,1.7,1.6,1.5,1.4,1.3,1.2,1.1,1.0,0.9,0.9,0.8,0.8,0.8,0.8,0.8},
 {2.2,2.1,2.0,1.9,1.8,1.7,1.6,1.5,1.4,1.3,1.2,1.1,1.1,1.1,1.1,1.1},
 {2.6,2.5,2.4,2.3,2.2,2.1,2.0,1.9,1.8,1.7,1.6,1.5,1.4,1.4,1.4,1.4},
 {3.0,2.9,2.8,2.7,2.6,2.5,2.4,2.3,2.2,2.1,2.0,1.9,1.8,1.7,1.7,1.7},
 {3.4,3.3,3.2,3.1,3.0,2.9,2.8,2.7,2.6,2.5,2.4,2.3,2.2,2.1,2.0,2.0},
 {3.8,3.7,3.6,3.5,3.4,3.3,3.2,3.1,3.0,2.9,2.8,2.7,2.6,2.5,2.4,2.4},
 {4.2,4.1,4.0,3.9,3.8,3.7,3.6,3.5,3.4,3.3,3.2,3.1,3.0,2.9,2.8,2.8},
 {4.6,4.5,4.4,4.3,4.2,4.1,4.0,3.9,3.8,3.7,3.6,3.5,3.4,3.3,3.2,3.2},
 {5.0,4.9,4.8,4.7,4.6,4.5,4.4,4.3,4.2,4.1,4.0,3.9,3.8,3.7,3.6,3.6},
 {5.4,5.3,5.2,5.1,5.0,4.9,4.8,4.7,4.6,4.5,4.4,4.3,4.2,4.1,4.0,4.0},
 {5.8,5.7,5.6,5.5,5.4,5.3,5.2,5.1,5.0,4.9,4.8,4.7,4.6,4.5,4.4,4.4},
 {6.2,6.1,6.0,5.9,5.8,5.7,5.6,5.5,5.4,5.3,5.2,5.1,5.0,4.9,4.8,4.8},
 {6.6,6.5,6.4,6.3,6.2,6.1,6.0,5.9,5.8,5.7,5.6,5.5,5.4,5.3,5.2,5.2},
 {7.0,6.9,6.8,6.7,6.6,6.5,6.4,6.3,6.2,6.1,6.0,5.9,5.8,5.7,5.6,5.6}
};

// ================= INTERPOLAÇÃO =================
float interp2D(int rpmVal, float mapVal) {
  byte iR = 0, iM = 0;

  while (iR < 15 && rpmVal > rpmAxis[iR + 1]) iR++;
  while (iM < 15 && mapVal < mapAxis[iM + 1]) iM++;

  float r1 = rpmAxis[iR];
  float r2 = rpmAxis[iR + 1];
  float m1 = mapAxis[iM];
  float m2 = mapAxis[iM + 1];

  float q11 = injTable[iR][iM];
  float q21 = injTable[iR + 1][iM];
  float q12 = injTable[iR][iM + 1];
  float q22 = injTable[iR + 1][iM + 1];

  float fr = (rpmVal - r1) / (r2 - r1);
  float fm = (mapVal - m1) / (m2 - m1);

  float a = q11 + fr * (q21 - q11);
  float b = q12 + fr * (q22 - q12);

  return a + fm * (b - a);
}

// ================= ISR =================
void rpmISR() {
  unsigned long now = micros();
  periodMicros = now - lastPulseMicros;
  lastPulseMicros = now;
}

void setup() {
  lcd.begin(16, 2);
  pinMode(rpmPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(rpmPin), rpmISR, RISING);
}

void loop() {
  // RPM
  if (periodMicros > 0)
    rpm = 60000000UL / (periodMicros * pulsesPerRev);

  // MAP filtrado
  mapBuffer[mapIndex++] = analogRead(mapPin);
  if (mapIndex >= MAP_SAMPLES) mapIndex = 0;

  long sum = 0;
  for (byte i = 0; i < MAP_SAMPLES; i++) sum += mapBuffer[i];
  int mapADC = sum / MAP_SAMPLES;

  if (mapADC <= adc_1V) mapBar = 0.0;
  else if (mapADC >= adc_5V) mapBar = -1.0;
  else mapBar = - (float)(mapADC - adc_1V) / (adc_5V - adc_1V);

  // TABELA
  Tinj = interp2D(rpm, mapBar);

  // LCD
  static unsigned long tLCD = 0;
  if (millis() - tLCD > 100) {
    lcd.setCursor(0, 0);
    lcd.print("RPM:    ");
    lcd.setCursor(4, 0);
    lcd.print(rpm);

    lcd.setCursor(0, 1);
    lcd.print("MAP:     ");
    lcd.setCursor(4, 1);
    lcd.print(mapBar, 2);
    lcd.setCursor(10, 1);
    lcd.print("T:");
    lcd.print(Tinj, 2);

    tLCD = millis();
  }
}
