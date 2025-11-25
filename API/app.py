# 1. Importar as ferramentas
import paho.mqtt.client as mqtt
from flask import Flask, request, jsonify
from flask_cors import CORS
import sqlite3  
import time     

MQTT_BROKER_ENDERECO = "broker.hivemq.com"
MQTT_BROKER_PORTA = 1883
MQTT_TOPICO = "esp32/commands" 

DATABASE_FILE = "telemetria.db" 

app = Flask(__name__)
CORS(app) 

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"Conectado ao Broker MQTT em {MQTT_BROKER_ENDERECO}!")
    else:
        print(f"Falha ao conectar, código: {rc}")

client = mqtt.Client() 
client.on_connect = on_connect
client.connect(MQTT_BROKER_ENDERECO, MQTT_BROKER_PORTA, 60)
client.loop_start() 

def map_angle(angle):
    angle = max(-20, min(20, angle)) 
    mapped_value = int((angle + 20) * (180 / 40))
    return mapped_value

@app.route("/api/send-command", methods=["POST"])
def handle_command():
    data = request.json
    print(f"(Fase 1) Recebido do Frontend: {data}")

    command_mqtt = ""
    
    try:
        tipo = data.get('type')
        valor = data.get('value')

        if tipo == 'MOUTH' and valor == 'OPEN':
            command_mqtt = "LED:BOCA:ON:255,120,0"
        elif tipo == 'MOUTH' and valor == 'CLOSED':
            command_mqtt = "LED:BOCA:OFF"
        elif tipo == 'TILT':
            angulo_mapeado = map_angle(valor) 
            command_mqtt = f"SERVO:NECK:{angulo_mapeado}"
        elif tipo == 'EYE' and valor == 'BLINK':
            command_mqtt = "BUZZER:BEEP:SHORT"
        elif tipo == 'HAND' and valor == 'OK':
            command_mqtt = "LED:EYES:ON:0,255,0"
        elif tipo == 'HAND' and valor == 'FIST':
            command_mqtt = "LED:EYES:ON:255,0,0"
        elif tipo == 'HAND' and valor == 'OPEN':
            command_mqtt = "LED:EYES:OFF"

        if command_mqtt:
            client.publish(MQTT_TOPICO, command_mqtt)
            print(f"(Fase 1) Publicado no MQTT: {command_mqtt}")
            return jsonify({"status": "sucesso", "comando_enviado": command_mqtt})
        else:
            return jsonify({"status": "erro", "mensagem": "Comando não mapeado"}), 400

    except Exception as e:
        print(f"(Fase 1) Erro ao processar: {e}")
        return jsonify({"status": "erro", "mensagem": str(e)}), 500

@app.route("/api/sensor-data", methods=["POST"])
def receive_sensor_data():
    data = request.json
    print(f"(Fase 2) Recebido do ESP32: {data}") 

    try:
        leituras_para_salvar = []
        
        if data.get('temp') is not None:
            leituras_para_salvar.append(('temperatura', data.get('temp')))
            
        if data.get('humidity') is not None:
            leituras_para_salvar.append(('umidade', data.get('humidity')))
            
        if data.get('light') is not None:
            leituras_para_salvar.append(('luminosidade', data.get('light')))
            
        if data.get('angle') is not None:
            leituras_para_salvar.append(('angulo', data.get('angle')))

        if not leituras_para_salvar:
            return jsonify({"status": "erro", "mensagem": "JSON inválido. Nenhum dado de sensor reconhecido (esperado 'temp', 'humidity', 'light' ou 'angle')"}), 400

        conn = sqlite3.connect(DATABASE_FILE)
        cursor = conn.cursor()

        cursor.executemany("INSERT INTO leituras_sensores (tipo_sensor, valor) VALUES (?, ?)", 
                           leituras_para_salvar)
        
        conn.commit()
        conn.close()
        
        print(f"(Fase 2) Salvo no DB: {len(leituras_para_salvar)} novas leituras.")
        return jsonify({"status": "sucesso", "dados_recebidos": data})

    except Exception as e:
        print(f"(Fase 2) Erro ao salvar no DB: {e}")
        return jsonify({"status": "erro", "mensagem": str(e)}), 500
    
@app.route("/api/sensor-data", methods=["GET"])
def send_sensor_data():
    print("(Fase 2) Frontend pediu dados dos sensores...")
    
    try:
        conn = sqlite3.connect(DATABASE_FILE)
        conn.row_factory = sqlite3.Row 
        cursor = conn.cursor()

        cursor.execute("SELECT * FROM leituras_sensores ORDER BY timestamp DESC LIMIT 20")
        
        leituras_raw = cursor.fetchall()
        conn.close()

        leituras_json = [dict(row) for row in leituras_raw]
        
        return jsonify(leituras_json)

    except Exception as e:
        print(f"(Fase 2) Erro ao ler do DB: {e}")
        return jsonify({"status": "erro", "mensagem": str(e)}), 500

def inicializar_banco():
    """
    Verifica e cria a tabela 'leituras_sensores' no banco de dados
    se ela ainda não existir.
    """
    print(f"Iniciando a verificação do banco de dados: {DATABASE_FILE}...")
    conn = None
    try:
        conn = sqlite3.connect(DATABASE_FILE)
        cursor = conn.cursor()

        cursor.execute("""
        CREATE TABLE IF NOT EXISTS leituras_sensores (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            tipo_sensor TEXT NOT NULL,
            valor REAL NOT NULL
        )
        """)

        conn.commit()
        print("Tabela 'leituras_sensores' verificada/criada com sucesso.")

    except Exception as e:
        print(f"Ocorreu um erro ao inicializar o banco: {e}")

    finally:
        if conn:
            conn.close()
    print("Inicialização do banco finalizada.")


if __name__ == "__main__":
    
    inicializar_banco()
    
    print("Iniciando o servidor Backend (API Fases 1 e 2)...")
    app.run(host='0.0.0.0', port=5000, debug=True)