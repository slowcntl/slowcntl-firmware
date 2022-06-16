#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <MQTT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <SparkFunBME280.h>
#include <SparkFunCCS811.h>

#define wifi_ssid "SSID"
#define wifi_password "PASSWORD"

#define update_host "192.168.42.1"
#define update_port 8000
#define update_url "/bme280-to-mqtt.bin"

#define update_topic "updates/bme280-to-mqtt"

#define mqtt_host "192.168.42.1"
#define mqtt_port 1883

#define mqtt_id "ID"

#define mqtt_size 6

const char *mqtt_topics[mqtt_size] = {
  "/sensors/DS/temp",
  "/sensors/BME/temp",
  "/sensors/BME/humi",
  "/sensors/BME/dew",
  "/sensors/CCS/co2",
  "/sensors/CCS/tvoc"
};

float mqtt_values[mqtt_size] = {0};

char mqtt_buffer[128] = {0};

WiFiClient wifi;
MQTTClient mqtt;

OneWire wire(4);
DallasTemperature sensors(&wire);

BME280 bme;

CCS811 ccs(0x5A);

long then = 0;

void mqtt_callback(String &topic, String &payload) {
  if (topic == update_topic) {
    ESPhttpUpdate.update(wifi, update_host, update_port, update_url);
  }
}

void connect() {
  mqtt.connect(WiFi.macAddress().c_str());

  mqtt.subscribe(update_topic, 2);
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  mqtt.begin(mqtt_host, mqtt_port, wifi);

  sensors.begin();

  Wire.begin(13, 14);

  bme.setI2CAddress(0x76);
  if (!bme.beginI2C()) {
    Serial.println("BME280 error. Please check wiring.");
    while (1) delay(10);
  }

  if (!ccs.begin()) {
    Serial.print("CCS811 error. Please check wiring.");
    while (1) delay(10);
  }
}

void loop() {
  int i;
  long now;

  if (WiFi.status() == WL_CONNECTED) {
    if (!mqtt.connected()) {
      connect();
    }

    mqtt.loop();
  }

  now = millis();

  if (now - then < 5000) return;

  then = now;

  sensors.requestTemperatures();

  if (ccs.dataAvailable()) {
    ccs.readAlgorithmResults();
  }

  mqtt_values[0] = sensors.getTempCByIndex(0);
  mqtt_values[1] = bme.readTempC();
  mqtt_values[2] = bme.readFloatHumidity();
  mqtt_values[3] = bme.dewPointC();
  mqtt_values[4] = ccs.getCO2();
  mqtt_values[5] = ccs.getTVOC();

  ccs.setEnvironmentalData(mqtt_values[2], mqtt_values[1]);

  for (i = 0; i < mqtt_size; ++i) {
    Serial.println(mqtt_values[i]);
  }

  for (i = 0; i < mqtt_size; ++i) {
    strcpy(mqtt_buffer, mqtt_id);
    strcat(mqtt_buffer, mqtt_topics[i]);

    mqtt.publish(mqtt_buffer, String(mqtt_values[i]).c_str(), false, 2);
  }
}
