int sensorPin = 4;
volatile long pulse;

void setup() {
  pinMode(sensorPin, INPUT);
  Serial.begin(115200);
  attachInterrupt(digitalPinToInterrupt(sensorPin), increase, RISING);
}

void loop() {
  Serial.println(pulse);
  delay(500);
}
void increase() {
  pulse++;
}
