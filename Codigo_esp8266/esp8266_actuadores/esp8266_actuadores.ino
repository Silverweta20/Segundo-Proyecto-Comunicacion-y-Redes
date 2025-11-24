#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// ===== WiFi settings =====
const char *ssid = "Redmi";            // Tu red WiFi
const char *password = "123456789";    // Contraseña WiFi

// ===== MQTT Broker settings =====
const char *mqtt_broker   = "broker.emqx.io";
const char *mqtt_topic    = "claseRedes/grupo4/cocina/MQ2";   // Topic para el MQ2
const char *mqtt_username = "emqx";
const char *mqtt_password = "public";
const int   mqtt_port     = 1883;

// ===== Sensor MQ-2 =====
const int  MQ2_ANALOG_PIN   = A0;      // MQ2 conectado al pin A0
const long reading_interval = 1000;    // ms entre lecturas
unsigned long last_reading_time = 0;

// ===== Telegram =====
#define TELEGRAM_BOT_TOKEN "8412864458:AAGsf7CwtYQFvVKc55bHmq61-I0T5Hmz4jU"
#define TELEGRAM_CHAT_ID   "7616687635"

// Umbral para enviar alerta
const int GAS_THRESHOLD = 400;                // bájalo para probar fácil
unsigned long lastAlertTime = 0;
const unsigned long ALERT_COOLDOWN = 1000;   // mínimo 60s entre notificaciones

WiFiClient espClient;          // para MQTT
PubSubClient mqtt_client(espClient);

WiFiClientSecure telegramClient;          // para Telegram (HTTPS)
UniversalTelegramBot bot(TELEGRAM_BOT_TOKEN, telegramClient);

// Prototipos
void connectToWiFi();
void connectToMQTTBroker();
void publishMQ2Reading();

void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.println("Iniciando...");

  connectToWiFi();

  // MUY IMPORTANTE para que no falle el certificado TLS
  telegramClient.setInsecure();
  telegramClient.setTimeout(15000);   // opcional pero ayuda

  mqtt_client.setServer(mqtt_broker, mqtt_port);
  connectToMQTTBroker();
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to the WiFi network");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void connectToMQTTBroker() {
  while (!mqtt_client.connected()) {
    String client_id = "esp8266-client-" + String(WiFi.macAddress());
    Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());

    if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT broker");

      mqtt_client.subscribe(mqtt_topic);                // opcional
      mqtt_client.publish(mqtt_topic, "ESP8266 MQ2 online"); // opcional
    } else {
      Serial.print("Failed to connect to MQTT broker, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" -> try again in 5 seconds");
      delay(5000);
    }
  }
}

// Publica la lectura del MQ2 y manda alerta a Telegram si pasa el umbral
void publishMQ2Reading() {
  int sensorValue = analogRead(MQ2_ANALOG_PIN);  // 0–1023
  String payload = String(sensorValue);

  Serial.print("MQ2 reading: ");
  Serial.println(payload);

  bool ok = mqtt_client.publish(mqtt_topic, payload.c_str());
  if (ok) {
    Serial.println("MQ2 value published");
  } else {
    Serial.println("Publish FAILED");
  }

  // ---- Alerta por Telegram ----
  if (sensorValue > GAS_THRESHOLD) {
    unsigned long now = millis();

    // Solo una alerta cada ALERT_COOLDOWN ms
    if (now - lastAlertTime > ALERT_COOLDOWN) {
      String msg = "⚠ ALARMA GAS en la cocina!\n"
                   "Valor MQ2: " + String(sensorValue);

      Serial.println("Intentando enviar alerta Telegram...");
      bool sent = bot.sendMessage(TELEGRAM_CHAT_ID, msg, "");  // "" = texto plano

      if (sent) {
        Serial.println("Telegram alert sent ✔");
      } else {
        Serial.println("Error sending Telegram alert ✖");
      }
      lastAlertTime = now;
    }
  }
}

void loop() {
  // Mantener conexión MQTT
  if (!mqtt_client.connected()) {
    connectToMQTTBroker();
  }
  mqtt_client.loop();

  // Enviar lectura del MQ2 cada reading_interval milisegundos
  unsigned long now = millis();
  if (now - last_reading_time >= reading_interval) {
    last_reading_time = now;
    publishMQ2Reading();
  }
}
