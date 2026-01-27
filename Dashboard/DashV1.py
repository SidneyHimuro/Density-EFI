import dash
from dash import dcc, html
from dash.dependencies import Input, Output, State
import plotly.graph_objects as go
import serial
import serial.tools.list_ports
import threading
import time

# ================= CONFIG GLOBAL =================
serial_data = {"RPM": 0, "MAP": 0.0, "TPS": 0.0, "TINJ": 0.0}
last_data_time = 0 
connection_active = False
ser_instance = None
data_lock = threading.Lock()

def serial_reader():
    global serial_data, ser_instance, last_data_time, connection_active
    while True:
        if connection_active and ser_instance and ser_instance.is_open:
            try:
                if ser_instance.in_waiting > 0:
                    line = ser_instance.readline().decode(errors="ignore").strip()
                    if line:
                        parts = line.split(",")
                        if len(parts) == 4:
                            try:
                                with data_lock:
                                    serial_data["RPM"]  = float(parts[0])
                                    serial_data["MAP"]  = float(parts[1])
                                    serial_data["TPS"]  = float(parts[2])
                                    serial_data["TINJ"] = float(parts[3])
                                last_data_time = time.time()
                            except: pass
            except:
                connection_active = False
        time.sleep(0.01)

threading.Thread(target=serial_reader, daemon=True).start()

# ================= COMPONENTES VISUAIS =================

def create_rpm_bar(value, shift_limit, n_intervals):
    # Lógica de cor e pisca do Shift Light
    color = 'rgb(0, 255, 0)' if value < 5500 else 'rgb(255, 255, 0)' if value < 7000 else 'rgb(255, 0, 0)'
    if value >= (shift_limit or 7200):
        color = 'rgb(255, 0, 0)' if n_intervals % 2 == 0 else 'rgba(0,0,0,0)'
    
    fig = go.Figure(go.Bar(x=[value], y=[""], orientation='h', marker=dict(color=color), width=0.6))
    fig.update_layout(
        xaxis=dict(range=[0, 8000], showgrid=False, zeroline=False, 
                   tickvals=[1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000],
                   ticktext=["1", "2", "3", "4", "5", "6", "7", "8"],
                   tickfont=dict(color="white", size=24, family="Arial Black")),
        yaxis=dict(visible=False), plot_bgcolor="rgba(40, 40, 40, 0.5)",
        paper_bgcolor="rgba(0,0,0,0)", margin=dict(l=20, r=20, t=10, b=30), height=100
    )
    return fig

def render_card(label, value, unit, border_color):
    return html.Div(style={
        "backgroundColor": "rgba(20, 20, 20, 0.9)", "border": f"3px solid {border_color}",
        "borderRadius": "15px", "padding": "20px", "margin": "10px", "flex": "1",
        "textAlign": "center", "minWidth": "280px"
    }, children=[
        html.Div(label.upper(), style={"color": "#AAA", "fontSize": "22px", "fontWeight": "900"}),
        html.Div([
            html.Span(value, style={"color": "white", "fontSize": "85px", "fontWeight": "bold"}),
            html.Span(f" {unit}", style={"color": border_color, "fontSize": "26px", "fontWeight": "bold", "marginLeft": "10px"})
        ])
    ])

# ================= LAYOUT =================

app = dash.Dash(__name__)

