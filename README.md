# 🏎️ Density EFI - Engine Management System (v1.0)

[Read in English](#english) | [Ler em Português](#português)

---

## 🇧🇷 Português

O **Density EFI** é um sistema de controle de injeção eletrônica (ECU) de código aberto para a plataforma **Arduino Mega 2560**. Esta versão foca em interface física direta, permitindo ajustes em tempo real sem a necessidade de um PC.

### 🚀 Funcionalidades Atuais
- ⛽ **Injeção de Combustível:** Controle via hardware (**Timer 3**) com precisão de microssegundos.
- ⚡ **Sincronismo:** Decodificação de roda fônica **60-2** via interrupção externa (Pino 21).
- 📊 **Mapa 16x16 Editável:** Tabela de injeção completa com interpolação 2D (RPM x MAP).
- 💾 **Persistência EEPROM:** Salva e recupera mapas de injeção automaticamente ao ligar.
- 🖥️ **Menu Carrossel (LCD 16x2):** - **Navegação:** UP/DOWN para trocar de menu, SELECT para entrar, LEFT para voltar.
  - **Aceleração de Botão:** Ao manter pressionado UP/DOWN no mapa, a velocidade de incremento aumenta.
  - **Monitoramento:** Tela dedicada para RPM, TPS, MAP e Tempo de Injeção (Tinj).
  - **Editor de Mapa:** Ajuste fino de célula a célula com precisão de 0.01ms.

### 📌 Pinagem de Referência (Arduino Mega 2560)
| Função | Pino Arduino | Observação |
| :--- | :--- | :--- |
| 🔄 **Sinal de Rotação** | D21 (INT0) | Roda Fônica 60-2 |
| 🏭 **Sensor MAP** | A4 | Analog In (0 a -1.0 bar) |
| 🏎️ **Sensor TPS** | A3 | Analog In (0% a 100%) |
| 💨 **Saída Injetor** | D22 | Digital Out (Timer 3) |
| 🔘 **Botões Shield** | A0 | Keypad Shield (Resistor Ladder) |

---

## 🇺🇸 English

**Density EFI** is an open-source engine management system (ECU) for the **Arduino Mega 2560**. This version focuses on a direct hardware interface, allowing real-time tuning without a PC.

### 🚀 Current Features
- ⛽ **Fuel Injection:** Hardware-controlled pulse width (**Timer 3**) with microsecond precision.
- ⚡ **Synchronization:** **60-2** trigger wheel decoding via external interrupt (Pin 21).
- 📊 **Editable 16x16 Map:** Full fuel table with 2D interpolation (RPM vs MAP).
- 💾 **EEPROM Persistence:** Automatically saves and restores maps on startup.
- 🖥️ **Carousel Menu (16x2 LCD):** - **Navigation:** UP/DOWN to switch menus, SELECT to enter, LEFT to exit.
  - **Button Acceleration:** Holding UP/DOWN in map mode increases increment speed.
  - **Monitoring:** Dedicated screen for RPM, TPS, MAP, and Injection Time (Tinj).
  - **Map Editor:** Fine-tuning cell-by-cell with 0.01ms precision.



### 📌 Pinout Reference (Arduino Mega 2560)
| Function | Arduino Pin | Note |
| :--- | :--- | :--- |
| 🔄 **RPM Signal** | D21 (INT0) | 60-2 Trigger Wheel |
| 🏭 **MAP Sensor** | A4 | Analog In (0 to -1.0 bar) |
| 🏎️ **TPS Sensor** | A3 | Analog In (0% to 100%) |
| 💨 **Injector Output** | D22 | Digital Out (Timer 3) |
| 🔘 **Shield Buttons** | A0 | Keypad Shield (Resistor Ladder) |

---

### ⚠️ Disclaimer
O ajuste de parâmetros do motor pode resultar em **danos mecânicos graves**. Este projeto tem fins educacionais. Use por sua conta e risco. / Adjusting engine parameters can result in **serious mechanical damage**. This project is for educational purposes. Use at your own risk.
