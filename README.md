# 🏎️ Density EFI - Engine Management System (v1.0)

[Read in English](#english) | [Ler em Português](#português)

---

## 🇧🇷 Português

O **Density EFI** é um sistema de controle de injeção eletrônica (ECU) de código aberto para a plataforma **Arduino Mega 2560**. Esta versão utiliza uma arquitetura modular para garantir precisão em tempo real e estabilidade no controle do motor.

### 🚀 Funcionalidades Atuais
- ⛽ **Injeção de Combustível:** Controle via hardware com precisão de microssegundos, operando em modo *Full Group* ou *Semi-Sequencial*.
- ⚡ **Sincronismo Avançado:** Decodificação de sinal de rotação com rastreamento de ângulo do virabrequim (0° a 720°).
- 📊 **Mapa 16x16 Interativo:** Tabela de injeção completa editável diretamente pelo LCD.
- 💾 **Persistência EEPROM:** Armazenamento automático de mapas e calibrações de sensores.
- 🖥️ **Interface HMI:** Menus dinâmicos para monitoramento (RPM, MAP, TPS, Tinj) e ajuste fino sem necessidade de PC.

### 📂 Estrutura Modular
- `Crank.cpp/h`: Gerenciamento de interrupções de rotação e cálculo de ângulo.
- `Injector.cpp/h`: Escalonador de injeção baseado em posição angular e tempo.
- `Density_EFI.ino`: Orquestrador da interface de usuário e integração de módulos.

---

## 🇺🇸 English

**Density EFI** is an open-source engine management system (ECU) for the **Arduino Mega 2560**. This version utilizes a modular architecture to ensure real-time precision and stable engine control.

### 🚀 Current Features
- ⛽ **Fuel Injection:** Hardware-level control with microsecond precision, supporting *Full Group* or *Semi-Sequential* modes.
- ⚡ **Advanced Synchronization:** Rotation signal decoding with crank angle tracking (0° to 720°).
- 📊 **Interactive 16x16 Map:** Full fuel table editable directly via the LCD interface.
- 💾 **EEPROM Persistence:** Automatic storage of maps and sensor calibrations.
- 🖥️ **HMI Interface:** Dynamic menus for real-time monitoring (RPM, MAP, TPS, Tinj) and fine-tuning without a PC.



### 📌 Pinagem de Referência / Pinout (Mega 2560)

| Função / Function | Pino / Pin | Nota / Note |
| :--- | :--- | :--- |
| **RPM Signal** | D2 (ou D21) | Entrada de Interrupção |
| **Injector Out** | D10 (ou D22) | Saída p/ Driver MOSFET |
| **MAP Sensor** | A4 | Entrada Analógica |
| **TPS Sensor** | A3 | Entrada Analógica |
| **LCD Pins** | 8, 9, 4, 5, 6, 7 | Interface 4-bits |
| **Buttons** | A0 | Escada de Resistores (Keypad) |



---

### ⚠️ Disclaimer
O ajuste de parâmetros do motor pode resultar em **danos mecânicos graves**. Este projeto tem fins educacionais. Use por sua conta e risco. / Adjusting engine parameters can result in **serious mechanical damage**. This project is for educational purposes. Use at your own risk.
