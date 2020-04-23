#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <MQTT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Bounce2.h>

#define wifi_ssid "SSID"
#define wifi_password "PASSWORD"

#define mqtt_host "192.168.42.1"

#define update_host "192.168.42.1"
#define update_port 8000
#define update_url "/ds18b20-to-mqtt.bin"

#define update_topic "updates/ds18b20-to-mqtt"

#define switch_topic "switches/room/light"
#define temperature_topic "sensors/fridge/temperature"

WiFiClient wifi;
MQTTClient mqtt;

OneWire wire(14);
DallasTemperature sensors(&wire);

Bounce debouncer;

long then = 0;

void mqtt_callback(String &topic, String &payload) {
  if (topic == update_topic) {
    ESPhttpUpdate.update(update_host, update_port, update_url);
  }
}

void setup_wifi() {
  delay(10);

  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void connect() {
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(mqtt_host);

  while (!mqtt.connect(WiFi.macAddress().c_str())) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("");
  Serial.println("MQTT connected");

  mqtt.subscribe(update_topic, 2);
}

void setup() {
  pinMode(4, INPUT_PULLUP);

  Serial.begin(115200);

  setup_wifi();

  mqtt.begin(mqtt_host, 1883, wifi);
  mqtt.onMessage(mqtt_callback);

  pinMode(14, OUTPUT);
  sensors.begin();

  debouncer.attach(4);
  debouncer.interval(50);
}

void loop() {
  if (!mqtt.connected()) {
    connect();
  }

  mqtt.loop();

  debouncer.update();

  if (debouncer.fell()) {
    mqtt.publish(switch_topic, "true", false, 2);
  }

  if (debouncer.rose()) {
    mqtt.publish(switch_topic, "false", false, 2);
  }

  long now = millis();

  if (now - then < 5000) return;

  then = now;

  sensors.requestTemperatures();

  float t = sensors.getTempCByIndex(0);

  mqtt.publish(temperature_topic, String(t).c_str(), false, 2);
}
