#include <LiquidCrystal.h>
#include <EEPROM.h>

// ================= LCD =================
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// ================= ESTADOS DO MENU =================
enum MenuState { MENU_PRINCIPAL, MONITORAMENTO, MAPA_INJ, MAPA_IGN, FUNCOES, CONFIGURACAO };
MenuState estadoAtual = MENU_PRINCIPAL;
int menuCursor = 0;
const int totalMenus = 5;
String nomesMenus[] = {"MONITORAMENTO", "MAPA INJ", "MAPA IGN", "FUNCOES", "CONFIGURACAO"};

// Variáveis de Edição e Aceleração
int editR = 0, editM = 0; 
byte campoFoco = 0; 
bool modoConfirmacao = false;
int selecaoConfirmar = 0; 
unsigned long tempoBotaoRetido = 0;
int intervaloAceleracao = 250; 
unsigned long lastBtnPress = 0; // Para substituir os delays bloqueantes

// ================= RPM / RODA FÔNICA =================
const byte rpmPin = 21;
const byte pulsesPerRev = 60;
const byte INJ_TOOTH = 21;
volatile unsigned long lastPulseMicros = 0, periodMicros = 0, avgPeriod = 0;
volatile byte toothCount = 0;
volatile bool syncOK = false;
volatile unsigned long lastPmsMicros = 0;
unsigned int rpm = 0;

// ================= SENSORES =================
const byte mapPin = A4;
const byte tpsPin = A3; 
const int adc_1V = 205, adc_5V = 1023;
const int tpsMinADC = 100, tpsMaxADC = 900;
float mapBar = 0.0, tpsPercent = 0.0;

// ================= INJETOR =================
#define INJ_PIN 22
volatile bool injectorOn = false;
volatile unsigned int injPulseTicksLatched = 0;
float Tinj_latched = 0.0, lastValidTinj = 1.5;

// ================= EIXOS E TABELA =================
const int rpmAxis[16] = {500, 800, 1200, 1600, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500};
const float mapAxis[16] = {0.0,-0.07,-0.14,-0.21,-0.28,-0.35,-0.42,-0.49,-0.56,-0.63,-0.70,-0.77,-0.84,-0.91,-0.96,-1.0};
float injTable[16][16]; 

// ================= FUNÇÕES AUXILIARES =================
int lerBotao() {
  int val = analogRead(0);
  if (val < 50)   return 1; // RIGHT
  if (val < 150)  return 2; // UP
  if (val < 350)  return 3; // DOWN
  if (val < 500)  return 4; // LEFT
  if (val < 750)  return 5; // SELECT
  return 0;
}

void salvarTabela() {
  int addr = 0;
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 16; j++) {
      EEPROM.put(addr, injTable[i][j]);
      addr += sizeof(float);
    }
  }
}

void carregarTabela() {
  float val;
  EEPROM.get(0, val);
  if (!isnan(val) && val > 0.1) {
    int addr = 0;
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 16; j++) {
        EEPROM.get(addr, injTable[i][j]);
        addr += sizeof(float);
      }
    }
  } else {
    for(int i=0; i<16; i++) for(int j=0; j<16; j++) injTable[i][j] = 1.5;
  }
}

float interp2D(int rpmVal, float mapVal) {
  byte iR = 0, iM = 0;
  while (iR < 14 && rpmVal > rpmAxis[iR + 1]) iR++;
  while (iM < 14 && mapVal < mapAxis[iM + 1]) iM++;
  float fr = (float)(rpmVal - rpmAxis[iR]) / (rpmAxis[iR + 1] - rpmAxis[iR]);
  float fm = (float)(mapVal - mapAxis[iM]) / (mapAxis[iM + 1] - mapAxis[iM]);
  float a = injTable[iR][iM] + fr * (injTable[iR + 1][iM] - injTable[iR][iM]);
  float b = injTable[iR][iM + 1] + fr * (injTable[iR + 1][iM + 1] - injTable[iR][iM + 1]);
  return a + fm * (b - a);
}

void rpmISR() {
  unsigned long now = micros();
  unsigned long p = now - lastPulseMicros;
  lastPulseMicros = now;
  if (avgPeriod == 0) avgPeriod = p;
  else avgPeriod = (avgPeriod * 7 + p) / 8;
  if (p > avgPeriod * 1.8) { toothCount = 0; syncOK = true; }
  else if (syncOK) { toothCount++; if (toothCount >= pulsesPerRev) toothCount = 0; }
  if (syncOK && toothCount == INJ_TOOTH) lastPmsMicros = now;
  periodMicros = p;
}

