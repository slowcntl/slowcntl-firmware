#! /bin/sh

for name in bme280-to-mqtt ds18b20-to-mqtt sht71-to-mqtt
do
  export PLATFORMIO_SRC_DIR=$name
  export PLATFORMIO_UPLOAD_PORT=upload/$name.bin
  mkdir -p upload
  pio run -e ota -t upload
done
