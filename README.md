# 🏎️ Density EFI - Engine Management System

[Read in English](#english) | [Ler em Português](#português)

---

## 🇧🇷 Português

O **Density EFI** é um sistema de controle de injeção eletrônica (ECU) de código aberto, desenvolvido para rodar no microcontrolador **ATmega2560** (Plataforma **Arduino Mega 2560**). Este projeto foca em **precisão absoluta no tempo de injeção** e leitura estável de sensores analógicos.

### 🚀 Funcionalidades
- ⛽ **Injeção de Combustível:** Tempo de abertura (Pulse Width) controlado por hardware (Timer 3) com precisão de microsegundos.  
- ⚡ **Sincronismo:** Leitura de roda fônica **60-2** via interrupção externa (Pino 21).  
- 📊 **Interpolação 2D:** Tabela 16x16 que cruza RPM e MAP para entrega precisa de combustível.  
- 🖥️ **Interface LCD:** Monitoramento em tempo real no display 16x2.  

### 📌 Pinagem de Referência (Arduino Mega 2560)
| Função | Pino Arduino | Status |
| :--- | :--- | :--- |
| 🔄 **Sinal de Rotação** | D21 (INT0) | Ativo |
| 🏭 **Sensor MAP** | A4 | Ativo |
| 🏎️ **Sensor TPS** | A5 | Ativo |
| 💨 **Saída Injetor** | D22 | Ativo (Timer 3) |

---

### 🛠️ Instalação
1. **Requisitos**
   - **Hardware:** Arduino Mega 2560  
   - **IDE:** Arduino IDE 1.8.x ou superior  
   - **Bibliotecas:** `LiquidCrystal` (nativa)  

2. **Próximos Passos**
   - Implementar **Aceleração Rápida (TPS Delta)**  
   - Implementar **Correção de Partida a Frio**  
   - Implementar **Saída de Ignição**  

---

### ⚠️ Disclaimer
O ajuste de parâmetros do motor pode resultar em **danos mecânicos graves**. Este projeto tem fins educacionais. Use por sua conta e risco.

---

## 🇺🇸 English

**Density EFI** is an open-source fuel injection management system (ECU) developed for the **ATmega2560** (Using **Arduino Mega 2560** hardware). This project focuses on **absolute precision in injection timing** and stable analog sensor readings.

### 🚀 Features
- ⛽ **Fuel Injection:** Pulse Width controlled by hardware (Timer 3) with microsecond precision.  
- ⚡ **Synchronization:** **60-2** trigger wheel pattern decoding via external interrupt (Pin 21).  
- 📊 **2D Interpolation:** 16x16 Fuel Map (RPM vs MAP) for accurate delivery.  
- 🖥️ **LCD Interface:** 16x2 display for real-time monitoring.  

### 📌 Pinout Reference (Arduino Mega 2560)
| Function | Arduino Pin | Status |
| :--- | :--- | :--- |
| 🔄 **RPM Signal** | D21 (INT0) | Active |
| 🏭 **MAP Sensor** | A4 | Active |
| 🏎️ **TPS Sensor** | A5 | Active |
| 💨 **Injector Output** | D22 | Active (Timer 3) |

---

### 🛠️ Installation
1. **Requirements**
   - **Hardware:** Arduino Mega 2560  
   - **IDE:** Arduino IDE 1.8.x or higher  
   - **Libraries:** `LiquidCrystal` (built-in)  

2. **Next Steps**
   - Implement **TPS Delta / Acceleration Enrichment**  
   - Implement **Cold Start Correction**  
   - Implement **Ignition Output**  

---

### ⚠️ Disclaimer
Adjusting engine parameters can result in **serious mechanical damage**. This project is for educational purposes. Use at your own risk.
