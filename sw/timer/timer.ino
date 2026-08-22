void setup() {
  Serial.begin(9600);
}

void loop() {
  unsigned long start_time = millis();
  delay(20);
  unsigned long end_time = millis();
  unsigned long elapsed_time = end_time - start_time;
  Serial.print("waited ");
  Serial.print(elapsed_time);
  Serial.println(" milliseconds");
}
