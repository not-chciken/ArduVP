#include <Adafruit_ADS1X15.h>
#include <Wire.h>

Adafruit_ADS1015 ads1015;

void setup(void)
{
  Serial.begin(9600);
  Serial.println("Starting ADS1015 demo!\n");

  ads1015.begin();
}

void loop(void)
{
  int16_t adc0, adc1, adc2, adc3;

  adc0 = ads1015.readADC_SingleEnded(0);
  adc1 = ads1015.readADC_SingleEnded(1);
  adc2 = ads1015.readADC_SingleEnded(2);
  adc3 = ads1015.readADC_SingleEnded(3);

  Serial.print("AIN0: ");
  Serial.print(ads1015.computeVolts(adc0) * 1000);
  Serial.println(" mV");

  Serial.print("AIN1: ");
  Serial.print(ads1015.computeVolts(adc1) * 1000);
  Serial.println(" mV");

  Serial.print("AIN2: ");
  Serial.print(ads1015.computeVolts(adc2) * 1000);
  Serial.println(" mV");

  Serial.print("AIN3: ");
  Serial.print(ads1015.computeVolts(adc3) * 1000);
  Serial.println(" mV");

  Serial.println(" ");

  delay(100);
}
