#include <LiquidCrystal.h>
#include <EEPROM.h>
#include "Crank.h"
#include "Injector.h"
#include "IgnitionControl.h" // ← ADICIONAR ESTE INCLUDE

// ================= LCD =================
// Shield Keyestudio usa pinos 8, 9, 4, 5, 6, 7
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// ================= ESTADOS DO MENU =================
enum MenuState { MENU_PRINCIPAL, MONITORAMENTO, MAPA_INJ, MAPA_IGN, FUNCOES, CONFIGURACAO };
MenuState estadoAtual = MENU_PRINCIPAL;
int menuCursor = 0;
const int totalMenus = 5;
String nomesMenus[] = {"MONITORAMENTO ", "MAPA INJCAO     ", "MAPA IGNICAO     ", "FUNCOES         ", "CONFIGURACAO  "};

// Variáveis de Edição e Interface
int editR = 0, editM = 0; 
byte campoFoco = 0; 
bool modoConfirmacao = false;
int selecaoConfirmar = 0; 
unsigned long tempoBotaoRetido = 0;
int intervaloAceleracao = 250; 
unsigned long lastBtnPress = 0; 

// ================= SENSORES =================
const byte mapPin = A4;
const byte tpsPin = A3; 
int tpsMinADC = 100, tpsMaxADC = 900; 
int mapAtmosADC = 940; 
float mapBar = 0.0, tpsPercent = 0.0;
float ignAdvance = 10.0; // Avanço de ignição atual em graus

// ================= IGNIÇÃO OFFSET =================
float ignOffsetDegrees = 0.0;  // Offset de calibração em graus (-10.0 a +10.0)
byte ignFixedAngle = 0;        // Ângulo fixo para calibração (0, 10, 20)
const int addrIgnOffset = 2100; // Endereço EEPROM para offset
const int addrIgnFixedAngle = 2104; // Endereço EEPROM para ângulo fixo

// Variáveis de Navegação Submenus
byte etapaConfig = 0;
int subMenuCursor = 0;
const int totalSubMenus = 4;  // Aumente de 3 para 4
String nomesSub[] = {"CALIBRAR TPS     ", "CALIBRAR MAP     ", "SINAL ROTACAO      ", "OFFSET IGNICAO     "};

byte etapaFuncoes = 0; 
int subMenuFuncoesCursor = 0;
const int totalSubFuncoes = 1;
String nomesSubFuncoes[] = {"ACEL. RAPIDA    "};

// ================= EIXOS E TABELA INJEÇÃO =================
const int rpmAxis[16] = {500, 800, 1200, 1600, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500};
const float mapAxis[16] = {0.0,-0.07,-0.14,-0.21,-0.28,-0.35,-0.42,-0.49,-0.56,-0.63,-0.70,-0.77,-0.84,-0.91,-0.96,-1.0};
float injTable[16][16]; 

// ================= TABELA DE IGNIÇÃO =================
float ignTable[16][16]; // Nova tabela de avanço de ignição (em graus)

// ================= ENDEREÇOS EEPROM IGNIÇÃO =================
const int addrIgnTableStart = 1500; // Endereço para tabela de ignição (após espaço da injeção)
const int addrIgnParams = 2000;     // Endereço para parâmetros de ignição

// ================= EEPROM ADDRESSES =================
const int addrTPSMin = 1030, addrTPSMax = 1034, addrMAPAtmos = 1038;
const int addrSinalRPM = 1042, addrAEMax = 1046, addrAEDecay = 1050;

//================CONFIGURAÇÃO DE IGNIÇÃO===========
void configureIgnitionMode() {
  if (pulsesPerRev == 60) {
    // Modo 60-2: centelha perdida
    setWastedSparkMode(true);
  } else {
    // Modo distribuidor: 1 canal
    setWastedSparkMode(false);
  }
}

void salvarIgnicaoConfig() {
  EEPROM.put(addrIgnOffset, ignOffsetDegrees);
  EEPROM.put(addrIgnFixedAngle, ignFixedAngle);
}

