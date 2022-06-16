#! /bin/sh

for name in bme280-to-mqtt ds18b20-to-mqtt sht71-to-mqtt
do
  mosquitto_pub -t updates/$name -m go
done
