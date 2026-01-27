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

### 🖥️ Dashboard em Tempo Real (Interface Python)
O projeto inclui um painel de instrumentos moderno desenvolvido em **Python (Dash/Plotly)** para monitoramento avançado via USB:
- 📈 **Visualização:** Barra de RPM estilo "Race" com escala de 1 a 8.
- 🚥 **Shift Light:** Alerta visual programável que faz a barra de RPM piscar no limite definido.
- 🔌 **Conectividade:** Seletor de porta COM dinâmico, botão Conectar/Desconectar e indicador LED de status de dados em tempo real.

### 📌 Pinagem de Referência (Arduino Mega 2560)
| Função | Pino Arduino | Status |
| :--- | :--- | :--- |
| 🔄 **Sinal de Rotação** | D21 (INT0) | Ativo |
| 🏭 **Sensor MAP** | A4 | Ativo |
| 🏎️ **Sensor TPS** | A5 | Ativo |
| 💨 **Saída Injetor** | D22 | Ativo (Timer 3) |

---

### 🛠️ Instalação e Uso
1. **Módulo de Injeção (Hardware)**
   - Carregue o firmware usando a Arduino IDE 1.8.x ou superior.
2. **Dashboard (Software)**
   - Instale as dependências: `pip install dash pyserial plotly`.
   - Execute o arquivo: `python main.py`.
   - Acesse no navegador: `http://127.0.0.1:8050`.

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

### 🖥️ Real-Time Dashboard (Python Interface)
The project includes a modern instrument cluster developed in **Python (Dash/Plotly)** for advanced USB monitoring:
- 📈 **Visualization:** "Race" style RPM bar with a 1 to 8 scale.
- 🚥 **Shift Light:** Programmable visual alert that flashes the RPM bar at a defined limit.
- 🔌 **Connectivity:** Dynamic COM port selector, Connect/Disconnect button, and real-time data status LED indicator.

### 📌 Pinout Reference (Arduino Mega 2560)
| Function | Arduino Pin | Status |
| :--- | :--- | :--- |
| 🔄 **RPM Signal** | D21 (INT0) | Active |
| 🏭 **MAP Sensor** | A4 | Active |
| 🏎️ **TPS Sensor** | A5 | Active |
| 💨 **Injector Output** | D22 | Active (Timer 3) |

---

### 🛠️ Installation and Usage
1. **Injection Module (Hardware)**
   - Upload the firmware using Arduino IDE 1.8.x or higher.
2. **Dashboard (Software)**
   - Install dependencies: `pip install dash pyserial plotly`.
   - Run the script: `python main.py`.
   - Open in your browser: `http://127.0.0.1:8050`.

---

### ⚠️ Disclaimer
Adjusting engine parameters can result in **serious mechanical damage**. This project is for educational purposes. Use at your own risk.
