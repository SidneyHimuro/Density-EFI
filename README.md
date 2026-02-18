# 🏎️ Density EFI - Engine Management System (v1.1)

[Read in English](#english) | [Ler em Português](#português)

---

### 💻 Layout Hardware Density EFI V1.0 
![Layout Hardware](images/DensityEFI_V1.png)

---
## 🇧🇷 Português

O **Density EFI** é um sistema de controle de injeção e ignição eletrônica de código aberto baseado na plataforma **Arduino Mega 2560**. Projetado para entusiastas e fins educacionais, o sistema utiliza uma arquitetura modular para oferecer precisão em tempo real no gerenciamento de motores a combustão interna.

### 🚀 Funcionalidades Principais

#### ⛽ Injeção Eletrônica
* **Precisão de Hardware:** Controle via **Timer4** (16-bit) com resolução de microssegundos.
* **Mapas Interativos:** Tabela 3D **16x16** (RPM x MAP) editável em tempo real via LCD.
* **Estratégias Avançadas:** Modos *Full Group* e *Semi-Sequencial* com compensação por tensão de bateria.
* **Segurança:** Função *Flood Clear* (corte de combustível com borboleta aberta na partida).

#### ⚡ Ignição Programável
* **Controle de Avanço:** Tabela 16x16 dedicada com avanço variável (5° a 40°).
* **Modos de Operação:** Suporte para **Centelha Perdida** (Wasted Spark) ou **Distribuidor**.
* **Gerenciamento de Dwell:** Tempo de carga configurável (1.0 a 5.0ms) para proteger as bobinas.
* **Sincronismo:** Compatível com roda fônica **60-2** (ajuste de dente de sincronia 0-59) e offset de calibração.

#### 🌡️ Sensoriamento e Atuação
* **Sensores:** MAP (Pressão), TPS (Posição da Borboleta), ECT (Temperatura do Motor) e monitoramento de Voltagem da Bateria.
* **Atuadores:** Controle de eletroventilador automático/manual com histerese e saídas para driver MOSFET de injetores e bobinas.

#### 📊 Interface e Telemetria
* **HMI Dinâmica:** Display LCD 16x2 com 4 telas de monitoramento (RPM, MAP, TPS, Injeção, Avanço, Temperatura).
* **Software Dash:** Telemetria via Serial (115200 baud) compatível com dashboards em Python/Plotly.

### 📂 Estrutura de Arquivos
- `Density_EFI.ino`: Orquestrador principal e lógica de interface.
- `Crank.cpp/h`: Gestão de interrupções, RPM e sincronismo de dentes.
- `IgnitionControl.cpp/h`: Lógica não-bloqueante para avanço e dwell.
- `Injector.cpp/h`: Cálculo de pulse-width e Enriquecimento por Aceleração ($AE_{tps}$).

---

## 🇺🇸 English

**Density EFI** is an open-source engine management system (ECU) for the **Arduino Mega 2560**. It features a modular architecture designed for real-time precision, offering full control over fuel injection and ignition timing.

### 🚀 Key Features
- ⛽ **Fuel Injection:** Hardware-timed pulses (Timer4) with 16x16 3D maps and *Flood Clear* protection.
- ⚡ **Programmable Ignition:** Supports *Wasted Spark* or *Distributor* modes, adjustable Dwell (1-5ms), and 16x16 advance maps.
- 🔄 **Robust Sync:** Optimized for 60-2 trigger wheels with configurable sync tooth and ignition offset.
- 🌡️ **Full Sensing:** Real-time monitoring of MAP, TPS, Engine Temp (ECT), and Battery Voltage.
- 🖥️ **HMI & Telemetry:** 4-screen LCD interface for PC-less tuning and 115200 baud serial output for data logging.

---

### 📌 Pinagem de Referência / Pinout (Mega 2560)

| Função / Function | Pino / Pin | Tipo / Type |
| :--- | :--- | :--- |
| **RPM Signal (Crank)** | D21 | Entrada (Interrupt) |
| **Injector Out** | D22 | Saída (MOSFET Driver) |
| **Ignition Coil A** | D40 | Cilindros 1-4 |
| **Ignition Coil B** | D38 | Cilindros 2-3 |
| **MAP Sensor** | A4 | Analógica (0-5V) |
| **TPS Sensor** | A3 | Analógica (0-5V) |
| **ECT (Temp)** | A1 | Analógica (NTC 10k) |
| **Battery Voltage** | A5 | Analógica (Divisor) |
| **Eletroventilador** | D47 | Saída (Relé) |
| **LCD Pins** | 8, 9, 4, 5, 6, 7 | Interface 4-bits |

---

### ⚠️ Disclaimer
O ajuste de parâmetros do motor pode resultar em **danos mecânicos graves**. Este projeto tem fins educacionais. Use por sua conta e risco.  
Adjusting engine parameters can result in **serious mechanical damage**. This project is for educational purposes. Use at your own risk.