#include "WiFi.h"
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h> 
#include <ESP32Servo.h> 

#define WIFI_SSID "Catatau"
#define WIFI_PASSWORD "ze colmeia"
#define WIFI_CHANNEL 6

#define dt 4
#define modelo DHT22
DHT dht(dt, modelo); 

#define ldr 34
#define botao 22
#define serv1 32 
#define serv2 23 

#define ledR 5
#define ledG 18
#define ledB 19

#define buz 27 

Servo servoNeck1; 
Servo servoNeck2;

float temperatura = 0.0;
float umidade = 0.0;
float intensidadeLuz = 0.0;
float angle = 0.0; 

unsigned long previousMillis = 0;
const long interval = 5000; 
volatile bool systemEnabled = true; 
volatile unsigned long lastInterruptTime = 0;

const float GAMMA = 0.7;
const float RL10 = 50.0; 

const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
String mqtt_client_id; 
const char* mqtt_topic_publish = "esp32/sensor_data"; 
const char* mqtt_topic_subscribe = "esp32/commands"; 

WiFiClient espClient;
PubSubClient client(espClient);

const char* serverAddress = "https://9a6c8e2af1ee.ngrok-free.app/api/sensor-data"; 

void connectToWiFi();
void reconnectMQTT();
void callback(char* topic, byte* payload, unsigned int length);
void readMPU6050();
void ler_sensores();
void sendAllSensorDataHTTP(); 
void publishSensorDataMQTT();
void handleButtonInterrupt();

void processMqttCommand(String command);
void setLedColor(int r, int g, int b);
void beepBuzzer(String duration);
void setServoAngle(int angle);
String splitString(String data, char separator, int index);

void connectToWiFi() {
  Serial.print("Conectando à rede: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    Serial.print(".");
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n\nConectado com sucesso!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n\nFalha ao conectar. Verifique o SSID e a senha.");
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    if (client.connect(mqtt_client_id.c_str())) {
      Serial.println("conectado!");
      client.subscribe(mqtt_topic_subscribe);
      Serial.print("Subscrito ao tópico: ");
      Serial.println(mqtt_topic_subscribe);
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  processMqttCommand(message);
}

void sendAllSensorDataHTTP() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado. Não é possível enviar HTTP.");
    return;
  }
  StaticJsonDocument<200> doc;
  doc["temp"] = temperatura;
  doc["humidity"] = umidade;
  doc["light"] = intensidadeLuz;
  doc["angle"] = angle;
  String jsonPayload;
  serializeJson(doc, jsonPayload);
  Serial.print("Enviando para API (TODOS OS SENSORES): ");
  Serial.println(jsonPayload);
  HTTPClient http;
  http.begin(serverAddress); 
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(jsonPayload);
  if (httpResponseCode > 0) {
    Serial.print("Código de Resposta HTTP: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("Erro no envio HTTP: ");
    Serial.println(httpResponseCode);
    Serial.println(http.errorToString(httpResponseCode));
  }
  http.end();
}

void publishSensorDataMQTT() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  StaticJsonDocument<200> doc;
  doc["temp"] = temperatura;
  doc["humidity"] = umidade;
  doc["light"] = intensidadeLuz;
  doc["angle"] = angle;
  String jsonPayload;
  serializeJson(doc, jsonPayload);
  Serial.print("Publicando MQTT no tópico ");
  Serial.print(mqtt_topic_publish);
  Serial.print(": ");
  Serial.println(jsonPayload);
  client.publish(mqtt_topic_publish, jsonPayload.c_str());
}

void readMPU6050() {
  angle = (float)random(-900, 900) / 10.0; 
  Serial.print("Ângulo (Simulado): "); Serial.print(angle); Serial.println(" graus");
}

void ler_sensores(){
  float temp_lida = dht.readTemperature();
  float umid_lida = dht.readHumidity();
  
  if (!isnan(temp_lida) && !isnan(umid_lida)) {
    temperatura = temp_lida;
    umidade = umid_lida;
    Serial.print("Temperatura: "); Serial.print(temperatura); Serial.println(" C");
    Serial.print("Umidade: "); Serial.print(umidade); Serial.println(" %");
  } else {
    Serial.println("Erro ao ler o sensor DHT22!");
  }

  int adcLdr = analogRead(ldr);
  float voltage = adcLdr * (3.3 / 4095.0);
  float resistance = 10 * (3.3 - voltage) / voltage; 
  
  intensidadeLuz = pow(RL10 / resistance, 1 / GAMMA) * 10;
  if (isnan(intensidadeLuz)) intensidadeLuz = 0;
  
  Serial.print("Intensidade Luminosa (Lux): "); Serial.println(intensidadeLuz);
  
  readMPU6050(); 
}

