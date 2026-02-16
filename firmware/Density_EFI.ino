#include <LiquidCrystal.h>
#include <EEPROM.h>
#include "Crank.h"
#include "Injector.h"
#include "IgnitionControl.h"

// ================= LCD =================
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// ================= ESTADOS DO MENU =================
enum MenuState { MENU_PRINCIPAL, MONITORAMENTO, MAPA_INJ, MAPA_IGN, FUNCOES, CONFIGURACAO };
MenuState estadoAtual = MENU_PRINCIPAL;
int menuCursor = 0;
const int totalMenus = 5;
String nomesMenus[] = {"MONITORAMENTO ", "MAPA INJCAO     ", "MAPA IGNICAO     ", "FUNCOES         ", "CONFIGURACAO  "};

// ================= VARIÁVEIS DE EDIÇÃO =================
int editR = 0, editM = 0;
byte campoFoco = 0;
bool modoConfirmacao = false;
int selecaoConfirmar = 0;
unsigned long tempoBotaoRetido = 0;
int intervaloAceleracao = 250;
unsigned long lastBtnPress = 0;
unsigned long tBotaoAcel = 0;

// ================= SENSORES =================
const byte mapPin = A4;
const byte tpsPin = A3;
int tpsMinADC = 100, tpsMaxADC = 900;
int mapAtmosADC = 940;
float mapBar = 0.0, tpsPercent = 0.0;

// ================= SENSOR DE BATERIA =================
const byte batPin = A5;
float tensaoBateria = 0.0;
const float R1 = 10000.0;
const float R2 = 3300.0;
const float fatorDivisor = (R1 + R2) / R2;

// ================= IGNIÇÃO OFFSET =================
float ignOffsetDegrees = 0.0;
const int addrIgnOffset = 2100;

// ================= SINAL ROTACAO =================
byte denteSincro = 0;
const int addrSyncTooth = 1058;

// ================= NAVEGAÇÃO SUBMENUS =================
byte etapaConfig = 0;
int subMenuCursor = 0;
const int totalSubMenus = 4;
String nomesSub[] = {"CALIBRAR TPS     ", "CALIBRAR MAP     ", "SINAL ROTACAO    ", "OFFSET IGNICAO   "};

byte etapaFuncoes = 0;
int subMenuFuncoesCursor = 0;
const int totalSubFuncoes = 3;
String nomesSubFuncoes[] = {"ACEL. RAPIDA    ", "DWELL BOBINA     ", "VENTILADOR       "};

// ================= EIXOS E TABELAS =================
const int rpmAxis[16] = {500, 800, 1200, 1600, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500};
const float mapAxis[16] = {0.0, -0.07, -0.14, -0.21, -0.28, -0.35, -0.42, -0.49, -0.56, -0.63, -0.70, -0.77, -0.84, -0.91, -0.96, -1.0};
float injTable[16][16];
float ignTable[16][16];

// ================= ENDEREÇOS EEPROM =================
const int addrIgnTableStart = 1500;
const int addrTPSMin = 1030, addrTPSMax = 1034, addrMAPAtmos = 1038;
const int addrSinalRPM = 1042, addrAEMax = 1046, addrAEDecay = 1050;
const int addrIgnDwell = 1054;

// ================= CONFIGURAÇÃO IGNIÇÃO =================
void configureIgnitionMode() {
    setWastedSparkMode(pulsesPerRev == 60);
}

void salvarIgnicaoConfig() {
    EEPROM.put(addrIgnOffset, ignOffsetDegrees);
}

void carregarIgnicaoConfig() {
    EEPROM.get(addrIgnOffset, ignOffsetDegrees);
    if (isnan(ignOffsetDegrees) || ignOffsetDegrees < -10.0 || ignOffsetDegrees > 10.0) {
        ignOffsetDegrees = 0.0;
    }
}

// ================= FUNÇÃO PARA LER BATERIA =================
float lerTensaoBateria() {
    int batADC = analogRead(batPin);
    return (batADC * 5.0 / 1024.0) * fatorDivisor;
}

// ================= SENSOR DE TEMPERATURA ECT =================
const byte ectPin = A1;  // Pino para sensor de temperatura do motor
float temperaturaECT = 0.0;  // Temperatura em °C

// Tabela genérica para sensor NTC (ex: 10kΩ @ 25°C)
// Valores aproximados para referência (ajuste conforme seu sensor)
const float tempTable[11][2] = {
  { 0, 32500 },   // 0°C -> 32.5kΩ
  { 10, 18000 },  // 10°C -> 18kΩ
  { 20, 11000 },  // 20°C -> 11kΩ
  { 30, 7000 },   // 30°C -> 7kΩ
  { 40, 5000 },   // 40°C -> 5kΩ
  { 50, 3500 },   // 50°C -> 3.5kΩ
  { 60, 2500 },   // 60°C -> 2.5kΩ
  { 70, 1800 },   // 70°C -> 1.8kΩ
  { 80, 1300 },   // 80°C -> 1.3kΩ
  { 90, 1000 },   // 90°C -> 1kΩ
  { 100, 800 }    // 100°C -> 800Ω
};

