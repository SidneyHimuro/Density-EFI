import tkinter as tk
from tkinter import messagebox, filedialog
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

        self.inputs = [
            ("RPM Marcha Lenta", "800"),
            ("MAP Marcha Lenta (kPa)", "40"),
            ("Limite Máximo RPM", "6500"),
            ("RPM Torque Máximo", "4000"),
            ("Deslocamento (Litros)", "1.8"),
            ("Pressão Turbo (BAR)", "0.0"),
            ("Tipo de Admissão (0=TBI,1=ITB)", "0"),
            ("Modo de Carga (0=MAP,1=TPS)", "0")
        ]

        self.entries = {}

        for label_text, default_val in self.inputs:
            frame = tk.Frame(left_frame, bg="#111")
            frame.pack(fill="x", padx=10, pady=4)

            tk.Label(frame, text=label_text,
                     fg="white", bg="#111", anchor="w").pack()

            ent = tk.Entry(frame, bg="#222", fg="white", insertbackground="white")
            ent.insert(0, default_val)
            ent.pack(fill="x")

            self.entries[label_text] = ent

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

    # =========================
    def calcular_matriz(self):
        d = {k: float(v.get()) for k, v in self.entries.items()}

        modo = int(d["Modo de Carga (0=MAP,1=TPS)"])
        tipo_adm = int(d["Tipo de Admissão (0=TBI,1=ITB)"])

        map_min = max(30, d["MAP Marcha Lenta (kPa)"])
        map_max = int((1.0 + d["Pressão Turbo (BAR)"]) * 100)

        eixo_rpm = np.linspace(d["RPM Marcha Lenta"], d["Limite Máximo RPM"], 16).astype(int)

        # eixo carga
        if modo == 0:
            eixo_carga = np.linspace(map_min, map_max, 16).astype(int)
        else:
            eixo_carga = np.linspace(5, 100, 16).astype(int)  # TPS %

        matriz = np.zeros((16, 16))

        ve_max = 85
        fator_admissao = 0.95 if tipo_adm == 0 else 1.05

        for i, rpm in enumerate(eixo_rpm):

            if rpm <= d["RPM Torque Máximo"]:
                fator_rpm = 0.6 + 0.4 * (rpm / d["RPM Torque Máximo"])
            else:
                queda = (rpm - d["RPM Torque Máximo"]) / (d["Limite Máximo RPM"] - d["RPM Torque Máximo"])
                fator_rpm = 1.0 - (0.25 * queda)

            for j, carga_val in enumerate(eixo_carga):

                # =========================
                # CÁLCULO DE CARGA
                # =========================
                if modo == 0:
                    carga = carga_val / map_max
                else:
                    carga = (carga_val / 100.0)

                    # ITB responde mais agressivo no TPS
                    if tipo_adm == 1:
                        carga = carga ** 1.3
                    else:
                        carga = carga ** 1.1

                # =========================
                # CURVA DE CARGA
                # =========================
                if carga < 0.4:
                    fator_carga = 0.45 + (carga * 0.8)
                elif carga < 0.7:
                    fator_carga = 0.65 + (carga * 0.5)
                else:
                    fator_carga = 0.85 + (carga * 0.3)

                ve = ve_max * fator_rpm * fator_carga * fator_admissao
                matriz[j, i] = int(np.clip(ve, 25, 95))

        return eixo_rpm, eixo_carga, matriz

    # =========================
    def gerar_tudo(self):
        rpms, cargas, matriz = self.calcular_matriz()

        for widget in self.frame_result.winfo_children():
            widget.destroy()

        table_frame = tk.Frame(self.frame_result, bg="black")
        table_frame.pack(pady=10)

        linhas = 16
        colunas = 16

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
                    bg = self.get_color(val)
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