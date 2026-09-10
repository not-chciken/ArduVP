const int pin_button = 2;
int state_button = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pin_button, INPUT);
}

void loop() {
  state_button = digitalRead(pin_button);

  if (state_button == HIGH) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}
