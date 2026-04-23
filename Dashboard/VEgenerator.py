import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import numpy as np
import datetime

class VEGeneratorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Gerador VEX Speeduino - Sidney Himuro")
        self.root.geometry("1200x800")
        self.root.configure(bg="black")

        main_frame = tk.Frame(root, bg="black")
        main_frame.pack(fill="both", expand=True)

        # --- LADO ESQUERDO (CONTROLES) ---
        left_frame = tk.Frame(main_frame, bg="#111", width=350)
        left_frame.pack(side="left", fill="y")

        tk.Label(left_frame, text="Gerador de VE Profissional",
                 font=("Arial", 14, "bold"), fg="white", bg="#111").pack(pady=(15, 0))
        tk.Label(left_frame, text="** Himuro Performance Logic **",
                 font=("Courier New", 9, "italic"), fg="#00ffcc", bg="#111").pack(pady=(0, 10))

        self.canvas = tk.Canvas(left_frame, bg="#111", highlightthickness=0)
        self.scrollbar = ttk.Scrollbar(left_frame, orient="vertical", command=self.canvas.yview)
        self.scrollable_frame = tk.Frame(self.canvas, bg="#111")

        self.scrollable_frame.bind("<Configure>", lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all")))
        self.canvas.create_window((0, 0), window=self.scrollable_frame, anchor="nw", width=320)
        self.canvas.configure(yscrollcommand=self.scrollbar.set)
        self.canvas.pack(side="top", fill="both", expand=True, padx=5)
        self.scrollbar.pack(side="right", fill="y")

        # BOTÃO FIXO NO RODAPÉ
        btn_container = tk.Frame(left_frame, bg="#111")
        btn_container.pack(side="bottom", fill="x", padx=15, pady=15)

        tk.Button(btn_container, text="GERAR E SALVAR TABELA (.table)",
                  command=self.gerar_e_salvar,
                  bg="#1f6feb", fg="white", font=("Arial", 11, "bold"),
                  height=2, cursor="hand2").pack(fill="x")

        self.comboboxes = {}
        self.entries = {}
        self.init_interface()

        # --- LADO DIREITO (PREVIEW) ---
        self.frame_result = tk.Frame(main_frame, bg="black")
        self.frame_result.pack(side="right", fill="both", expand=True)

    def init_interface(self):
        modes = {"Tipo de Admissão": ["TBI Comum", "ITB"], "Modo de Carga": ["MAP", "TPS"]}
        for field, options in modes.items():
            tk.Label(self.scrollable_frame, text=field, fg="#aaa", bg="#111", font=("Arial", 9)).pack(anchor="w", pady=(5,0))
            cb = ttk.Combobox(self.scrollable_frame, values=options, state="readonly")
            cb.set(options[0])
            cb.pack(fill="x", pady=(0, 5))
            self.comboboxes[field] = cb
        
        self.comboboxes["Modo de Carga"].bind('<<ComboboxSelected>>', self.refresh_inputs)
        self.refresh_inputs()

    def refresh_inputs(self, event=None):
        if hasattr(self, 'inputs_sub_frame'):
            self.inputs_sub_frame.destroy()
        
        self.inputs_sub_frame = tk.Frame(self.scrollable_frame, bg="#111")
        self.inputs_sub_frame.pack(fill="x")
        self.entries.clear()

        # Campo Fator RPM atualizado com "ITB Original"
        tk.Label(self.inputs_sub_frame, text="Fator RPM (Perfil de Comando)", fg="#aaa", bg="#111", font=("Arial", 9)).pack(anchor="w", pady=(5,0))
        self.combo_perfil = ttk.Combobox(self.inputs_sub_frame, state="readonly", 
                                         values=["TBI Original", "TBI + Comando Médio", "TBI + Comando Alta", "ITB Original", "ITB + Comando Médio", "ITB + Comando Alta"])
        self.combo_perfil.set("TBI Original")
        self.combo_perfil.pack(fill="x", pady=5)

        modo = self.comboboxes["Modo de Carga"].get()
        fields = [
            ("RPM Marcha Lenta", "850"),
            ("Limite Máximo RPM", "6500"),
            ("Potência Máxima (HP)", "130"),
            ("Torque Máximo (Kgfm)", "18.0"),
            ("RPM Torque Máximo", "4000"),
            ("Deslocamento (Litros)", "2.0"),
            ("VE Máximo (Teto %)", "95"),
            ("Fator de Carga Inicial", "0.35")
        ]

        if modo == "MAP":
            fields += [
                ("MAP Marcha Lenta (kPa)", "40"), 
                ("MAP Mínimo (kPa)", "25"), 
                ("MAP Máximo da Tabela", "100"), # Campo solicitado
                ("Pressão Turbo (BAR)", "0.0"),
                ("VE Lenta MAP", "0")
            ]
        else:
            fields += [("VE lenta TPS 0%", "0")]
        
        for label, default in fields:
            tk.Label(self.inputs_sub_frame, text=label, fg="#ccc", bg="#111", font=("Arial", 8)).pack(anchor="w")
            ent = tk.Entry(self.inputs_sub_frame, bg="#222", fg="white", borderwidth=0, insertbackground="white")
            ent.insert(0, default)
            ent.pack(fill="x", pady=(0, 5), ipady=3)
            self.entries[label] = ent

    def get_safe_color(self, val, vmin, vmax):
        if vmax <= vmin: ratio = 0
        else: ratio = (val - vmin) / (vmax - vmin)
        ratio = max(0, min(1, ratio))
        r = int(255 * ratio)
        g = int(255 * (1 - ratio))
        return f'#{r:02x}{g:02x}44'

    def calcular(self):
        d = {k: float(v.get()) for k, v in self.entries.items()}
        ve_max = d["VE Máximo (Teto %)"]
        perfil = self.combo_perfil.get()
        
        perfis_cfg = {
            "TBI Original": (0.40, 0.5),
            "TBI + Comando Médio": (0.30, 0.6),
            "TBI + Comando Alta": (0.20, 0.7),
            "ITB Original": (0.25, 0.75), # Perfil ITB Original
            "ITB + Comando Médio": (0.15, 0.8),
            "ITB + Comando Alta": (0.08, 0.9)
        }
        queda, curva = perfis_cfg[perfil]

        eixo_rpm = np.linspace(d["RPM Marcha Lenta"], d["Limite Máximo RPM"], 16).astype(int)
        
        modo = self.comboboxes["Modo de Carga"].get()
        if modo == "MAP":
            map_max_manual = d.get("MAP Máximo da Tabela", 100)
            eixo_carga = np.linspace(d["MAP Mínimo (kPa)"], map_max_manual, 16).astype(int)
            boost = d.get("Pressão Turbo (BAR)", 0.0)
            limite_real = ve_max if boost < 0.1 else ve_max * 1.5
            lenta_val = d.get("VE Lenta MAP", 0)
        else:
            eixo_carga = np.linspace(0, 100, 16).astype(int)
            limite_real = ve_max
            lenta_val = d.get("VE lenta TPS 0%", 0)

        matriz = np.zeros((16, 16))
        for i, rpm in enumerate(eixo_rpm):
            if rpm <= d["RPM Torque Máximo"]:
                f_rpm = 0.70 + 0.30 * (rpm / d["RPM Torque Máximo"])
            else:
                prog = (rpm - d["RPM Torque Máximo"]) / (d["Limite Máximo RPM"] - d["RPM Torque Máximo"])
                f_rpm = 1.0 - (queda * (prog ** curva))

            for j, carga in enumerate(eixo_carga):
                c_norm = (carga - eixo_carga[0]) / (eixo_carga[-1] - eixo_carga[0])
                f_carga = d["Fator de Carga Inicial"] + (1.0 - d["Fator de Carga Inicial"]) * (c_norm ** 0.8)
                matriz[j, i] = np.clip(ve_max * f_rpm * f_carga, 20, limite_real)

        if lenta_val > 0:
            c_idx = np.argmin(np.abs(eixo_carga - d.get("MAP Marcha Lenta (kPa)", 40))) if modo == "MAP" else 0
            rpm_idx = 0 
            for j in range(16):
                for i in range(16):
                    dist = np.sqrt((i - rpm_idx)**2 + (j - c_idx)**2)
                    if dist < 5:
                        f = 1 - (dist / 5)
                        matriz[j, i] = (matriz[j, i] * (1 - f)) + (lenta_val * f)

        return eixo_rpm, eixo_carga, matriz

    def gerar_e_salvar(self):
        try:
            self.current_rpms, self.current_cargas, self.current_matriz = self.calcular()
            self.exibir_preview()
            path = filedialog.asksaveasfilename(defaultextension=".table", filetypes=[("VEX Table", "*.table")])
            if path: self.salvar_xml(path)
        except Exception as e:
            messagebox.showerror("Erro", f"Verifique os valores inseridos: {e}")

    def exibir_preview(self):
        for w in self.frame_result.winfo_children(): w.destroy()
        container = tk.Frame(self.frame_result, bg="black")
        container.pack(expand=True)
        vmax, vmin = np.max(self.current_matriz), np.min(self.current_matriz)
        for i in range(16):
            tk.Label(container, text=f"{self.current_cargas[15-i]}", fg="#00ffcc", bg="black", font=("Arial", 7), width=4).grid(row=i, column=0)
            for j in range(16):
                val = int(self.current_matriz[15-i, j])
                color = self.get_safe_color(val, vmin, vmax)
                tk.Label(container, text=str(val), bg=color, fg="white", width=4, height=2, font=("Arial", 8)).grid(row=i, column=j+1, padx=1, pady=1)
        for j in range(16):
            tk.Label(container, text=f"{self.current_rpms[j]}", fg="#00ffcc", bg="black", font=("Arial", 7)).grid(row=16, column=j+1, pady=5)

    def salvar_xml(self, path):
        now = datetime.datetime.now().strftime("%a %b %d %H:%M:%S BRT %Y")
        with open(path, "w", encoding="utf-8") as f:
            f.write('<?xml version="1.0" encoding="UTF-8" standalone="no"?>\n<tableData xmlns="http://www.EFIAnalytics.com/:table">\n')
            f.write(f'<bibliography author="Sidney Himuro" writeDate="{now}"/>\n<versionInfo fileFormat="1.0"/>\n<table cols="16" rows="16">\n')
            f.write('<xAxis cols="16" name="rpm">\n')
            for r in self.current_rpms: f.write(f'          {int(r)} \n')
            f.write('      </xAxis>\n<yAxis name="fuelLoad" rows="16">\n')
            for c in self.current_cargas: f.write(f'          {int(c)} \n')
            f.write('      </yAxis>\n<zValues cols="16" rows="16">\n')
            for row in self.current_matriz:
                f.write('          ' + " ".join([f"{v:.1f}" for v in row]) + '\n')
            f.write('      </zValues>\n</table>\n</tableData>')
        messagebox.showinfo("Sucesso", "Tabela salva com sucesso!")

if __name__ == "__main__":
    root = tk.Tk()
    app = VEGeneratorApp(root)
    root.mainloop()