void carregarIgnicaoConfig() {
  EEPROM.get(addrIgnOffset, ignOffsetDegrees);
  EEPROM.get(addrIgnFixedAngle, ignFixedAngle);
  
  // Validar valores
  if (isnan(ignOffsetDegrees) || ignOffsetDegrees < -10.0 || ignOffsetDegrees > 10.0) {
    ignOffsetDegrees = 0.0;
  }
  if (ignFixedAngle != 0 && ignFixedAngle != 10 && ignFixedAngle != 20) {
    ignFixedAngle = 0;
  }
}

// ================= FUNÇÕES DE APOIO =================
int lerBotao() {
  int val = analogRead(0);
  if (val < 50)   return 1; // RIGHT
  if (val < 150)  return 2; // UP
  if (val < 350)  return 3; // DOWN (Ajustado para o Shield)
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
  // Salva também parâmetros de injeção
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
    
    // Tentar carregar ignição também (opcional)
    float ignVal;
    EEPROM.get(addrIgnTableStart, ignVal);
    if (!isnan(ignVal) && ignVal >= 5.0 && ignVal <= 40.0) {
      // Já foi carregado por carregarTabelaIgnicao(), mas mantém consistência
    }
  } else {
    for(int i=0; i<16; i++) for(int j=0; j<16; j++) injTable[i][j] = 1.5;
  }
}

