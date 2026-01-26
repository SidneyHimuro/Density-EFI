# Density EFI - Engine Management System 🏎️

[Read in English](#english) | [Ler em Português](#português)

---

## Português

O **Density EFI** é um sistema de controle de injeção ~~e ignição~~ eletrônica (ECU) de código aberto, desenvolvido para rodar no microcontrolador **ATmega2560** (Plataforma **Arduino Mega 2560**). Este projeto foca em precisão absoluta no tempo de injeção e leitura estável de sensores analógicos.

### 🚀 Funcionalidades
- **Injeção de Combustível:** Tempo de abertura (Pulse Width) controlado por hardware (Timer 3) com precisão de microsegundos.
- ~~**Controle de Ignição:** Mapa de avanço com resolução de 0.5°.~~ (Pendente)
- **Sincronismo:** Leitura de roda fônica **60-2** via interrupção externa (Pino 21).
- **Interpolação 2D:** Tabela 16x16 que cruza RPM e MAP para entrega precisa de combustível.
- **Interface LCD:** Monitoramento em tempo real no display 16x2.

### 📌 Pinagem de Referência (Arduino Mega 2560)
| Função | Pino Arduino | Status |
| :--- | :--- | :--- |
| **Sinal de Rotação** | ~~D2~~ **D21 (INT0)** | Ativo |
| **Sensor MAP** | ~~A1~~ **A4** | Ativo |
| **Sensor TPS** | ~~A2~~ **A5** | Ativo |
| ~~**Sensor ECT (Água)**~~ | ~~A3~~ | Pendente |
| **Saída Injetor** | ~~D11~~ **D22** | Ativo (Timer 3) |
| ~~**Ignição A**~~ | ~~D12~~ | Inativo |

---

## English

**Density EFI** is an open-source fuel injection ~~and ignition~~ management system (ECU) developed for the **ATmega2560** (Using **Arduino Mega 2560** hardware). It prioritizes injection timing precision and stable analog sensor processing.

### 🚀 Features
- **Fuel Injection:** Pulse Width controlled by hardware (Timer 3) with microsecond precision.
- ~~**Ignition Control:** Advance map with 0.5° resolution.~~ (Pending)
- **Synchronization:** **60-2** trigger wheel pattern decoding via external interrupt (Pin 21).
- **2D Interpolation:** 16x16 Fuel Map (RPM vs MAP) for accurate delivery.
- **LCD Interface:** 16x2 display for real-time monitoring.

### 📌 Pinout Reference (Arduino Mega 2560)
| Function | Arduino Pin Name | Status |
| :--- | :--- | :--- |
| **RPM Signal** | ~~D2~~ **D21 (INT0)** | Active |
| **MAP Sensor** | ~~A1~~ **A4** | Active |
| **TPS Sensor** | ~~A2~~ **A5** | Active |
| **Injector Output**| ~~D11~~ **D22** | Active (Timer 3) |

---

## 🛠️ Instalação / Installation

### 1. Requisitos / Requirements
- **Hardware:** Arduino Mega 2560.
- **IDE:** Arduino IDE 1.8.x ou superior.
- **Bibliotecas:** `LiquidCrystal` (Nativa).

### 2. Próximos Passos
- Implementar **Aceleração Rápida (TPS Delta)**.
- Implementar **Correção de Partida a Frio**.
- Implementar **Saída de Ignição**.

## ⚠️ Disclaimer

O ajuste de parâmetros do motor pode resultar em danos mecânicos graves. Este projeto tem fins educacionais. Use por sua conta e risco.