// ================= CONTROLE DO ELETROVENTILADOR =================
const byte fanPin = 47;  // Pino para acionamento do eletroventilador
bool fanAuto = true;     // Modo automático (true) ou manual (false)
bool fanManualState = false;  // Estado manual do ventilador
int fanTempOn = 95;      // Temperatura para ligar (°C)
int fanTempOff = 90;     // Temperatura para desligar (°C)
int fanHysteresis = 5;   // Histérese padrão

// Endereços EEPROM para o ventilador (use endereços livres)
const int addrFanAuto = 1060;
const int addrFanTempOn = 1064;
const int addrFanTempOff = 1068;

// ================= FUNÇÕES DWELL =================
void salvarDwellConfig() {
    EEPROM.put(addrIgnDwell, getIgnitionDwell());
}

void carregarDwellConfig() {
    float dwell;
    EEPROM.get(addrIgnDwell, dwell);
    if (!isnan(dwell) && dwell >= 1.0 && dwell <= 5.0) {
        setIgnitionDwell(dwell);
    }
}

// ================= FUNÇÕES DE APOIO =================
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
    EEPROM.put(addrAEMax, AE_TPS_max);
    EEPROM.put(addrAEDecay, AE_decay_ms);
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
        EEPROM.get(addrTPSMin, tpsMinADC);
        EEPROM.get(addrTPSMax, tpsMaxADC);
        EEPROM.get(addrMAPAtmos, mapAtmosADC);
        EEPROM.get(addrSinalRPM, pulsesPerRev);
        EEPROM.get(addrAEMax, AE_TPS_max);
        EEPROM.get(addrAEDecay, AE_decay_ms);
    } else {
        for (int i = 0; i < 16; i++)
            for (int j = 0; j < 16; j++)
                injTable[i][j] = 1.5;
    }
}

// ================= FUNÇÕES IGNIÇÃO =================
void salvarTabelaIgnicao() {
    int addr = addrIgnTableStart;
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            EEPROM.put(addr, ignTable[i][j]);
            addr += sizeof(float);
        }
    }
}

void carregarTabelaIgnicao() {
    float val;
    EEPROM.get(addrIgnTableStart, val);
    if (!isnan(val) && val >= 5.0 && val <= 40.0) {
        int addr = addrIgnTableStart;
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 16; j++) {
                EEPROM.get(addr, ignTable[i][j]);
                addr += sizeof(float);
            }
        }
    } else {
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 16; j++) {
                if (rpmAxis[i] < 2000) {
                    ignTable[i][j] = 10.0 + (j * 0.5);
                } else if (rpmAxis[i] < 5000) {
                    ignTable[i][j] = 28.0 + (j * 0.3);
                } else {
                    ignTable[i][j] = 24.0 + (j * 0.2);
                }
            }
        }
        salvarTabelaIgnicao();
    }
}

float interp2DIgn(int rpmVal, float mapVal) {
    byte iR = 0, iM = 0;
    while (iR < 14 && rpmVal > rpmAxis[iR + 1]) iR++;
    while (iM < 14 && mapVal < mapAxis[iM + 1]) iM++;

    float fr = (float)(rpmVal - rpmAxis[iR]) / (rpmAxis[iR + 1] - rpmAxis[iR]);
    float fm = (float)(mapVal - mapAxis[iM]) / (mapAxis[iM + 1] - mapAxis[iM]);

    float a = ignTable[iR][iM] + fr * (ignTable[iR + 1][iM] - ignTable[iR][iM]);
    float b = ignTable[iR][iM + 1] + fr * (ignTable[iR + 1][iM + 1] - ignTable[iR][iM + 1]);

    return a + fm * (b - a) + ignOffsetDegrees;
}

