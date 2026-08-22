void setup() {
  Serial.begin(9600);
}

constexpr size_t NUM_ANALOG_PINS = 6;

void loop() {
  constexpr size_t pins[NUM_ANALOG_PINS] {A0, A1, A2, A3, A4, A5};
  for (size_t i = 0; i < NUM_ANALOG_PINS; ++i) {
    int sensorValue = analogRead(pins[i]);
    float voltage = sensorValue * (5.0 / 1023.0);
    Serial.print("Voltage PIN A");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(voltage);
    Serial.println(" V");
  }
  Serial.println("");
}
