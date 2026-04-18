import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import numpy as np
import datetime

class VEGeneratorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Gerador VEX Speeduino - Sidney Himuro")
        self.root.geometry("1200x700")
        self.root.configure(bg="black")

        main_frame = tk.Frame(root, bg="black")
        main_frame.pack(fill="both", expand=True)

        # =========================
        # LADO ESQUERDO
        # =========================
        left_frame = tk.Frame(main_frame, bg="#111", width=300)
        left_frame.pack(side="left", fill="y")

        tk.Label(left_frame, text="Gerador de VE",
                 font=("Arial", 18, "bold"),
                 fg="white", bg="#111").pack(pady=(10, 0))

        tk.Label(left_frame, text="** Himuro Performance **",
                 font=("Courier New", 10, "italic"),
                 fg="#00ffcc", bg="#111").pack(pady=(0, 10))

        # Container para campos dinâmicos
        self.dynamic_frame = tk.Frame(left_frame, bg="#111")
        self.dynamic_frame.pack(fill="x")

        # Campos com opções (combobox)
        self.combo_fields = {
            "Tipo de Admissão": {"options": ["TBI comum", "ITB (Throttle body)"], "default": "TBI comum", "map": {"TBI comum": 0, "ITB (Throttle body)": 1}},
            "Modo de Carga": {"options": ["MAP", "TPS"], "default": "MAP", "map": {"MAP": 0, "TPS": 1}}
        }

        self.entries = {}      # para Entry normais
        self.comboboxes = {}   # para Combobox
        self.labels_ref = {}   # para referência dos labels
        self.tps_fields = {}   # para armazenar referência dos campos TPS (ocultos)

        # Criar campos Combobox primeiro
        for field_name, config in self.combo_fields.items():
            frame = tk.Frame(left_frame, bg="#111")
            frame.pack(fill="x", padx=10, pady=4)

            tk.Label(frame, text=field_name,
                     fg="white", bg="#111", anchor="w").pack()

            combo = ttk.Combobox(frame, values=config["options"], state="readonly", background="#222")
            combo.set(config["default"])
            combo.pack(fill="x")
            
            if field_name == "Modo de Carga":
                combo.bind('<<ComboboxSelected>>', self.on_modo_carga_change)

            self.comboboxes[field_name] = combo

        # Separador
        tk.Frame(left_frame, bg="#333", height=2).pack(fill="x", pady=10, padx=10)

        # Criar campos TPS ocultos com valores padrão
        self.create_hidden_tps_fields()
        
        # Criar campos dinâmicos baseados no modo selecionado
        self.create_dynamic_fields("MAP")

        # Botão gerar
        tk.Button(left_frame,
                  text="GERAR TABELA",
                  command=self.gerar_tudo,
                  bg="#1f6feb",
                  fg="white",
                  font=("Arial", 11, "bold"),
                  height=2).pack(pady=20, padx=10, fill="x")

        # =========================
        # LADO DIREITO
        # =========================
        self.frame_result = tk.Frame(main_frame, bg="black")
        self.frame_result.pack(side="right", fill="both", expand=True)

    def create_hidden_tps_fields(self):
        """Cria campos TPS ocultos com valores padrão"""
        # Frame oculto para campos TPS
        self.hidden_frame = tk.Frame(self.root)
        
        tps_defaults = [
            ("TPS Marcha Lenta (%)", "0"),
            ("TPS Mínimo (%)", "0"),
            ("TPS Máximo (%)", "100"),
        ]
        
        for label_text, default_val in tps_defaults:
            ent = tk.Entry(self.hidden_frame)
            ent.insert(0, default_val)
            self.tps_fields[label_text] = ent

    def create_dynamic_fields(self, modo):
        # Limpar campos dinâmicos existentes
        for widget in self.dynamic_frame.winfo_children():
            widget.destroy()
        
        self.entries.clear()
        self.labels_ref.clear()
        
        # Campos comuns para ambos os modos
        common_fields = [
            ("RPM Marcha Lenta", "800"),
            ("Limite Máximo RPM", "6500"),
            ("RPM Torque Máximo", "4000"),
            ("Deslocamento (Litros)", "1.8"),
            ("VE Máximo (%)", "95"),
        ]
        
        # Campos específicos do modo
        if modo == "MAP":
            specific_fields = [
                ("MAP Marcha Lenta (kPa)", "40"),
                ("MAP Mínimo (kPa)", "30"),
                ("Pressão Turbo (BAR)", "0.0"),
            ]
        else:  # TPS - mostrar campo VE lenta
            specific_fields = [
                ("VE lenta TPS 0%", "0"),
            ]
        
        # Combinar todos os campos
        all_fields = common_fields + specific_fields
        
        # Criar os campos visíveis
        for label_text, default_val in all_fields:
            frame = tk.Frame(self.dynamic_frame, bg="#111")
            frame.pack(fill="x", padx=10, pady=4)

            label = tk.Label(frame, text=label_text,
                           fg="white", bg="#111", anchor="w")
            label.pack()

            ent = tk.Entry(frame, bg="#222", fg="white", insertbackground="white")
            ent.insert(0, default_val)
            ent.pack(fill="x")

            self.entries[label_text] = ent
            self.labels_ref[label_text] = label
        
        # Se modo TPS, adicionar uma label informativa
        if modo == "TPS":
            info_frame = tk.Frame(self.dynamic_frame, bg="#111")
            info_frame.pack(fill="x", padx=10, pady=10)
            
            tk.Label(info_frame, 
                    text="Modo TPS: usando valores padrão\nTPS: 0% a 100%\n(Deixe 0 em VE lenta para cálculo automático)",
                    fg="#00ffcc", 
                    bg="#111", 
                    font=("Arial", 9, "italic"),
                    justify="center").pack()

    def on_modo_carga_change(self, event=None):
        modo_selecionado = self.comboboxes["Modo de Carga"].get()
        self.create_dynamic_fields(modo_selecionado)

    # =========================
    def get_color(self, value, vmin=25, vmax=95):
        ratio = (value - vmin) / (vmax - vmin)
        ratio = max(0, min(1, ratio))

        if ratio < 0.5:
            r = int(ratio * 2 * 255)
            g = 255
            b = int(255 - ratio * 2 * 255)
        else:
            r = 255
            g = int(255 - (ratio - 0.5) * 2 * 255)
            b = 0

        return f'#{r:02x}{g:02x}{b:02x}'

    def interpolar_valores_proximos(self, matriz, rpm_idx, carga_inicial, valor_alvo):
        """Interpola valores próximos ao valor alvo na matriz"""
        linhas, colunas = matriz.shape
        
        # Aplica o valor alvo na posição específica
        matriz[carga_inicial, rpm_idx] = valor_alvo
        
        # Interpola para cargas próximas
        if carga_inicial > 0:
            # Interpolar para cargas menores
            for c in range(carga_inicial - 1, -1, -1):
                fator = (c + 1) / (carga_inicial + 1)
                matriz[c, rpm_idx] = max(20, int(valor_alvo * (0.7 + 0.3 * fator)))
        
        if carga_inicial < linhas - 1:
            # Interpolar para cargas maiores
            for c in range(carga_inicial + 1, linhas):
                fator = (c - carga_inicial) / (linhas - carga_inicial)
                # Aumenta gradualmente até o valor calculado originalmente
                valor_original = matriz[c, rpm_idx]
                if valor_original > valor_alvo:
                    matriz[c, rpm_idx] = int(valor_alvo + (valor_original - valor_alvo) * fator)
        
        # Interpola para RPMs próximos
        for c in range(linhas):
            if c != carga_inicial:
                # Interpolar horizontalmente (RPMs próximos)
                for r in range(colunas):
                    if r != rpm_idx:
                        distancia_rpm = abs(r - rpm_idx)
                        if distancia_rpm <= 2:  # Interpolar até 2 colunas de distância
                            fator_rpm = 1.0 - (distancia_rpm * 0.15)
                            valor_original = matriz[c, r]
                            matriz[c, r] = int(valor_original * (0.7 + 0.3 * fator_rpm))
        
        return matriz

    # =========================
    def calcular_matriz(self):
        # Lê valores dos Entry visíveis
        d = {k: float(v.get()) for k, v in self.entries.items()}
        
        # Adiciona valores TPS do frame oculto quando necessário
        modo_str = self.comboboxes["Modo de Carga"].get()
        if modo_str == "TPS":
            for k, v in self.tps_fields.items():
                d[k] = float(v.get())

        # Lê valores dos Combobox (converte para inteiro)
        tipo_adm_str = self.comboboxes["Tipo de Admissão"].get()

        modo = self.combo_fields["Modo de Carga"]["map"][modo_str]
        tipo_adm = self.combo_fields["Tipo de Admissão"]["map"][tipo_adm_str]

        # Configurar eixos baseado no modo
        if modo == 0:  # MAP
            map_min = max(10, d["MAP Mínimo (kPa)"])
            map_max = int((1.0 + d["Pressão Turbo (BAR)"]) * 100)
            
            # Garantir que map_min não ultrapasse map_max
            if map_min >= map_max:
                map_min = max(10, map_max - 10)
                messagebox.showwarning("Aviso", "MAP mínimo ajustado para menos que MAP máximo.")
            
            eixo_carga = np.linspace(map_min, map_max, 16).astype(int)
            
        else:  # TPS - usando valores fixos do frame oculto
            tps_min = float(self.tps_fields["TPS Mínimo (%)"].get())
            tps_max = float(self.tps_fields["TPS Máximo (%)"].get())
            
            eixo_carga = np.linspace(tps_min, tps_max, 16).astype(int)

        eixo_rpm = np.linspace(d["RPM Marcha Lenta"], d["Limite Máximo RPM"], 16).astype(int)
        
        matriz = np.zeros((16, 16))

        ve_max_base = d["VE Máximo (%)"]
        desloc = d["Deslocamento (Litros)"]
        fator_desloc = 1.0 + (desloc - 1.8) * 0.05
        fator_desloc = np.clip(fator_desloc, 0.85, 1.15)

        ve_max = ve_max_base * fator_desloc

        fator_admissao = 0.95 if tipo_adm == 0 else 1.05

        # Primeiro, calcula a matriz normalmente
        for i, rpm in enumerate(eixo_rpm):
            if rpm <= d["RPM Torque Máximo"]:
                fator_rpm = 0.6 + 0.4 * (rpm / d["RPM Torque Máximo"])
            else:
                queda = (rpm - d["RPM Torque Máximo"]) / (d["Limite Máximo RPM"] - d["RPM Torque Máximo"])
                fator_rpm = 1.0 - (0.25 * queda)

            for j, carga_val in enumerate(eixo_carga):
                if modo == 0:  # MAP
                    carga_norm = (carga_val - eixo_carga[0]) / (eixo_carga[-1] - eixo_carga[0])
                    carga_norm = np.clip(carga_norm, 0.0, 1.0)
                else:  # TPS
                    carga_norm = carga_val / 100.0  # Normalizado para 0-1 baseado em 100%
                    carga_norm = np.clip(carga_norm, 0.0, 1.0)
                    if tipo_adm == 1:
                        carga_norm = carga_norm ** 1.3
                    else:
                        carga_norm = carga_norm ** 1.1

                # Fator de carga com curva suave
                fator_carga = 0.35 + 0.65 * (carga_norm ** 0.7)
                if carga_norm > 0.95:
                    fator_carga = min(fator_carga, 1.0)

                ve = ve_max * fator_rpm * fator_carga * fator_admissao
                matriz[j, i] = int(np.clip(ve, 20, ve_max_base * 1.1))

        # Se modo TPS e tem valor de VE lenta definido
        if modo == 1 and "VE lenta TPS 0%" in d:
            ve_lenta = d["VE lenta TPS 0%"]
            if ve_lenta > 0:
                # Encontra o índice da RPM mais próxima da marcha lenta
                rpm_lenta = d["RPM Marcha Lenta"]
                rpm_idx = np.argmin(np.abs(eixo_rpm - rpm_lenta))
                
                # Encontra o índice da carga mais baixa (0% TPS)
                carga_idx = 0  # Primeira linha corresponde à menor carga
                
                # Guarda uma cópia da matriz original para referência
                matriz_original = matriz.copy()
                
                # Aplica a interpolação
                matriz = self.interpolar_valores_proximos(matriz, rpm_idx, carga_idx, ve_lenta)
                
                # Suaviza transições
                matriz = self.suavizar_transicoes(matriz, matriz_original, rpm_idx, carga_idx)

        return eixo_rpm, eixo_carga, matriz

    def suavizar_transicoes(self, matriz, matriz_original, rpm_idx_ref, carga_idx_ref):
        """Suaviza as transições entre valores interpolados e originais"""
        linhas, colunas = matriz.shape
        
        for i in range(linhas):
            for j in range(colunas):
                # Calcula distância do ponto de referência
                dist_carga = abs(i - carga_idx_ref) / linhas
                dist_rpm = abs(j - rpm_idx_ref) / colunas
                distancia_total = np.sqrt(dist_carga**2 + dist_rpm**2)
                
                # Se está longe o suficiente, mantém o valor original
                if distancia_total > 0.3:
                    matriz[i, j] = matriz_original[i, j]
                # Se está numa zona de transição, faz um blend
                elif distancia_total > 0.15:
                    fator_blend = (distancia_total - 0.15) / 0.15
                    matriz[i, j] = int(matriz[i, j] * (1 - fator_blend) + matriz_original[i, j] * fator_blend)
        
        return matriz

    # =========================
    def gerar_tudo(self):
        try:
            rpms, cargas, matriz = self.calcular_matriz()
        except Exception as e:
            messagebox.showerror("Erro no cálculo", str(e))
            return

        for widget in self.frame_result.winfo_children():
            widget.destroy()

        table_frame = tk.Frame(self.frame_result, bg="black")
        table_frame.pack(pady=10)

        linhas = 16
        colunas = 16
        # Valor para escala de cor (usar VE máximo do cálculo)
        ve_max_calc = np.max(matriz)

        for i in range(linhas + 1):
            for j in range(colunas + 1):
                if i == linhas and j == 0:
                    text = ""
                    bg = "#333"
                    fg = "white"
                elif i == linhas and j > 0:
                    text = str(rpms[j - 1])
                    bg = "#222"
                    fg = "white"
                elif j == 0 and i < linhas:
                    text = str(cargas[linhas - 1 - i])
                    bg = "#222"
                    fg = "white"
                else:
                    val = int(matriz[linhas - 1 - i][j - 1])
                    text = str(val)
                    bg = self.get_color(val, vmin=20, vmax=ve_max_calc)
                    fg = "black"

                tk.Label(
                    table_frame,
                    text=text,
                    width=5,
                    height=2,
                    bg=bg,
                    fg=fg,
                    borderwidth=1,
                    relief="solid",
                    font=("Arial", 9, "bold")
                ).grid(row=i, column=j, padx=1, pady=1)

        self.salvar_vex(rpms, cargas, matriz)

    # =========================
    def salvar_vex(self, rpms, cargas, matriz):
        try:
            file_path = filedialog.asksaveasfilename(
                defaultextension=".table",
                filetypes=[("VEX Table", "*.table")]
            )
            if not file_path:
                return

            now = datetime.datetime.now().strftime("%a %b %d %H:%M:%S BRT %Y")

            with open(file_path, "w", encoding="UTF-8") as f:
                f.write('<?xml version="1.0" encoding="UTF-8" standalone="no"?>\n')
                f.write('<tableData xmlns="http://www.EFIAnalytics.com/:table">\n')
                f.write(f'<bibliography author="Sidney Himuro" writeDate="{now}"/>\n')
                f.write('<versionInfo fileFormat="1.0"/>\n')
                f.write('<table cols="16" rows="16">\n')

                f.write('<xAxis cols="16" name="rpm">\n')
                for val in rpms:
                    f.write(f' {val}\n')
                f.write('</xAxis>\n')

                f.write('<yAxis name="fuelLoad" rows="16">\n')
                for val in cargas:
                    f.write(f' {val}\n')
                f.write('</yAxis>\n')

                f.write('<zValues cols="16" rows="16">\n')
                for row in matriz:
                    f.write(" ".join(map(str, row)) + "\n")
                f.write('</zValues>\n')

                f.write('</table>\n</tableData>')

            messagebox.showinfo("Sucesso", "Tabela gerada com sucesso!")
        except Exception as e:
            messagebox.showerror("Erro", str(e))


if __name__ == "__main__":
    root = tk.Tk()
    app = VEGeneratorApp(root)
    root.mainloop()