void salvarTabelasCompletas() {
    salvarTabela();
    salvarTabelaIgnicao();
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

void salvarSyncTooth() {
    EEPROM.put(addrSyncTooth, denteSincro);
}

void carregarSyncTooth() {
    byte val;
    EEPROM.get(addrSyncTooth, val);
    denteSincro = (val >= 0 && val < 60) ? val : 0;
}

// ================= FUNÇÃO PARA LER TEMPERATURA ECT =================
float lerTemperaturaECT() {
  int ectADC = analogRead(ectPin);
  float tensao = (ectADC * 5.0) / 1024.0;
  
  // Para sensor NTC com resistor pull-up de 10kΩ
  // Resistência do sensor = (tensão / (5 - tensão)) * 10kΩ
  if (tensao >= 4.99) return -40;  // Sensor aberto ou desconectado
  if (tensao <= 0.01) return 140;  // Curto ou temperatura muito alta
  
  float resistencia = (tensao / (5.0 - tensao)) * 10000.0;
  
  // Interpolação na tabela
  if (resistencia >= tempTable[0][1]) return tempTable[0][0];
  if (resistencia <= tempTable[10][1]) return tempTable[10][0];
  
  for (int i = 0; i < 10; i++) {
    if (resistencia <= tempTable[i][1] && resistencia >= tempTable[i+1][1]) {
      float t1 = tempTable[i][0];
      float t2 = tempTable[i+1][0];
      float r1 = tempTable[i][1];
      float r2 = tempTable[i+1][1];
      
      return t1 + (t2 - t1) * (r1 - resistencia) / (r1 - r2);
    }
  }
  
  return 85;  // Valor padrão se não encontrar
}

// ================= FUNÇÕES DO ELETROVENTILADOR =================
void setupFan() {
  pinMode(fanPin, OUTPUT);
  digitalWrite(fanPin, LOW);
}

void updateFan(float temperatura) {
  if (fanAuto) {
    // Modo automático baseado na temperatura
    if (temperatura >= fanTempOn && !digitalRead(fanPin)) {
      digitalWrite(fanPin, HIGH);
      Serial.println("Ventilador LIGADO (automático)");
    } else if (temperatura <= fanTempOff && digitalRead(fanPin)) {
      digitalWrite(fanPin, LOW);
      Serial.println("Ventilador DESLIGADO (automático)");
    }
  } else {
    // Modo manual
    digitalWrite(fanPin, fanManualState ? HIGH : LOW);
  }
}

void setFanAuto(bool autoMode) {
  fanAuto = autoMode;
  if (!fanAuto) {
    fanManualState = digitalRead(fanPin); // Sincroniza estado manual
  }
}

void setFanManual(bool state) {
  fanManualState = state;
  if (!fanAuto) {
    digitalWrite(fanPin, state ? HIGH : LOW);
  }
}

void salvarFanConfig() {
  EEPROM.put(addrFanAuto, fanAuto);
  EEPROM.put(addrFanTempOn, fanTempOn);
  EEPROM.put(addrFanTempOff, fanTempOff);
}

void carregarFanConfig() {
  EEPROM.get(addrFanAuto, fanAuto);
  EEPROM.get(addrFanTempOn, fanTempOn);
  EEPROM.get(addrFanTempOff, fanTempOff);
  
  // Validação
  if (fanTempOn < 50 || fanTempOn > 120) fanTempOn = 95;
  if (fanTempOff < 45 || fanTempOff > 115) fanTempOff = 90;
  if (fanTempOff >= fanTempOn) fanTempOff = fanTempOn - fanHysteresis;
}

// ================= SETUP =================
void setup() {
    lcd.begin(16, 2);
    Serial.begin(115200);

    carregarTabela();
    carregarTabelaIgnicao();
    carregarIgnicaoConfig();
    carregarDwellConfig();
    carregarSyncTooth();
    carregarFanConfig();
    
    setupCrank();
    setupInjector();
    setupIgnition();
    setupFan();
    configureIgnitionMode();

    lcd.print("HimuroPerform.");
    lcd.setCursor(0, 1);
    lcd.print("Density EFI v1.1");
    delay(2000);
    lcd.clear();
}

// ================= LOOP PRINCIPAL =================
void loop() {
    // 1. Leituras de Sensores
    int mapADC = analogRead(mapPin);
    int tpsADC = analogRead(tpsPin);
    temperaturaECT = lerTemperaturaECT();

    float kpaFaltante = (float)(mapAtmosADC - mapADC) / 10.24;
    mapBar = -(kpaFaltante / 100.0);
    mapBar = constrain(mapBar, -1.0, 0.0);

    tpsPercent = constrain((float)(tpsADC - tpsMinADC) * 100.0 / (tpsMaxADC - tpsMinADC), 0, 100);

    // 2. Atualização de Estados
    updateCrankRPM();
    updateInjectorAE(tpsPercent, rpm);
    updateFan(temperaturaECT);  // <-- ADICIONAR

    // 3. Cálculo dos parâmetros de ignição e injeção
    if (rpm > 40 && syncOK) {
        Tinj_latched = interp2D(rpm, mapBar);
        setIgnitionAdvance(interp2DIgn(rpm, mapBar));
        updateIgnition(rpm, syncOK, pulsesPerRev, toothCount);
    } else {
        updateIgnition(0, false, pulsesPerRev, 0);
    }

    // 4. Verificação do dente de sincronismo (debug)
    static bool syncEvent = false;
    if (toothCount == denteSincro && !syncEvent) {
        syncEvent = true;
        Serial.println("Dente de sincronismo atingido!");
    } else if (toothCount != denteSincro) {
        syncEvent = false;
    }

    // 5. Execução da Injeção
    runInjector(rpm, mapBar, tpsPercent);

    // 6. Telemetria Serial - 7 valores para o Dash
    static unsigned long tSer = 0;
    if (millis() - tSer > 100) {
        tensaoBateria = lerTensaoBateria();

        Serial.print(rpm);                  Serial.print(",");
        Serial.print(mapBar, 3);             Serial.print(",");
        Serial.print(tpsPercent, 1);         Serial.print(",");
        Serial.print(Tinj_latched + AE_TPS, 2); Serial.print(",");
        Serial.print(getIgnitionAdvance(), 1);  Serial.print(",");
        Serial.print(getIgnitionDwell(), 1);    Serial.print(",");
        Serial.println(tensaoBateria, 1);

        tSer = millis();
    }

    // ================= MENU LCD =================
    static unsigned long tLCD = 0;
    if (millis() - tLCD > 80) {
        int btn = lerBotao();
        unsigned long m = millis();
        bool blinkState = (m / 400) % 2;

        if (estadoAtual == MENU_PRINCIPAL) {
            if (m - lastBtnPress > 200) {
                if (btn == 2) { menuCursor = (menuCursor - 1 + totalMenus) % totalMenus; lcd.clear(); lastBtnPress = m; }
                if (btn == 3) { menuCursor = (menuCursor + 1) % totalMenus; lcd.clear(); lastBtnPress = m; }
                if (btn == 5) {
                    estadoAtual = (MenuState)(menuCursor + 1);
                    etapaConfig = 0; etapaFuncoes = 0; subMenuCursor = 0;
                    lcd.clear(); lastBtnPress = m;
                }
            }
            int itemTopo = (menuCursor / 2) * 2;
            int itemBaixo = itemTopo + 1;
            lcd.setCursor(0, 0);
            lcd.print(menuCursor == itemTopo ? "> " : "  ");
            lcd.print(nomesMenus[itemTopo]);
            lcd.setCursor(0, 1);
            if (itemBaixo < totalMenus) {
                lcd.print(menuCursor == itemBaixo ? "> " : "  ");
                lcd.print(nomesMenus[itemBaixo]);
            } else {
                lcd.print("                ");
            }
        }

        else if (estadoAtual == MONITORAMENTO) {
    if (btn == 5 && (m - lastBtnPress > 200)) {
        estadoAtual = MENU_PRINCIPAL;
        lcd.clear();
        lastBtnPress = m;
    }

    static byte telaMonitor = 0;
    const byte totalTelas = 4;  // Agora são 4 telas

    if (btn == 1 && (m - lastBtnPress > 200)) {
        telaMonitor = (telaMonitor + 1) % totalTelas;
        lcd.clear();
        lastBtnPress = m;
    }

    if (btn == 4 && (m - lastBtnPress > 200)) {
        telaMonitor = (telaMonitor - 1 + totalTelas) % totalTelas;
        lcd.clear();
        lastBtnPress = m;
    }

    // TELA 0 - RPM, IGN, MAP, INJ
    if (telaMonitor == 0) {
        lcd.setCursor(0, 0); lcd.print("R:"); lcd.print(rpm); lcd.print("      ");
        lcd.setCursor(8, 0); lcd.print("I:"); lcd.print(getIgnitionAdvance(), 1); lcd.print((char)223);
        lcd.setCursor(0, 1); lcd.print("M:"); lcd.print(mapBar, 2); lcd.print("       ");
        lcd.setCursor(8, 1); lcd.print("F:"); lcd.print(Tinj_latched + AE_TPS, 2); lcd.print("ms     ");
    }
    // TELA 1 - Dwell e Ponto de ignição
    else if (telaMonitor == 1) {
        lcd.setCursor(0, 0); lcd.print("R:"); lcd.print(rpm); lcd.print("       ");
        lcd.setCursor(8, 0); lcd.print("D:"); lcd.print(getIgnitionDwell(), 1); lcd.print("ms     ");
        lcd.setCursor(0, 1); lcd.print("IG:"); lcd.print(getIgnitionAdvance(), 1); lcd.print((char)223); lcd.print("       ");
        lcd.setCursor(8, 1); lcd.print("PT:"); lcd.print(getIgnitionAdvance(), 1); lcd.print((char)223); lcd.print("      ");
    }
    // TELA 2 - Bateria e TPS
    else if (telaMonitor == 2) {
        int batADC = analogRead(A5);
        float tensaoBateria = (batADC * 5.0 / 1024.0) * ((10.0 + 3.3) / 3.3);
        lcd.setCursor(0, 0); lcd.print("R:"); lcd.print(rpm); lcd.print("       ");
        lcd.setCursor(8, 0); lcd.print("T:"); lcd.print((int)tpsPercent); lcd.print("%         ");
        lcd.setCursor(0, 1); lcd.print("V:"); lcd.print(tensaoBateria, 1); lcd.print("V     ");
        lcd.setCursor(8, 1); lcd.print("F:"); lcd.print(Tinj_latched + AE_TPS, 2); lcd.print("ms      ");
    }
    // TELA 3 - Temperatura e status do ventilador
    else if (telaMonitor == 3) {
        lcd.setCursor(0, 0); lcd.print("ECT:"); lcd.print((int)temperaturaECT); lcd.print((char)223); lcd.print("C  ");
        lcd.setCursor(8, 0); lcd.print("FAN:");
        if (digitalRead(fanPin)) {
            lcd.print("ON ");
        } else {
            lcd.print("OFF");
        }
        lcd.setCursor(0, 1); lcd.print("MODO:");
        lcd.print(fanAuto ? "AUTO " : "MANUAL");
        lcd.setCursor(8, 1); lcd.print("SET:");
        lcd.print(fanTempOn); lcd.print((char)223); lcd.print("C");
    }
}

else if (estadoAtual == FUNCOES) {
    if (btn == 4 && (m - lastBtnPress > 200)) {
        if (etapaFuncoes > 0) {
            etapaFuncoes = 0;
            lcd.clear();
        } else {
            estadoAtual = MENU_PRINCIPAL;
            lcd.clear();
        }
        lastBtnPress = m;
    }

    if (etapaFuncoes == 0) {
        // Navegação UP/DOWN para 3 opções
        if (btn == 2 && (m - lastBtnPress > 200)) { // UP
            subMenuFuncoesCursor = (subMenuFuncoesCursor > 0) ? subMenuFuncoesCursor - 1 : 0;
            lastBtnPress = m;
        }
        if (btn == 3 && (m - lastBtnPress > 200)) { // DOWN
            subMenuFuncoesCursor = (subMenuFuncoesCursor < 2) ? subMenuFuncoesCursor + 1 : 2;
            lastBtnPress = m;
        }
        if (btn == 5 && (m - lastBtnPress > 200)) { // SELECT
            if (subMenuFuncoesCursor == 0) etapaFuncoes = 1;      // ACEL. RAPIDA
            else if (subMenuFuncoesCursor == 1) etapaFuncoes = 10; // DWELL
            else if (subMenuFuncoesCursor == 2) etapaFuncoes = 11; // VENTILADOR
            lcd.clear();
            lastBtnPress = m;
        }

        // Redesenha menu
        lcd.setCursor(0, 0);
        if (subMenuFuncoesCursor == 0) {
            lcd.print("> ACEL. RAPIDA  ");
        } else if (subMenuFuncoesCursor == 1) {
            lcd.print("  ACEL. RAPIDA  ");
        } else {
            lcd.print("  ACEL. RAPIDA  ");
        }

        lcd.setCursor(0, 1);
        if (subMenuFuncoesCursor == 1) {
            lcd.print("> DWELL BOBINA  ");
        } else if (subMenuFuncoesCursor == 2) {
            lcd.print("> VENTILADOR    ");
        } else {
            lcd.print("  DWELL BOBINA  ");
        }
    }
    
    else if (etapaFuncoes == 1) { // ACEL. RAPIDA - GANHO AE
        if (btn == 2 && (m - lastBtnPress > 200)) { 
            AE_TPS_max = constrain(AE_TPS_max + 0.1, 0.0, 5.0); 
            lastBtnPress = m; 
        }
        if (btn == 3 && (m - lastBtnPress > 200)) { 
            AE_TPS_max = constrain(AE_TPS_max - 0.1, 0.0, 5.0); 
            lastBtnPress = m; 
        }
        if (btn == 5 && (m - lastBtnPress > 200)) { 
            etapaFuncoes = 2; 
            lcd.clear(); 
            lastBtnPress = m; 
        }

        lcd.setCursor(0, 0); 
        lcd.print("GANHO AE (ms)   ");
        lcd.setCursor(0, 1); 
        lcd.print("VALOR: "); 
        lcd.print(AE_TPS_max, 1); 
        lcd.print("   ");
    }
    
    else if (etapaFuncoes == 2) { // ACEL. RAPIDA - DECAIMENTO
        if (btn == 2 && (m - lastBtnPress > 200)) { 
            AE_decay_ms = constrain(AE_decay_ms + 10, 50, 1000); 
            lastBtnPress = m; 
        }
        if (btn == 3 && (m - lastBtnPress > 200)) { 
            AE_decay_ms = constrain(AE_decay_ms - 10, 50, 1000); 
            lastBtnPress = m; 
        }
        if (btn == 5 && (m - lastBtnPress > 200)) {
            EEPROM.put(addrAEMax, AE_TPS_max);
            EEPROM.put(addrAEDecay, AE_decay_ms);
            lcd.clear(); 
            lcd.print("AE SALVO!"); 
            delay(1000);
            etapaFuncoes = 0; 
            subMenuFuncoesCursor = 0; 
            lcd.clear(); 
            lastBtnPress = m;
        }

        lcd.setCursor(0, 0); 
        lcd.print("DECAIMENTO (ms) ");
        lcd.setCursor(0, 1); 
        lcd.print("VALOR: "); 
        lcd.print((int)AE_decay_ms); 
        lcd.print("   ");
    }
    
    else if (etapaFuncoes == 10) { // DWELL BOBINA
        if (btn == 2 && (m - lastBtnPress > 200)) { 
            setIgnitionDwell(getIgnitionDwell() + 0.1); 
            lastBtnPress = m; 
        }
        if (btn == 3 && (m - lastBtnPress > 200)) { 
            setIgnitionDwell(getIgnitionDwell() - 0.1); 
            lastBtnPress = m; 
        }
        if (btn == 5 && (m - lastBtnPress > 200)) {
            salvarDwellConfig();
            lcd.clear(); 
            lcd.print("DWELL SALVO!"); 
            delay(1000);
            etapaFuncoes = 0; 
            subMenuFuncoesCursor = 0; 
            lcd.clear(); 
            lastBtnPress = m;
        }

        lcd.setCursor(0, 0); 
        lcd.print("DWELL BOBINA    ");
        lcd.setCursor(0, 1); 
        lcd.print("VALOR: "); 
        lcd.print(getIgnitionDwell(), 1); 
        lcd.print("ms   ");
    }
    
    else if (etapaFuncoes == 11) { // VENTILADOR
        static byte subEtapaFan = 0;
        
        if (m - lastBtnPress > 200) {
            // Submenu do ventilador
            if (subEtapaFan == 0) { // MODO
                lcd.setCursor(0, 0); 
                lcd.print("VENTILADOR      ");
                lcd.setCursor(0, 1); 
                lcd.print("MODO: "); 
                lcd.print(fanAuto ? "AUTO " : "MANUAL");
                
                if (btn == 2 || btn == 3) {
                    fanAuto = !fanAuto;
                    setFanAuto(fanAuto);
                    lastBtnPress = m;
                }
                if (btn == 5) {
                    subEtapaFan = 1;
                    lcd.clear();
                    lastBtnPress = m;
                }
            }
            else if (subEtapaFan == 1) { // TEMP LIGAR
                lcd.setCursor(0, 0); 
                lcd.print("TEMP LIGAR     ");
                lcd.setCursor(0, 1); 
                lcd.print(fanTempOn); 
                lcd.print((char)223); 
                lcd.print("C   ");
                
                if (btn == 2) { 
                    fanTempOn = constrain(fanTempOn + 1, 50, 120); 
                    lastBtnPress = m; 
                }
                if (btn == 3) { 
                    fanTempOn = constrain(fanTempOn - 1, 50, 120); 
                    lastBtnPress = m; 
                }
                if (btn == 5) { 
                    subEtapaFan = 2; 
                    lcd.clear(); 
                    lastBtnPress = m; 
                }
            }
            else if (subEtapaFan == 2) { // TEMP DESLIGAR
                lcd.setCursor(0, 0); 
                lcd.print("TEMP DESLIGAR  ");
                lcd.setCursor(0, 1); 
                lcd.print(fanTempOff); 
                lcd.print((char)223); 
                lcd.print("C   ");
                
                if (btn == 2) { 
                    fanTempOff = constrain(fanTempOff + 1, 45, 115); 
                    lastBtnPress = m; 
                }
                if (btn == 3) { 
                    fanTempOff = constrain(fanTempOff - 1, 45, 115); 
                    lastBtnPress = m; 
                }
                if (fanTempOff >= fanTempOn) fanTempOff = fanTempOn - fanHysteresis;
                if (btn == 5) { 
                    if (!fanAuto) {
                        subEtapaFan = 3;  // Vai para controle manual
                    } else {
                        salvarFanConfig();
                        lcd.clear(); 
                        lcd.print("FAN SALVO!"); 
                        delay(1000);
                        etapaFuncoes = 0; 
                        subMenuFuncoesCursor = 0; 
                        lcd.clear();
                    }
                    lastBtnPress = m;
                }
            }
            else if (subEtapaFan == 3 && !fanAuto) { // MANUAL
                lcd.setCursor(0, 0); 
                lcd.print("MANUAL         ");
                lcd.setCursor(0, 1); 
                lcd.print(fanManualState ? "LIGADO " : "DESLIGADO");
                
                if (btn == 2 || btn == 3) {
                    setFanManual(!fanManualState);
                    lastBtnPress = m;
                }
                if (btn == 5) {
                    salvarFanConfig();
                    lcd.clear(); 
                    lcd.print("FAN SALVO!"); 
                    delay(1000);
                    etapaFuncoes = 0; 
                    subMenuFuncoesCursor = 0; 
                    lcd.clear();
                    lastBtnPress = m;
                }
            }
        }
    }
}
     

        else if (estadoAtual == CONFIGURACAO) {
            if (btn == 4 && (m - lastBtnPress > 200)) {
                estadoAtual = MENU_PRINCIPAL; etapaConfig = 0; lcd.clear(); lastBtnPress = m;
            }

            if (m - lastBtnPress > 200) {
                if (etapaConfig == 0) {
                    if (btn == 2) { subMenuCursor = (subMenuCursor - 1 + totalSubMenus) % totalSubMenus; lcd.clear(); lastBtnPress = m; }
                    if (btn == 3) { subMenuCursor = (subMenuCursor + 1) % totalSubMenus; lcd.clear(); lastBtnPress = m; }
                    if (btn == 5) {
                        if (subMenuCursor == 0) etapaConfig = 1;
                        else if (subMenuCursor == 1) etapaConfig = 3;
                        else if (subMenuCursor == 2) etapaConfig = 4;
                        else if (subMenuCursor == 3) etapaConfig = 6;
                        lcd.clear(); lastBtnPress = m;
                    }

                    int subTopo = (subMenuCursor / 2) * 2;
                    int subBaixo = subTopo + 1;
                    lcd.setCursor(0, 0);
                    lcd.print(subMenuCursor == subTopo ? "> " : "  ");
                    lcd.print(nomesSub[subTopo]);
                    lcd.setCursor(0, 1);
                    if (subBaixo < totalSubMenus) {
                        lcd.print(subMenuCursor == subBaixo ? "> " : "  ");
                        lcd.print(nomesSub[subBaixo]);
                    } else {
                        lcd.print("                ");
                    }
                } else if (etapaConfig == 1) {
                    lcd.setCursor(0, 0); lcd.print("TPS 0% (SOLTO)  ");
                    lcd.setCursor(0, 1); lcd.print("ADC: "); lcd.print(tpsADC); lcd.print(" SEL     ");
                    if (btn == 5) { tpsMinADC = tpsADC; etapaConfig = 2; lcd.clear(); lastBtnPress = m; }
                } else if (etapaConfig == 2) {
                    lcd.setCursor(0, 0); lcd.print("TPS 100%(FUNDO) ");
                    lcd.setCursor(0, 1); lcd.print("ADC: "); lcd.print(tpsADC); lcd.print("      ");
                    if (btn == 5) {
                        tpsMaxADC = tpsADC;
                        EEPROM.put(addrTPSMin, tpsMinADC);
                        EEPROM.put(addrTPSMax, tpsMaxADC);
                        etapaConfig = 0; lcd.clear(); lcd.print("TPS SALVO!"); delay(1000); lcd.clear(); lastBtnPress = m;
                    }
                } else if (etapaConfig == 3) {
                    lcd.setCursor(0, 0); lcd.print("MAP ATMOSFERICO ");
                    lcd.setCursor(0, 1); lcd.print("ADC: "); lcd.print(mapADC); lcd.print("      ");
                    if (btn == 5) {
                        mapAtmosADC = mapADC;
                        EEPROM.put(addrMAPAtmos, mapAtmosADC);
                        etapaConfig = 0; lcd.clear(); lcd.print("MAP SALVO!"); delay(1000); lcd.clear(); lastBtnPress = m;
                    }
                } else if (etapaConfig == 4) {
                    lcd.setCursor(0, 0); lcd.print("TIPO SINAL RPM  ");
                    lcd.setCursor(0, 1);
                    if (btn == 2 || btn == 3) {
                        pulsesPerRev = (pulsesPerRev == 60) ? 1 : 60;
                        configureIgnitionMode();
                        lastBtnPress = m;
                    }
                    lcd.print(pulsesPerRev == 60 ? "> 60-2 (FONICA) " : "> DISTRIBUIDOR  ");
                    if (btn == 5) {
                        etapaConfig = (pulsesPerRev == 60) ? 7 : 5;
                        selecaoConfirmar = 0;
                        lcd.clear();
                        lastBtnPress = m;
                    }
                } else if (etapaConfig == 7) {
                    lcd.setCursor(0, 0); lcd.print("DENTE SINCRO    ");
                    lcd.setCursor(0, 1); lcd.print("VALOR: "); lcd.print(denteSincro); lcd.print(" (0-59)  ");

                    if (btn == 2 && (m - lastBtnPress > 200)) {
                        denteSincro = (denteSincro < 59) ? denteSincro + 1 : 59;
                        lastBtnPress = m;
                        lcd.setCursor(7, 1); lcd.print(denteSincro); lcd.print(" ");
                    }
                    if (btn == 3 && (m - lastBtnPress > 200)) {
                        denteSincro = (denteSincro > 0) ? denteSincro - 1 : 0;
                        lastBtnPress = m;
                        lcd.setCursor(7, 1); lcd.print(denteSincro); lcd.print(" ");
                    }
                    if (btn == 5 && (m - lastBtnPress > 200)) {
                        etapaConfig = 5;
                        lcd.clear();
                        lastBtnPress = m;
                    }
                } else if (etapaConfig == 5) {
                    lcd.setCursor(0, 0); lcd.print(" ITEM CRITICO!! ");
                    lcd.setCursor(0, 1);
                    if (btn == 1 || btn == 4) { selecaoConfirmar = !selecaoConfirmar; lastBtnPress = m; }
                    lcd.print(selecaoConfirmar == 0 ? ">SALVAR  " : " SALVAR  ");
                    lcd.print(selecaoConfirmar == 1 ? ">CANCELAR" : " CANCELAR");
                    if (btn == 5) {
                        if (selecaoConfirmar == 0) {
                            EEPROM.put(addrSinalRPM, pulsesPerRev);
                            salvarSyncTooth();
                            lcd.clear(); lcd.print("SINAL SALVO!");
                        } else {
                            carregarTabela();
                            carregarTabelaIgnicao();
                            lcd.clear(); lcd.print("CANCELADO!");
                        }
                        delay(1000); etapaConfig = 0; lcd.clear(); lastBtnPress = m;
                    }
                } else if (etapaConfig == 6) {
                    lcd.setCursor(0, 0); lcd.print("OFFSET IGN (graus)");
                    lcd.setCursor(0, 1); lcd.print(ignOffsetDegrees, 1); lcd.print(" ");
                    if (btn == 2) {
                        ignOffsetDegrees = constrain(ignOffsetDegrees + 0.5, -10.0, 10.0);
                        lastBtnPress = m;
                    }
                    if (btn == 3) {
                        ignOffsetDegrees = constrain(ignOffsetDegrees - 0.5, -10.0, 10.0);
                        lastBtnPress = m;
                    }
                    if (btn == 5) {
                        salvarIgnicaoConfig();
                        lcd.clear(); lcd.print("OFFSET SALVO!"); delay(1000);
                        etapaConfig = 0; lcd.clear(); lastBtnPress = m;
                    }
                }
            }
        }

else if (estadoAtual == MAPA_INJ) {
    if (!modoConfirmacao) {
        if (btn == 2 || btn == 3) {
            if (tempoBotaoRetido == 0) { tempoBotaoRetido = m; intervaloAceleracao = 300; tBotaoAcel = 0; }
            if (m - tBotaoAcel > (unsigned long)intervaloAceleracao) {
                tBotaoAcel = m;
                if (m - tempoBotaoRetido > 800) intervaloAceleracao = 40; 
                else if (m - tempoBotaoRetido > 400) intervaloAceleracao = 150;
                if (campoFoco == 0) { if(btn == 2) editR = (editR + 1) % 16; if(btn == 3) editR = (editR + 15) % 16; } 
                else if (campoFoco == 1) { if(btn == 2) editM = (editM + 1) % 16; if(btn == 3) editM = (editM + 15) % 16; } 
                else if (campoFoco == 2) { 
                    if(btn == 2) injTable[editR][editM] += 0.1; 
                    if(btn == 3) injTable[editR][editM] -= 0.1;
                    injTable[editR][editM] = constrain(injTable[editR][editM], 0.1, 20.0);
                }
            }
        } else { tempoBotaoRetido = 0; }
        
        if (m - lastBtnPress > 200) {
            if (btn == 1) { campoFoco = (campoFoco + 1) % 3; lastBtnPress = m; }
            if (btn == 4) { campoFoco = (campoFoco - 1 + 3) % 3; lastBtnPress = m; }
            if (btn == 5) { modoConfirmacao = true; lcd.clear(); lastBtnPress = m; }
        }
        
        lcd.setCursor(0,0);
        if (campoFoco == 0 && blinkState) lcd.print("R:     "); 
        else { lcd.print("R: "); lcd.print(rpmAxis[editR]); }
        lcd.print("     ");
        
        lcd.setCursor(8,0);
        if (campoFoco == 1 && blinkState) lcd.print("M:     "); 
        else { lcd.print("M:"); lcd.print(mapAxis[editM],2); }
        lcd.print("    ");
        
        lcd.setCursor(0,1);
        if (campoFoco == 2 && blinkState) lcd.print("T.Inj:       "); 
        else { lcd.print("T.Inj: "); lcd.print(injTable[editR][editM],2); lcd.print("ms"); }
        lcd.print("    ");
    } else {
        if (m - lastBtnPress > 200) {
            if (btn == 1 || btn == 4) { selecaoConfirmar = !selecaoConfirmar; lastBtnPress = m; }
            if (btn == 5) { 
                if(selecaoConfirmar == 0) salvarTabela(); 
                modoConfirmacao = false; 
                estadoAtual = MENU_PRINCIPAL; 
                lcd.clear(); 
                lastBtnPress = m; 
            }
        }
        lcd.setCursor(0,0); lcd.print("Deseja Salvar?  ");
        lcd.setCursor(0,1); 
        lcd.print(selecaoConfirmar == 0 ? ">SIM    " : " SIM    "); 
        lcd.print(selecaoConfirmar == 1 ? ">NAO    " : " NAO    ");
    }
}

else if (estadoAtual == MAPA_IGN) {
    if (!modoConfirmacao) {
        if (btn == 2 || btn == 3) {
            if (tempoBotaoRetido == 0) { tempoBotaoRetido = m; intervaloAceleracao = 300; tBotaoAcel = 0; }
            if (m - tBotaoAcel > (unsigned long)intervaloAceleracao) {
                tBotaoAcel = m;
                if (m - tempoBotaoRetido > 800) intervaloAceleracao = 40; 
                else if (m - tempoBotaoRetido > 400) intervaloAceleracao = 150;
                if (campoFoco == 0) { if(btn == 2) editR = (editR + 1) % 16; if(btn == 3) editR = (editR + 15) % 16; } 
                else if (campoFoco == 1) { if(btn == 2) editM = (editM + 1) % 16; if(btn == 3) editM = (editM + 15) % 16; } 
                else if (campoFoco == 2) { 
                    if(btn == 2) ignTable[editR][editM] += 0.5; 
                    if(btn == 3) ignTable[editR][editM] -= 0.5;
                    ignTable[editR][editM] = constrain(ignTable[editR][editM], 5.0, 40.0);
                }
            }
        } else { tempoBotaoRetido = 0; }
        
        if (m - lastBtnPress > 200) {
            if (btn == 1) { campoFoco = (campoFoco + 1) % 3; lastBtnPress = m; }
            if (btn == 4) { campoFoco = (campoFoco - 1 + 3) % 3; lastBtnPress = m; }
            if (btn == 5) { modoConfirmacao = true; lcd.clear(); lastBtnPress = m; }
        }
        
        lcd.setCursor(0,0);
        if (campoFoco == 0 && blinkState) lcd.print("R:        "); 
        else { lcd.print("R: "); lcd.print(rpmAxis[editR]); }
        lcd.print("     ");
        
        lcd.setCursor(8,0);
        if (campoFoco == 1 && blinkState) lcd.print("M:        "); 
        else { lcd.print("M:"); lcd.print(mapAxis[editM],2); }
        lcd.print("    ");
        
        lcd.setCursor(0,1);
        if (campoFoco == 2 && blinkState) lcd.print("Avanco:       "); 
        else { lcd.print("Avanco: "); lcd.print(ignTable[editR][editM],1); lcd.print((char)223); }
        lcd.print("       ");
    } else {
        if (m - lastBtnPress > 200) {
            if (btn == 1 || btn == 4) { selecaoConfirmar = !selecaoConfirmar; lastBtnPress = m; }
            if (btn == 5) { 
                if(selecaoConfirmar == 0) salvarTabelasCompletas();
                modoConfirmacao = false; 
                estadoAtual = MENU_PRINCIPAL; 
                lcd.clear(); 
                lastBtnPress = m; 
            }
        }
        lcd.setCursor(0,0); lcd.print("Deseja Salvar?  ");
        lcd.setCursor(0,1); 
        lcd.print(selecaoConfirmar == 0 ? ">SIM    " : " SIM      "); 
        lcd.print(selecaoConfirmar == 1 ? ">NAO    " : " NAO      ");
    }
}
    else {
      if (btn == 4 && (m - lastBtnPress > 200)) { estadoAtual = MENU_PRINCIPAL; lcd.clear(); lastBtnPress = m; }
      lcd.setCursor(0,0); lcd.print("> "); lcd.print(nomesMenus[menuCursor]);
      lcd.setCursor(0,1); lcd.print("EM DESENVOLVIM. ");
    }
    tLCD = m;
  } // Fim do if (millis() - tLCD > 80)
} // Fim do loop()