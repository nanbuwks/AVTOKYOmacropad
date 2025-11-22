#define PIN_LED PB2

void setup() {
  pinMode(PIN_LED, OUTPUT);
}

void loop() {
  digitalWrite(PIN_LED, HIGH);
  delay(800);
  digitalWrite(PIN_LED, LOW);
  delay(200);
}
