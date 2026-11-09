#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "secrets.h"

const int PIN_CAPACITIVO = 34;
const int PIN_RESISTIVO = 35;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long ultimoEnvio = 0;
const unsigned long INTERVALO_MS = 1000;

void conectarWiFi()
{
  if (WiFi.status() == WL_CONNECTED)
    return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nWi-Fi OK, IP: %s\n", WiFi.localIP().toString().c_str());
}

void conectarMqtt()
{
  while (!mqtt.connected())
  {
    Serial.print("Conectando ao broker MQTT... ");
    String clientId = String("esp32-") + DEVICE_ID;
    if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS))
    {
      Serial.println("OK");
    }
    else
    {
      Serial.printf("falhou, rc=%d. Nova tentativa em 5s\n", mqtt.state());
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  analogReadResolution(12);
  conectarWiFi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setKeepAlive(30);
  mqtt.setBufferSize(512);
}

void loop()
{
  conectarWiFi();
  if (!mqtt.connected())
    conectarMqtt();
  mqtt.loop();

  unsigned long agora = millis();
  if (agora - ultimoEnvio >= INTERVALO_MS)
  {
    ultimoEnvio = agora;

    JsonDocument doc;
    doc["dispositivoId"] = DEVICE_ID;
    doc["capacitivoAdc"] = analogRead(PIN_CAPACITIVO);
    doc["resistivoAdc"] = analogRead(PIN_RESISTIVO);

    // doc["temperaturaC"] = temperatura;
    // doc["umidadeArPercentual"] = umidade;
    // doc["bombaLigada"] = bombaLigada;

    char buffer[256];
    size_t n = serializeJson(doc, buffer);

    if (mqtt.publish(MQTT_TOPIC, (const uint8_t *)buffer, n))
    {
      Serial.printf("Publicado: %s\n", buffer);
    }
    else
    {
      Serial.println("Falha ao publicar");
    }
  }
}