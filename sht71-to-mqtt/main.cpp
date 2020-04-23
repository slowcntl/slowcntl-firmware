#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <MQTT.h>
#include <SHT1x.h>
#include <math.h>

#define wifi_ssid "SSID"
#define wifi_password "PASSWORD"

#define mqtt_host "192.168.42.1"

#define update_host "192.168.42.1"
#define update_port 8000
#define update_url "/sht71-to-mqtt.bin"

#define update_topic "updates/sht71-to-mqtt"

#define light_topic "actuators/room/light"

#define temperature_topic "sensors/room/temperature"
#define humidity_topic "sensors/room/humidity"
#define dewpoint_topic "sensors/room/dewpoint"

WiFiClient wifi;
MQTTClient mqtt;

SHT1x sht1x(14, 13, 3.5);

long then = 0;

void mqtt_callback(String &topic, String &payload) {
  if (topic == update_topic) {
    ESPhttpUpdate.update(update_host, update_port, update_url);
  } else if (topic == light_topic) {
    if (payload[0] == 't') {
      digitalWrite(4, HIGH);
    } else {
      digitalWrite(4, LOW);
    }
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
  mqtt.subscribe(light_topic, 2);
}

void setup() {
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);

  Serial.begin(115200);

  setup_wifi();

  mqtt.begin(mqtt_host, 1883, wifi);
  mqtt.onMessage(mqtt_callback);
}

double dewpoint(float t, float h)
{
  float tn = t < 0.0 ? 272.62 : 243.12;
  float m = t < 0.0 ? 22.46 : 17.62;

  float l = logf(h / 100.0);
  float r = m * t / (tn + t);

  return tn * (l + r) / (m - l - r);
}

void loop() {
  if (!mqtt.connected()) {
    connect();
  }

  mqtt.loop();

  long now = millis();

  if (now - then < 5000) return;

  then = now;

  float t = sht1x.readTemperatureC();
  float h = sht1x.readHumidity();
  float dp = dewpoint(t, h);

  mqtt.publish(temperature_topic, String(t).c_str(), false, 2);
  mqtt.publish(humidity_topic, String(h).c_str(), false, 2);
  mqtt.publish(dewpoint_topic, String(dp).c_str(), false, 2);
}
