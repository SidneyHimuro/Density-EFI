# Density EFI - Engine Management System 🏎️

[Read in English](#english) | [Ler em Português](#português)

---

## Português

O **Density EFI** é um sistema de controle de injeção e ignição eletrônica (ECU) de código aberto, desenvolvido para rodar diretamente no microcontrolador **ATmega2560**. Este projeto foca em precisão absoluta, estabilidade de sinal e uma interface de usuário simplificada para ajustes em tempo real.

### 🚀 Funcionalidades
- **Injeção de Combustível:** Ajuste de tempo de abertura (Pulse Width) com precisão de **0.01ms**.
- **Controle de Ignição:** Mapa de avanço com resolução de **0.5°**.
- **Sincronismo:** Lógica avançada para leitura de roda fônica **60-2**.
- **Interface LCD:** Menus de navegação via display 16x2 com lógica de incremento acelerado nos botões.
- **Compensações Ativas:** - Enriquecimento de aceleração (AE) via variação de TPS.
  - Correção por temperatura do motor (ECT) para partidas a frio e proteção térmica.

### 📌 Pinagem de Referência (Standalone ATmega2560)
| Função | Pino Digital/Analógico | Pino Físico (TQFP-100) |
| :--- | :--- | :--- |
| **Sinal de Rotação** | D2 (INT4) | Pino 6 |
| **Sensor MAP** | A1 | Pino 96 |
| **Sensor TPS** | A2 | Pino 95 |
| **Sensor ECT (Água)** | A3 | Pino 94 |
| **Saída Injetor** | D11 | Pino 13 |
| **Ignição A** | D12 | Pino 14 |
| **Ignição B** | D13 | Pino 15 |



---

## English

**Density EFI** is an open-source engine management system (ECU) developed to run natively on the **ATmega2560** microcontroller. This project focuses on absolute precision, signal stability, and a simplified user interface for real-time tuning.

### 🚀 Features
- **Fuel Injection:** Pulse Width adjustment with **0.01ms** precision.
- **Ignition Control:** Advance map with **0.5°** resolution.
- **Synchronization:** Advanced logic for **60-2** trigger wheel pattern.
- **LCD Interface:** 16x2 display navigation with smart button acceleration logic.
- **Active Corrections:** - Acceleration Enrichment (AE) based on TPS delta.
  - Engine Coolant Temperature (ECT) compensation for cold starts and thermal protection.

### 📌 Pinout Reference (Standalone ATmega2560)
| Function | Arduino Pin Name | Physical Pin (TQFP-100) |
| :--- | :--- | :--- |
| **RPM Signal** | D2 (INT4) | Pin 6 |
| **MAP Sensor** | A1 | Pin 96 |
| **TPS Sensor** | A2 | Pin 95 |
| **ECT Sensor** | A3 | Pin 94 |
| **Injector Output**| D11 | Pin 13 |
| **Ignition A** | D12 | Pin 14 |
| **Ignition B** | D13 | Pin 15 |



---

## 🛠️ Instalação / Installation

### 1. Requisitos / Requirements
- **Hardware:** ATmega2560 Standalone PCB ou Arduino Mega 2560.
- **IDE:** Arduino IDE 1.8.x ou superior.
- **Programador:** USBasp ou Arduino como ISP (para gravação via ICSP).

### 2. Como usar / How to use
- **Navegação:** Use as teclas para alternar entre `MONITOR`, `FUEL MAP`, `IGN MAP` e `SETUP`.
- **Ajuste:** Pressione as setas para alterar valores. Segure por 1.5s para aumentar a velocidade de incremento.
- **Salvar:** Segure o botão **SELECT** por 2 segundos para gravar os dados na EEPROM.

## ⚠️ Disclaimer / Isenção de Responsabilidade

Adjusting engine parameters can result in severe mechanical damage. This project is for educational and experimental purposes. Use it at your own risk.

O ajuste de parâmetros do motor pode resultar em danos mecânicos graves. Este projeto tem fins educacionais e experimentais. Use por sua conta e risco.

## License / Licença
Distributed under the MIT License. See `LICENSE` for more information.