// ================= FUNÇÕES PARA IGNIÇÃO =================
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
    // Valores padrão para ignição (avanço em graus)
    for(int i = 0; i < 16; i++) {
      for(int j = 0; j < 16; j++) {
        if (rpmAxis[i] < 2000) {
          ignTable[i][j] = 10.0 + (j * 0.5); // Avanço progressivo com vácuo
        } else if (rpmAxis[i] < 5000) {
          ignTable[i][j] = 28.0 + (j * 0.3); // Avanço máximo na faixa média
        } else {
          ignTable[i][j] = 24.0 + (j * 0.2); // Reduz avanço em altas RPM
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
  
  float advance = a + fm * (b - a);
  
  // Aplicar offset de calibração
  advance += ignOffsetDegrees;
  
  return advance;
}


void salvarTabelasCompletas() {
  salvarTabela();           // Salva tabela de injeção (função existente)
  salvarTabelaIgnicao();    // Salva tabela de ignição (nova função)
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

// ================= SETUP & LOOP =================
void setup() {
  lcd.begin(16, 2);
  Serial.begin(115200);
  
  carregarTabela();        
  carregarTabelaIgnicao(); 
  carregarIgnicaoConfig(); // Adicione esta linha
  setupCrank();            
  setupInjector();         
  setupIgnition();         

  // Configurar modo de ignição baseado no sinal RPM
  configureIgnitionMode();
  

  // TESTE: Verificar se tabelas foram carregadas
  Serial.println("=== DENSITY EFI v1.1 ===");
  Serial.print("Inj[0][0]: "); Serial.println(injTable[0][0]);
  Serial.print("Ign[0][0]: "); Serial.println(ignTable[0][0]);
  Serial.print("Ign[15][15]: "); Serial.println(ignTable[15][15]);
  
  lcd.print("HimuroPerform.");
  lcd.setCursor(0, 1); lcd.print("Density EFI v1.1");
  delay(2000); lcd.clear();
}

void loop() {
  // 1. Leituras de Sensores
  int mapADC = analogRead(mapPin);
  int tpsADC = analogRead(tpsPin);
  
  // Cálculo baseado na inclinação do sensor GM 1-Bar
  // mapAtmosADC é o seu ponto de referência (100kPa). 
  // Cada 1kPa no sensor GM equivale a aproximadamente 10 unidades de ADC no Arduino.
  float kpaFaltante = (float)(mapAtmosADC - mapADC) / 10.24; 
  
  // mapBar: 0.0 é atmosfera, -0.80 é vácuo forte.
  mapBar = -(kpaFaltante / 100.0); 

  // Garante que o vácuo não ultrapasse os limites da sua tabela (Eixo de 0.0 a -1.0)
  mapBar = constrain(mapBar, -1.0, 0.0);
  
  tpsPercent = constrain((float)(tpsADC - tpsMinADC) * 100.0 / (tpsMaxADC - tpsMinADC), 0, 100);

  // 2. Atualização de Estados (RPM e Enriquecimento)
  updateCrankRPM();
  updateInjectorAE(tpsPercent, rpm);

  // 3. Cálculo do Tempo de Injeção Base
  if (rpm > 40 && syncOK) { // Reduzi de 400 para 40 para permitir leitura na partida
    Tinj_latched = interp2D(rpm, mapBar);
    
    // 3b. CÁLCULO DO AVANÇO DE IGNIÇÃO (NOVO)
    ignAdvance = interp2DIgn(rpm, mapBar);
  ignAdvance = constrain(ignAdvance, 5.0, 40.0); // Limites de segurança
  
  
  if (pulsesPerRev == 60) {
  // Para 60-2, use a função baseada em dentes
  updateIgnitionFromTeeth(toothCount, syncOK);
} else {
  // Para distribuidor, use a função original
  updateIgnition(rpm, syncOK, pulsesPerRev);
}
  

} else {
  Tinj_latched = 0; 
  ignAdvance = 10.0; // Avanço seguro padrão quando motor parado
  updateIgnition(0, false); // Desabilita ignição quando motor parado
}

  // 4. Execução da Injeção (Timer 4 disparado aqui)
  runInjector(rpm, mapBar, tpsPercent);

  // 5. Telemetria Serial
  static unsigned long tSer = 0;
  if (millis() - tSer > 100) {
    Serial.print(rpm); Serial.print(","); Serial.print(mapBar); Serial.print(",");
    Serial.print(tpsPercent, 1); Serial.print(","); Serial.print(Tinj_latched + AE_TPS);
    Serial.print(","); Serial.println(ignAdvance, 1); // Novo: avanço de ignição
    tSer = millis();
  }
  
  // --- Lógica de LCD e Menu ---
  static unsigned long tLCD = 0;
  static unsigned long tBotaoAcel = 0; 
  if (millis() - tLCD > 80) { 
    int btn = lerBotao();
    unsigned long m = millis();
    bool blinkState = (m / 400) % 2;

    if (estadoAtual == MENU_PRINCIPAL) {
      if (m - lastBtnPress > 200) {
        if (btn == 2) { menuCursor = (menuCursor - 1 + totalMenus) % totalMenus; lcd.clear(); lastBtnPress = m; }
        if (btn == 3) { menuCursor = (menuCursor + 1) % totalMenus; lcd.clear(); lastBtnPress = m; }
        if (btn == 5) { estadoAtual = (MenuState)(menuCursor + 1); etapaConfig = 0; etapaFuncoes = 0; subMenuCursor = 0; lcd.clear(); lastBtnPress = m; }
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
      } else { lcd.print("                "); }
    } 
    else if (estadoAtual == FUNCOES) {
      if (btn == 4 && (m - lastBtnPress > 200)) { 
        if(etapaFuncoes > 0) { etapaFuncoes = 0; lcd.clear(); }
        else { estadoAtual = MENU_PRINCIPAL; lcd.clear(); }
        lastBtnPress = m; 
      }
      else if (etapaFuncoes == 3) { // Adicionar após AE decay
  lcd.setCursor(0,0); lcd.print("DWELL BOBINA    ");
  lcd.setCursor(0,1); lcd.print("VALOR: "); lcd.print(getIgnitionDwell(), 1); lcd.print("ms");
  if(btn == 2) { setIgnitionDwell(getIgnitionDwell() + 0.1); lastBtnPress = m; }
  if(btn == 3) { setIgnitionDwell(getIgnitionDwell() - 0.1); lastBtnPress = m; }
  if(btn == 5) { etapaFuncoes = 0; lcd.clear(); lastBtnPress = m; }
}      
      if (m - lastBtnPress > 200) {
        if (etapaFuncoes == 0) {
           lcd.setCursor(0,0); lcd.print("> ACEL. RAPIDA  ");
           lcd.setCursor(0,1); lcd.print("                ");
           if(btn == 5) { etapaFuncoes = 1; lcd.clear(); lastBtnPress = m; }
        }
        else if (etapaFuncoes == 1) {
           lcd.setCursor(0,0); lcd.print("GANHO AE (ms)   ");
           lcd.setCursor(0,1); lcd.print("VALOR: "); lcd.print(AE_TPS_max, 1);
           if(btn == 2) { AE_TPS_max += 0.1; lastBtnPress = m; }
           if(btn == 3) { AE_TPS_max -= 0.1; lastBtnPress = m; }
           AE_TPS_max = constrain(AE_TPS_max, 0.0, 5.0);
           if(btn == 5) { etapaFuncoes = 2; lcd.clear(); lastBtnPress = m; }
        }
        else if (etapaFuncoes == 2) {
           lcd.setCursor(0,0); lcd.print("DECAIMENTO (ms) ");
           lcd.setCursor(0,1); lcd.print("VALOR: "); lcd.print((int)AE_decay_ms);
           if(btn == 2) { AE_decay_ms += 10; lastBtnPress = m; }
           if(btn == 3) { AE_decay_ms -= 10; lastBtnPress = m; }
           AE_decay_ms = constrain(AE_decay_ms, 50, 1000);
           if(btn == 5) { 
              EEPROM.put(addrAEMax, AE_TPS_max); EEPROM.put(addrAEDecay, AE_decay_ms);
              lcd.clear(); lcd.print("AE SALVO!"); delay(1000);
              etapaFuncoes = 0; lcd.clear(); lastBtnPress = m; 
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
            if(subMenuCursor == 0) etapaConfig = 1;
            else if(subMenuCursor == 1) etapaConfig = 3;
            else if(subMenuCursor == 2) etapaConfig = 4;
            
            lcd.clear(); lastBtnPress = m; 
          }
          int subTopo = (subMenuCursor / 2) * 2;
          int subBaixo = subTopo + 1;
          lcd.setCursor(0, 0);
          lcd.print(subMenuCursor == subTopo ? "> " : "  "); lcd.print(nomesSub[subTopo]);
          lcd.setCursor(0, 1);
          if (subBaixo < totalSubMenus) {
            lcd.print(subMenuCursor == subBaixo ? "> " : "  "); lcd.print(nomesSub[subBaixo]);
          } else { lcd.print("                "); }
        }
        else if (etapaConfig == 1) { 
          lcd.setCursor(0,0); lcd.print("TPS 0% (SOLTO)  ");
          lcd.setCursor(0,1); lcd.print("ADC: "); lcd.print(tpsADC); lcd.print(" SEL");
          if (btn == 5) { tpsMinADC = tpsADC; etapaConfig = 2; lcd.clear(); lastBtnPress = m; }
        }
        else if (etapaConfig == 2) { 
          lcd.setCursor(0,0); lcd.print("TPS 100%(FUNDO) ");
          lcd.setCursor(0,1); lcd.print("ADC: "); lcd.print(tpsADC); 
          if (btn == 5) { 
            tpsMaxADC = tpsADC; 
            EEPROM.put(addrTPSMin, tpsMinADC); EEPROM.put(addrTPSMax, tpsMaxADC);
            etapaConfig = 0; lcd.clear(); lcd.print("TPS SALVO!"); delay(1000); lcd.clear(); lastBtnPress = m; 
          }
        }
        else if (etapaConfig == 3) { 
          lcd.setCursor(0,0); lcd.print("MAP ATMOSFERICO ");
          lcd.setCursor(0,1); lcd.print("ADC: "); lcd.print(mapADC);
          if (btn == 5) {
            mapAtmosADC = mapADC;
            EEPROM.put(addrMAPAtmos, mapAtmosADC);
            etapaConfig = 0; lcd.clear(); lcd.print("MAP SALVO!"); delay(1000); lcd.clear(); lastBtnPress = m;
          }
        }
        else if (etapaConfig == 4) { 
  lcd.setCursor(0,0); lcd.print("TIPO SINAL RPM  ");
  lcd.setCursor(0,1);
  if (btn == 2 || btn == 3) { 
    pulsesPerRev = (pulsesPerRev == 60) ? 1 : 60; 
    configureIgnitionMode(); // Atualiza modo de ignição
    lastBtnPress = m; 
  }
  lcd.print(pulsesPerRev == 60 ? "> 60-2 (FONICA) " : "> DISTRIBUIDOR  ");
  if (btn == 5) { etapaConfig = 5; selecaoConfirmar = 0; lcd.clear(); lastBtnPress = m; }
}
        else if (etapaConfig == 5) { 
          lcd.setCursor(0,0); lcd.print(" ITEM CRITICO!! ");
          lcd.setCursor(0,1);
          if (btn == 1 || btn == 4) { selecaoConfirmar = !selecaoConfirmar; lastBtnPress = m; }
          lcd.print(selecaoConfirmar == 0 ? ">SALVAR  " : " SALVAR  ");
          lcd.print(selecaoConfirmar == 1 ? ">CANCELAR" : " CANCELAR");
          if (btn == 5) {
            if (selecaoConfirmar == 0) { EEPROM.put(addrSinalRPM, pulsesPerRev); lcd.clear(); lcd.print("SINAL SALVO!"); }
            else { 
              // Recarrega valores da EEPROM
              carregarTabela();
              carregarTabelaIgnicao();
              lcd.clear(); lcd.print("CANCELADO!"); 
            }
            delay(1000); etapaConfig = 0; lcd.clear(); lastBtnPress = m;
          }
        }
      }
    }
    else if (estadoAtual == MONITORAMENTO) {
      if (btn == 5 && (m - lastBtnPress > 200)) { estadoAtual = MENU_PRINCIPAL; lcd.clear(); lastBtnPress = m; }
      lcd.setCursor(0, 0); lcd.print("R:"); lcd.print(rpm); lcd.print("    ");
      //lcd.setCursor(8, 0); lcd.print("T:"); lcd.print((int)tpsPercent); lcd.print("%  ");
      lcd.setCursor(8, 0); lcd.print("I:"); lcd.print(ignAdvance, 1);
      lcd.setCursor(0, 1); lcd.print("M:"); lcd.print(mapBar, 2); lcd.print("  ");
      lcd.setCursor(8, 1); lcd.print(Tinj_latched + AE_TPS, 2); lcd.print("ms ");
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
              if(btn == 2) injTable[editR][editM] += 0.01; if(btn == 3) injTable[editR][editM] -= 0.01;
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
        if (campoFoco == 0 && blinkState) lcd.print("R:     "); else { lcd.print("R: "); lcd.print(rpmAxis[editR]); }
        lcd.print("     ");
        lcd.setCursor(8,0);
        if (campoFoco == 1 && blinkState) lcd.print("M:     "); else { lcd.print("M:"); lcd.print(mapAxis[editM],2); }
        lcd.print("    ");
        lcd.setCursor(0,1);
        if (campoFoco == 2 && blinkState) lcd.print("T.Inj:       "); else { lcd.print("T.Inj: "); lcd.print(injTable[editR][editM],2); lcd.print("ms"); }
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
              if(btn == 2) ignTable[editR][editM] += 0.5; // Incremento de 0.5 graus
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
        if (campoFoco == 0 && blinkState) lcd.print("R:        "); else { lcd.print("R: "); lcd.print(rpmAxis[editR]); }
        lcd.print("     ");
        lcd.setCursor(8,0);
        if (campoFoco == 1 && blinkState) lcd.print("M:        "); else { lcd.print("M:"); lcd.print(mapAxis[editM],2); }
        lcd.print("    ");
        lcd.setCursor(0,1);
        if (campoFoco == 2 && blinkState) lcd.print("Avanco:       "); else { lcd.print("Avanco: "); lcd.print(ignTable[editR][editM],1); lcd.print((char)223); } // (char)223 = símbolo de grau
        lcd.print("       ");
      } else {
        if (m - lastBtnPress > 200) {
          if (btn == 1 || btn == 4) { selecaoConfirmar = !selecaoConfirmar; lastBtnPress = m; }
          if (btn == 5) { 
            if(selecaoConfirmar == 0) salvarTabelasCompletas(); // Salva ambas tabelas
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
  }
}