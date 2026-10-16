#include <BMP180.h>

// Reads temperature and pressure of a BMP180 in a loop using the BMP180
// library (https://github.com/styropyr0/BMP180), which depends on SensorHub.

#define OSS 3

BMP180 bmp180;

void setup() {
  Serial.begin(9600);
  Serial.println("BMP180 demo");

  if (!bmp180.begin()) {
    Serial.println("BMP180 not found");
    while (true) {
    }
  }

  bmp180.setPressureOverSampling(OSS);
}

void loop() {
  float temperature = bmp180.readTemperature();
  float pressure = bmp180.readPressure();

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Pressure: ");
  Serial.print(pressure / 100.0);
  Serial.println(" hPa");

  delay(100);
}