app.layout = html.Div(style={"backgroundColor": "#000", "minHeight": "100vh", "padding": "20px 40px"}, children=[
    
    html.H1("Dash - Density EFI v1.0", style={"textAlign": "center", "color": "white", "fontWeight": "bold", "marginBottom": "30px"}),

    # Toolbar
    html.Div(style={
        "display": "flex", "justifyContent": "center", "alignItems": "center", 
        "gap": "30px", "marginBottom": "40px", "padding": "15px", "backgroundColor": "#111", "borderRadius": "12px"
    }, children=[
        dcc.Dropdown(
            id='port-dropdown',
            options=[{'label': p.device, 'value': p.device} for p in serial.tools.list_ports.comports()],
            placeholder="Porta COM",
            style={"width": "180px", "color": "black"}
        ),

        html.Button("CONECTAR", id="btn-connect", n_clicks=0, style={
            "fontSize": "16px", "fontWeight": "bold", "padding": "10px 20px", "borderRadius": "8px", "border": "none"
        }),

        # LED e Status corrigidos
        html.Div(style={"display": "flex", "alignItems": "center", "gap": "12px", "minWidth": "150px"}, children=[
            html.Div(id="led-indicator", style={
                "width": "15px", "height": "15px", "borderRadius": "50%", "transition": "all 0.3s"
            }),
            html.Span(id="status-text", style={"fontWeight": "bold", "fontSize": "14px", "color": "#AAA"})
        ]),

        html.Div([
            html.Label("SHIFT: ", style={"fontWeight": "bold", "color": "#e74c3c"}),
            dcc.Input(id="shift-input", type="number", value=7200, style={
                "backgroundColor": "#222", "color": "white", "border": "1px solid #444", "padding": "5px", "width": "80px"
            })
        ])
    ]),

    dcc.Graph(id="rpm-bar", config={'displayModeBar': False}),

    html.Div(style={"display": "flex", "flexWrap": "wrap", "justifyContent": "center", "gap": "20px", "marginTop": "30px"}, children=[
        html.Div(id="map-container"),
        html.Div(id="tps-container"),
        html.Div(id="tinj-container")
    ]),

    dcc.Interval(id="update-timer", interval=100),
    dcc.Interval(id='port-refresh', interval=2000)
])

# ================= CALLBACKS =================

@app.callback(
    [Output("btn-connect", "children"), Output("btn-connect", "style")],
    [Input("btn-connect", "n_clicks")],
    [State("port-dropdown", "value")]
)
def handle_btn(n, port):
    global connection_active, ser_instance
    if n % 2 == 1:
        if port:
            try:
                ser_instance = serial.Serial(port, 115200, timeout=0.1)
                connection_active = True
                return "DESCONECTAR", {"backgroundColor": "#e74c3c", "color": "white"}
            except:
                return "ERRO PORTA", {"backgroundColor": "#555", "color": "white"}
    connection_active = False
    if ser_instance: ser_instance.close(); ser_instance = None
    return "CONECTAR", {"backgroundColor": "#3498db", "color": "white"}

@app.callback(
    [Output("led-indicator", "style"), Output("status-text", "children"),
     Output("rpm-bar", "figure"), Output("map-container", "children"),
     Output("tps-container", "children"), Output("tinj-container", "children")],
    [Input("update-timer", "n_intervals")],
    [State("shift-input", "value")]
)
def sync_ui(n, shift):
    global last_data_time, connection_active
    
    # Lógica do Indicador Visual (LED)
    elapsed = time.time() - last_data_time
    if not connection_active:
        led_s = {"backgroundColor": "#444", "boxShadow": "none"}
        txt = "OFFLINE"
    elif elapsed < 0.3:
        led_s = {"backgroundColor": "#2ecc71", "boxShadow": "0 0 15px #2ecc71"}
        txt = "ONLINE"
    else:
        led_s = {"backgroundColor": "#f1c40f", "boxShadow": "0 0 15px #f1c40f"}
        txt = "SEM DADOS"

    with data_lock:
        rpm, m, t, i = serial_data["RPM"], serial_data["MAP"], serial_data["TPS"], serial_data["TINJ"]

    return (
        led_s, txt,
        create_rpm_bar(rpm, shift, n),
        render_card("MAP", f"{m:.2f}", "bar", "#3498db"),
        render_card("TPS", f"{int(t)}", "%", "#f39c12"),
        render_card("Tinj", f"{i:.2f}", "ms", "#9b59b6")
    )

@app.callback(Output('port-dropdown', 'options'), Input('port-refresh', 'n_intervals'))
def refresh_ports(n):
    return [{'label': p.device, 'value': p.device} for p in serial.tools.list_ports.comports()]

if __name__ == "__main__":
    app.run(debug=False)