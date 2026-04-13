import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import numpy as np
import datetime

class IgnGeneratorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Gerador de Ignição Speeduino - Sidney Himuro")
        self.root.geometry("1200x700")
        self.root.configure(bg="black")

        main_frame = tk.Frame(root, bg="black")
        main_frame.pack(fill="both", expand=True)

        # =========================
        # LADO ESQUERDO (COM SCROLL)
        # =========================
        left_canvas = tk.Canvas(main_frame, bg="#111", width=300, highlightthickness=0)
        left_canvas.pack(side="left", fill="y")

        scrollbar = tk.Scrollbar(main_frame, orient="vertical", command=left_canvas.yview)
        scrollbar.pack(side="left", fill="y")

        left_canvas.configure(yscrollcommand=scrollbar.set)
        left_canvas.bind('<Configure>', lambda e: left_canvas.configure(scrollregion=left_canvas.bbox("all")))

        left_frame = tk.Frame(left_canvas, bg="#111", width=300)
        left_canvas.create_window((0, 0), window=left_frame, anchor="nw")

        def _on_mousewheel(event):
            left_canvas.yview_scroll(int(-1*(event.delta/120)), "units")
        left_canvas.bind_all("<MouseWheel>", _on_mousewheel)

        tk.Label(left_frame, text="Gerador de Ignição",
                 font=("Arial", 18, "bold"),
                 fg="white", bg="#111").pack(pady=(10, 0))

        tk.Label(left_frame, text="** Himuro Performance **",
                 font=("Courier New", 10, "italic"),
                 fg="#00ffcc", bg="#111").pack(pady=(0, 10))

        # ========== CAMPOS DE ENTRADA ==========
        self.inputs = [
            ("RPM Marcha Lenta", "800"),
            ("Avanço em Marcha Lenta (graus)", "10"),
            ("Avanço Máximo (graus)", "38"),
            ("Avanço no Pico de Torque (graus)", "28"),
            ("RPM Torque Máximo", "4000"),
            ("Limite Máximo RPM", "6500"),
            ("Expoente da Curva de Avanço", "1.2"),       # novo: não linearidade
            ("Retardo por Carga Máxima (graus)", "5"),    # novo: retard em alta carga
            ("Pressão Turbo (BAR)", "0.0"),
            ("Retardo por Turbo (graus/BAR)", "6"),
            ("Expoente Retardo Boost", "1.2"),            # novo: progressividade boost
            ("MAP Mínimo (kPa)", "30"),
            ("MAP Marcha Lenta (kPa)", "40"),
            ("Taxa de Compressão (:1)", "10.0")
        ]

        self.combo_fields = {
            "Tipo de Admissão": {"options": ["TBI comum", "ITB (Throttle body)"], "default": "TBI comum", "map": {"TBI comum": 0, "ITB (Throttle body)": 1}},
            "Modo de Carga": {"options": ["MAP", "TPS"], "default": "MAP", "map": {"MAP": 0, "TPS": 1}}
        }

        self.entries = {}
        self.comboboxes = {}

        for label_text, default_val in self.inputs:
            frame = tk.Frame(left_frame, bg="#111")
            frame.pack(fill="x", padx=10, pady=4)
            tk.Label(frame, text=label_text, fg="white", bg="#111", anchor="w").pack()
            ent = tk.Entry(frame, bg="#222", fg="white", insertbackground="white")
            ent.insert(0, default_val)
            ent.pack(fill="x")
            self.entries[label_text] = ent

        for field_name, config in self.combo_fields.items():
            frame = tk.Frame(left_frame, bg="#111")
            frame.pack(fill="x", padx=10, pady=4)
            tk.Label(frame, text=field_name, fg="white", bg="#111", anchor="w").pack()
            combo = ttk.Combobox(frame, values=config["options"], state="readonly", background="#222")
            combo.set(config["default"])
            combo.pack(fill="x")
            self.comboboxes[field_name] = combo

        # Botão gerar
        tk.Button(left_frame,
                  text="GERAR TABELA DE IGNIÇÃO",
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

    # =========================
    def get_color(self, value, vmin=0, vmax=40):
        ratio = (value - vmin) / (vmax - vmin)
        ratio = max(0, min(1, ratio))

        if ratio < 0.33:
            r = 0
            g = int(255 * (ratio / 0.33))
            b = 255
        elif ratio < 0.66:
            r = int(255 * ((ratio - 0.33) / 0.33))
            g = 255
            b = int(255 * (1 - (ratio - 0.33) / 0.33))
        else:
            r = 255
            g = int(255 * (1 - (ratio - 0.66) / 0.34))
            b = 0
        return f'#{r:02x}{g:02x}{b:02x}'

    # =========================
    def calcular_matriz_ignicao(self):
        d = {k: float(v.get()) for k, v in self.entries.items()}
        modo_str = self.comboboxes["Modo de Carga"].get()
        tipo_adm_str = self.comboboxes["Tipo de Admissão"].get()
        modo = self.combo_fields["Modo de Carga"]["map"][modo_str]
        tipo_adm = self.combo_fields["Tipo de Admissão"]["map"][tipo_adm_str]

        # Parâmetros
        idle_rpm = d["RPM Marcha Lenta"]
        idle_adv = d["Avanço em Marcha Lenta (graus)"]
        max_adv = d["Avanço Máximo (graus)"]
        torque_adv = d["Avanço no Pico de Torque (graus)"]
        rpm_torque_peak = d["RPM Torque Máximo"]
        rpm_max = d["Limite Máximo RPM"]
        advance_exp = d["Expoente da Curva de Avanço"]      # não linearidade
        max_load_retard = d["Retardo por Carga Máxima (graus)"]  # graus removidos em carga total
        boost_pressure = d["Pressão Turbo (BAR)"]
        boost_retard_per_bar = d["Retardo por Turbo (graus/BAR)"]
        boost_exp = d["Expoente Retardo Boost"]            # progressividade do boost
        map_min = max(10, d["MAP Mínimo (kPa)"])
        map_max = int((1.0 + boost_pressure) * 100)
        compression_ratio = d["Taxa de Compressão (:1)"]

        # ========== CORREÇÃO FÍSICA DA TAXA DE COMPRESSÃO ==========
        # Modelo: avanço ∝ 1 / sqrt(CR)  (maior CR queima mais rápido, menor avanço)
        ref_cr = 10.0
        factor_cr = np.sqrt(ref_cr / compression_ratio)
        factor_cr = np.clip(factor_cr, 0.75, 1.25)  # limites seguros

        # Eixos
        if modo == 0:
            eixo_carga = np.linspace(map_min, map_max, 16).astype(int)
        else:
            eixo_carga = np.linspace(5, 100, 16).astype(int)

        eixo_rpm = np.linspace(idle_rpm, rpm_max, 16).astype(int)
        matriz = np.zeros((16, 16))
        fator_adm = 0.98 if tipo_adm == 1 else 1.0

        for i, rpm in enumerate(eixo_rpm):
            # ========== CURVA NÃO LINEAR DE AVANÇO POR RPM ==========
            # Normaliza RPM entre 0 (marcha lenta) e 1 (limite máximo)
            rpm_norm = (rpm - idle_rpm) / (rpm_max - idle_rpm)
            rpm_norm = np.clip(rpm_norm, 0.0, 1.0)
            # Curva não linear com expoente configurável
            # Mapeia o avanço desejado nos pontos idle e torque peak e max
            # Usamos interpolação entre dois pontos principais com curva de potência
            if rpm <= rpm_torque_peak:
                # Do idle ao torque peak
                t = (rpm - idle_rpm) / (rpm_torque_peak - idle_rpm) if rpm_torque_peak > idle_rpm else 0
                # Aplica não linearidade: t ^ advance_exp
                t_curve = t ** advance_exp
                base_adv = idle_adv + t_curve * (torque_adv - idle_adv)
            else:
                # Do torque peak ao redline
                t = (rpm - rpm_torque_peak) / (rpm_max - rpm_torque_peak) if rpm_max > rpm_torque_peak else 0
                t_curve = t ** advance_exp
                base_adv = torque_adv + t_curve * (max_adv - torque_adv)

            base_adv = np.clip(base_adv, 0, 50)

            for j, carga_val in enumerate(eixo_carga):
                # ========== NORMALIZAÇÃO DA CARGA ==========
                if modo == 0:  # MAP
                    carga_norm = (carga_val - map_min) / (map_max - map_min) if map_max > map_min else 0
                    carga_norm = np.clip(carga_norm, 0.0, 1.0)
                    # Retardo por boost progressivo (não linear)
                    if boost_pressure > 0 and carga_val > 100:
                        boost_ratio = (carga_val - 100) / (map_max - 100) if map_max > 100 else 0
                        boost_ratio = np.clip(boost_ratio, 0.0, 1.0)
                        # Aplica expoente para tornar progressivo
                        boost_ratio_curve = boost_ratio ** boost_exp
                        boost_retard = boost_retard_per_bar * boost_pressure * boost_ratio_curve
                    else:
                        boost_retard = 0
                else:  # TPS
                    carga_norm = carga_val / 100.0
                    if boost_pressure > 0 and carga_val > 80:
                        boost_ratio = (carga_norm - 0.8) / 0.2
                        boost_ratio = np.clip(boost_ratio, 0.0, 1.0)
                        boost_ratio_curve = boost_ratio ** boost_exp
                        boost_retard = boost_retard_per_bar * boost_pressure * boost_ratio_curve
                    else:
                        boost_retard = 0

                # ========== RETARDO POR CARGA (estilo ECU real) ==========
                # Aplica retard linear (ou com curva) desde carga baixa até carga máxima
                # Usa a mesma normalização de carga (0..1)
                load_retard = max_load_retard * carga_norm

                # Fator de correção de mistura para ITB (ligeiramente diferente)
                if tipo_adm == 1:
                    # ITB tende a precisar de um pouco menos de retard por carga
                    load_retard = load_retard * 0.9

                # ========== AVANÇO FINAL ==========
                ign_adv = base_adv * fator_adm * factor_cr - boost_retard - load_retard

                # Limites realistas
                matriz[j, i] = int(np.clip(ign_adv, 5, 45))

        return eixo_rpm, eixo_carga, matriz

    # =========================
    def gerar_tudo(self):
        try:
            rpms, cargas, matriz = self.calcular_matriz_ignicao()
        except Exception as e:
            messagebox.showerror("Erro no cálculo", str(e))
            return

        for widget in self.frame_result.winfo_children():
            widget.destroy()

        table_frame = tk.Frame(self.frame_result, bg="black")
        table_frame.pack(pady=10)

        linhas, colunas = 16, 16
        ign_max = np.max(matriz)

        for i in range(linhas + 1):
            for j in range(colunas + 1):
                if i == linhas and j == 0:
                    text, bg, fg = "", "#333", "white"
                elif i == linhas and j > 0:
                    text, bg, fg = str(rpms[j-1]), "#222", "white"
                elif j == 0 and i < linhas:
                    text, bg, fg = str(cargas[linhas-1-i]), "#222", "white"
                else:
                    val = int(matriz[linhas-1-i][j-1])
                    text, bg, fg = str(val), self.get_color(val, vmin=5, vmax=max(ign_max, 35)), "black"

                tk.Label(table_frame, text=text, width=5, height=2, bg=bg, fg=fg,
                         borderwidth=1, relief="solid", font=("Arial", 9, "bold")
                         ).grid(row=i, column=j, padx=1, pady=1)

        self.salvar_arquivo(rpms, cargas, matriz)

    # =========================
    def salvar_arquivo(self, rpms, cargas, matriz):
        try:
            file_path = filedialog.asksaveasfilename(
                defaultextension=".table",
                filetypes=[("Ignition Table", "*.table")]
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
                f.write('<yAxis name="ignLoad" rows="16">\n')
                for val in cargas:
                    f.write(f' {val}\n')
                f.write('</yAxis>\n')
                f.write('<zValues cols="16" rows="16">\n')
                for row in matriz:
                    f.write(" ".join(map(str, row)) + "\n")
                f.write('</zValues>\n')
                f.write('<tr>\n</tableData>')
            messagebox.showinfo("Sucesso", "Tabela de ignição gerada!")
        except Exception as e:
            messagebox.showerror("Erro", str(e))

if __name__ == "__main__":
    root = tk.Tk()
    app = IgnGeneratorApp(root)
    root.mainloop()