void processMqttCommand(String command) {
  if (command == "TOGGLE") {
    systemEnabled = !systemEnabled;
    Serial.print("Estado do sistema alterado para: ");
    Serial.println(systemEnabled ? "ATIVADO" : "DESATIVADO");
    return;
  }

  String tipo = splitString(command, ':', 0);
  String part1 = splitString(command, ':', 1);
  String part2 = splitString(command, ':', 2);
  String part3 = splitString(command, ':', 3);

  if (tipo == "LED") {
    String target = part1;
    String action = part2;
    String value = part3;

    if (target == "EYES" || target == "BOCA") {
      if (action == "ON") {
        int r = splitString(value, ',', 0).toInt();
        int g = splitString(value, ',', 1).toInt();
        int b = splitString(value, ',', 2).toInt();
        Serial.print("Acionando LED ("); Serial.print(target); Serial.print(") - R:"); Serial.print(r); Serial.print(" G:"); Serial.print(g); Serial.print(" B:"); Serial.println(b);
        setLedColor(r, g, b);
      } else if (action == "OFF") {
        Serial.print("Desligando LED ("); Serial.print(target); Serial.println(")");
        setLedColor(0, 0, 0);
      }
    }
  } else if (tipo == "SERVO") {
    String target = part1;
    int value = part2.toInt();

    if (target == "NECK") {
      Serial.print("Acionando Servos (Pescoço) para: "); Serial.println(value);
      setServoAngle(value);
    }
  } else if (tipo == "BUZZER") {
    String action = part1;
    String duration = part2;

    if (action == "BEEP") {
      Serial.print("Acionando Buzzer: "); Serial.println(duration);
      beepBuzzer(duration);
    }
  } else {
    Serial.print("Comando MQTT não reconhecido: ");
    Serial.println(command);
  }
}

void setLedColor(int r, int g, int b) {
  analogWrite(ledR, r);
  analogWrite(ledG, g);
  analogWrite(ledB, b);
}

void beepBuzzer(String duration) {
  if (duration == "LONG") {
    tone(buz, 2500, 1000); 
  } else if (duration == "SHORT") {
    tone(buz, 2500, 1000); 
  }
}

void setServoAngle(int angle) {
  angle = constrain(angle, 0, 180);
  servoNeck1.write(angle);
  servoNeck2.write(angle); 
}

String splitString(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void IRAM_ATTR handleButtonInterrupt() {
  if (millis() - lastInterruptTime > 500) {
    systemEnabled = !systemEnabled;
    lastInterruptTime = millis();
    Serial.print("Sistema ");
    Serial.println(systemEnabled ? "ATIVADO" : "DESATIVADO");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando o ESP32...");
  randomSeed(analogRead(0)); 
  
  dht.begin();
  pinMode(ldr, INPUT);
  pinMode(botao, INPUT_PULLUP); 
  
  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);
  pinMode(buz, OUTPUT);
  
  servoNeck1.attach(serv1); 
  servoNeck2.attach(serv2);
  
  setLedColor(0, 0, 0);
  servoNeck1.write(90); 
  servoNeck2.write(90); 
  
  attachInterrupt(digitalPinToInterrupt(botao), handleButtonInterrupt, FALLING); 

  connectToWiFi();

  mqtt_client_id = "ESP32Client-";
  mqtt_client_id += WiFi.macAddress();
  mqtt_client_id.replace(":", ""); 
  Serial.print("Usando MQTT Client ID Único: ");
  Serial.println(mqtt_client_id);
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  Serial.println("Setup concluído!");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nConexão Wi-Fi perdida. Tentando reconectar...");
    connectToWiFi(); 
  }
  
  if (WiFi.status() == WL_CONNECTED && !client.connected()) {
    reconnectMQTT();
  }
  
  if (client.connected()) {
    client.loop(); 
  }

  unsigned long currentMillis = millis();

  if (systemEnabled && (currentMillis - previousMillis >= interval)) {
    previousMillis = currentMillis;

    ler_sensores();

    sendAllSensorDataHTTP(); 

    publishSensorDataMQTT();
  }
}