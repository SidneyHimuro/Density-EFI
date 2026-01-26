#include <LiquidCrystal.h>

// ================= LCD =================
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// ================= RPM / RODA FÔNICA =================
const byte rpmPin = 21;
const byte pulsesPerRev = 60;      // 60-2
const byte INJ_TOOTH = 21;         // dente PMS de disparo

volatile unsigned long lastPulseMicros = 0;
volatile unsigned long periodMicros = 0;
volatile unsigned long lastPeriod = 0;
volatile byte toothCount = 0;

volatile bool syncOK = false;
volatile bool pmsEvent = false;

unsigned int rpm = 0;

// ================= MAP =================
const byte mapPin = A4;
const int adc_1V = 205;
const int adc_5V = 1023;

float mapBar = 0.0;

// ================= TPS =================
const byte tpsPin = A5;
const int tpsMinADC = 100;   // ajuste conforme seu sensor
const int tpsMaxADC = 900;   // ajuste conforme seu sensor
float tpsPercent = 0.0;


// ================= INJETOR =================
#define INJ_PIN 22

volatile bool injectorOn = false;
volatile unsigned int injPulseTicksLatched = 0;

float Tinj_calc = 0.0;
float Tinj_latched = 0.0;

// ================= EIXOS =================
const int rpmAxis[16] = {
   500, 800, 1200, 1600, 2000, 2500, 3000, 3500,
  4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500
};

const float mapAxis[16] = {
   0.0,-0.07,-0.14,-0.21,-0.28,-0.35,-0.42,-0.49,
  -0.56,-0.63,-0.70,-0.77,-0.84,-0.91,-0.96,-1.0
};

// ================= TABELA =================
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

  while (iR < 14 && rpmVal > rpmAxis[iR + 1]) iR++;
  while (iM < 14 && mapVal < mapAxis[iM + 1]) iM++;

  float fr = (float)(rpmVal - rpmAxis[iR]) / (rpmAxis[iR + 1] - rpmAxis[iR]);
  float fm = (float)(mapVal - mapAxis[iM]) / (mapAxis[iM + 1] - mapAxis[iM]);

  float a = injTable[iR][iM]     + fr * (injTable[iR + 1][iM]     - injTable[iR][iM]);
  float b = injTable[iR][iM + 1] + fr * (injTable[iR + 1][iM + 1] - injTable[iR][iM + 1]);

  return a + fm * (b - a);
}

// ================= RPM ISR =================
void rpmISR() {
  unsigned long now = micros();
  unsigned long p = now - lastPulseMicros;
  lastPulseMicros = now;

  if (p > (lastPeriod * 3) / 2) {
    toothCount = 0;
    syncOK = true;
  } else {
    toothCount++;
    if (toothCount >= pulsesPerRev) toothCount = 0;
  }

  lastPeriod = p;
  periodMicros = p;

  if (syncOK && toothCount == INJ_TOOTH) {
    pmsEvent = true;
  }
}

// ================= TIMER3 ISR =================
ISR(TIMER3_COMPA_vect) {
  digitalWrite(INJ_PIN, LOW);
  injectorOn = false;
  TIMSK3 &= ~(1 << OCIE3A);
}

// ================= SETUP =================
void setup() {
  lcd.begin(16, 2);
  Serial.begin(115200);

  pinMode(rpmPin, INPUT_PULLUP);
  pinMode(INJ_PIN, OUTPUT);
  digitalWrite(INJ_PIN, LOW);

  attachInterrupt(digitalPinToInterrupt(rpmPin), rpmISR, RISING);

  TCCR3A = 0;
  TCCR3B = (1 << WGM32) | (1 << CS31);  // prescaler 8
}

// ================= LOOP =================
void loop() {

  // TPS
  int tpsADC = analogRead(tpsPin);
  if (tpsADC <= tpsMinADC) tpsPercent = 0.0;
  else if (tpsADC >= tpsMaxADC) tpsPercent = 100.0;
  else {
    tpsPercent = (float)(tpsADC - tpsMinADC) * 100.0 /
                 (tpsMaxADC - tpsMinADC);
  }


  // ---------- RPM ----------
  static unsigned long rpmFilt = 0;
  if (periodMicros > 0 && (micros() - lastPulseMicros) < 300000) {
    unsigned long rpmNow = 60000000UL / (periodMicros * pulsesPerRev);
    rpmFilt = (rpmFilt * 3 + rpmNow) / 4;
    rpm = rpmFilt;
  } else {
    rpm = 0;
    syncOK = false;
  }

  // ---------- PMS EVENT ----------
  if (pmsEvent && syncOK && rpm > 0) {
    pmsEvent = false;

    int mapADC = analogRead(mapPin);
    if (mapADC <= adc_1V) mapBar = 0.0;
    else if (mapADC >= adc_5V) mapBar = -1.0;
    else mapBar = -(float)(mapADC - adc_1V) / (adc_5V - adc_1V);

    Tinj_calc = interp2D(rpm, mapBar);
    Tinj_latched = Tinj_calc;

    injPulseTicksLatched = (unsigned int)(Tinj_latched * 2000.0);

    if (injPulseTicksLatched > 50) {
      digitalWrite(INJ_PIN, HIGH);
      injectorOn = true;

      TCNT3 = 0;
      OCR3A = injPulseTicksLatched;
      TIFR3 |= (1 << OCF3A);
      TIMSK3 |= (1 << OCIE3A);
    }

    Serial.print("RPM ");
    Serial.print(rpm);
    Serial.print(" | MAP ");
    Serial.print(mapBar, 2);
    Serial.print(" | TPS ");
    Serial.println((int)tpsPercent);
    Serial.print(" | Tinj ");
    Serial.println(Tinj_latched, 3);
    
  }

  // ---------- LCD ----------
  static unsigned long tLCD = 0;
  if (millis() - tLCD > 20) {
    lcd.setCursor(0, 0);
    lcd.print("RPM:     ");
    lcd.setCursor(4, 0);
    lcd.print(rpm);

    lcd.setCursor(0, 1);
    lcd.print("MAP:     ");
    lcd.setCursor(4, 1);
    lcd.print(mapBar, 2);

    lcd.setCursor(10, 1);
    lcd.print("T:");
    lcd.print(Tinj_latched, 2);

    lcd.setCursor(9,0); lcd.print("TPS:    ");
    lcd.setCursor(14, 0);
    lcd.print((int)tpsPercent);

    tLCD = millis();
  }
}