ISR(TIMER3_COMPA_vect) {
  digitalWrite(INJ_PIN, LOW);
  injectorOn = false;
  TIMSK3 &= ~(1 << OCIE3A);
}

// ================= SETUP =================
void setup() {
  lcd.begin(16, 2);
  Serial.begin(115200);
  carregarTabela();
  lcd.setCursor(0, 0); lcd.print("HimuroPerformance");
  lcd.setCursor(0, 1); lcd.print("Density EFI v1.0");
  
  // Substituído delay(2000) por um loop que não trava o sistema se necessário, 
  // mas como é no setup (motor desligado), o delay aqui é aceitável.
  delay(2000); 
  lcd.clear();
  
  pinMode(rpmPin, INPUT_PULLUP); pinMode(INJ_PIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(rpmPin), rpmISR, RISING);
  TCCR3A = 0; TCCR3B = (1 << WGM32) | (1 << CS31);
}

// ================= LOOP PRINCIPAL =================
void loop() {
  // --- Lógica de Sensores e Injeção (Sempre Rodando) ---
  tpsPercent = constrain((float)(analogRead(tpsPin) - tpsMinADC) * 100.0 / (tpsMaxADC - tpsMinADC), 0, 100);
  int mapADC = analogRead(mapPin);
  mapBar = (mapADC <= adc_1V) ? 0.0 : (mapADC >= adc_5V) ? -1.0 : -(float)(mapADC - adc_1V) / (adc_5V - adc_1V);

  unsigned long p;
  noInterrupts(); p = periodMicros; interrupts();
  if (p > 0 && (micros() - lastPulseMicros) < 300000) rpm = 60000000UL / (p * pulsesPerRev);
  else { rpm = 0; syncOK = false; }

  if (rpm > 400 && syncOK) lastValidTinj = interp2D(rpm, mapBar);
  Tinj_latched = lastValidTinj;
  injPulseTicksLatched = (unsigned int)(Tinj_latched * 2000.0);

  static unsigned long lastInj = 0;
  if (!injectorOn && rpm > 0) {
    unsigned long interval = 60000000UL / rpm;
    if (micros() - lastInj >= interval) {
      lastInj = micros();
      if (injPulseTicksLatched > 50) {
        digitalWrite(INJ_PIN, HIGH); injectorOn = true;
        TCNT3 = 0; OCR3A = injPulseTicksLatched;
        TIFR3 |= (1 << OCF3A); TIMSK3 |= (1 << OCIE3A);
      }
    }
  }

  // --- Serial para Dashboard ---
  static unsigned long tSer = 0;
  if (millis() - tSer > 100) {
    Serial.print(rpm); Serial.print(","); Serial.print(mapBar); Serial.print(",");
    Serial.print(tpsPercent); Serial.print(","); Serial.println(Tinj_latched);
    tSer = millis();
  }

  // --- Interface LCD (Sem Delays Bloqueantes) ---
  static unsigned long tLCD = 0;
  static unsigned long tBotaoAcel = 0; 

  if (millis() - tLCD > 80) { 
    int btn = lerBotao();
    bool blinkState = (millis() / 400) % 2;
    unsigned long m = millis();

    if (estadoAtual == MENU_PRINCIPAL) {
      if (m - lastBtnPress > 200) { // Debounce não bloqueante
        if (btn == 2) { menuCursor = (menuCursor - 1 + totalMenus) % totalMenus; lcd.clear(); lastBtnPress = m; }
        if (btn == 3) { menuCursor = (menuCursor + 1) % totalMenus; lcd.clear(); lastBtnPress = m; }
        if (btn == 5) { estadoAtual = (MenuState)(menuCursor + 1); lcd.clear(); lastBtnPress = m; }
      }
      
      lcd.setCursor(0, 0); lcd.print("> MENU PRINCIPAL ");
      lcd.setCursor(0, 1); 
      lcd.print("["); lcd.print(nomesMenus[menuCursor]); lcd.print("]");
      for (int i = nomesMenus[menuCursor].length() + 2; i < 16; i++) lcd.print(" ");
    } 
    else if (estadoAtual == MONITORAMENTO) {
      if (btn == 4 && (m - lastBtnPress > 200)) { estadoAtual = MENU_PRINCIPAL; lcd.clear(); lastBtnPress = m; }
      
      lcd.setCursor(0, 0); lcd.print("R:"); lcd.print(rpm); lcd.print("    ");
      lcd.setCursor(8, 0); lcd.print("T:"); lcd.print((int)tpsPercent); lcd.print("%     ");
      lcd.setCursor(0, 1); lcd.print("M:"); lcd.print(mapBar, 2); lcd.print("   ");
      lcd.setCursor(8, 1); lcd.print(Tinj_latched, 2); lcd.print("ms ");
    } 
    else if (estadoAtual == MAPA_INJ) {
      if (!modoConfirmacao) {
        // Lógica de Edição com Aceleração (Já é semi-não bloqueante, mas ajustada)
        if (btn == 2 || btn == 3) {
          if (tempoBotaoRetido == 0) {
            tempoBotaoRetido = m;
            intervaloAceleracao = 300;
            tBotaoAcel = 0;
          }
          if (m - tBotaoAcel > (unsigned long)intervaloAceleracao) {
            tBotaoAcel = m;
            if (m - tempoBotaoRetido > 800) intervaloAceleracao = 40; 
            else if (m - tempoBotaoRetido > 400) intervaloAceleracao = 150;

            if (campoFoco == 0) { 
               if(btn == 2) editR = (editR + 1) % 16;
               if(btn == 3) editR = (editR + 15) % 16;
            } else if (campoFoco == 1) { 
               if(btn == 2) editM = (editM + 1) % 16;
               if(btn == 3) editM = (editM + 15) % 16;
            } else if (campoFoco == 2) { 
               if(btn == 2) injTable[editR][editM] += 0.01;
               if(btn == 3) injTable[editR][editM] -= 0.01;
               injTable[editR][editM] = constrain(injTable[editR][editM], 0.1, 20.0);
            }
          }
        } else { tempoBotaoRetido = 0; }

        // Navegação de Campos (RIGHT/LEFT/SELECT)
        if (m - lastBtnPress > 200) {
          if (btn == 1) { campoFoco = (campoFoco + 1) % 3; lastBtnPress = m; }
          if (btn == 4) { campoFoco = (campoFoco - 1 + 3) % 3; lastBtnPress = m; }
          if (btn == 5) { modoConfirmacao = true; lcd.clear(); lastBtnPress = m; }
        }

        // --- Impressão LCD ---
        lcd.setCursor(0,0);
        if (campoFoco == 0 && blinkState) lcd.print("R:    "); 
        else { lcd.print("R: "); lcd.print(rpmAxis[editR]); }
        lcd.print("     ");

        lcd.setCursor(8,0);
        if (campoFoco == 1 && blinkState) lcd.print("M:    "); 
        else { lcd.print("M:"); lcd.print(mapAxis[editM],2); }
        lcd.print("    ");

        lcd.setCursor(0,1);
        if (campoFoco == 2 && blinkState) lcd.print("T.Inj:       "); 
        else { 
            lcd.print("T.Inj: "); 
            lcd.print(injTable[editR][editM],2); 
            lcd.print("ms"); 
        }
        lcd.print("    ");

      } else {
        // Modo Confirmação
        if (m - lastBtnPress > 200) {
          if (btn == 2 || btn == 3) { selecaoConfirmar = !selecaoConfirmar; lastBtnPress = m; }
          if (btn == 5) { 
            if(selecaoConfirmar == 0) salvarTabela(); 
            modoConfirmacao = false; estadoAtual = MENU_PRINCIPAL; lcd.clear(); lastBtnPress = m;
          }
        }
        lcd.setCursor(0,0); lcd.print("Deseja Salvar?  ");
        lcd.setCursor(0,1); 
        lcd.print(selecaoConfirmar == 0 ? ">SIM    " : " SIM    "); 
        lcd.print(selecaoConfirmar == 1 ? ">NAO   " : " NAO   ");
      }
    } else {
      if (btn == 4 && (m - lastBtnPress > 200)) { estadoAtual = MENU_PRINCIPAL; lcd.clear(); lastBtnPress = m; }
      lcd.setCursor(0,0); lcd.print(nomesMenus[menuCursor]); lcd.print("           ");
      lcd.setCursor(0,1); lcd.print("EM DESENVOLVIM. ");
    }
    tLCD = m;
  }
}