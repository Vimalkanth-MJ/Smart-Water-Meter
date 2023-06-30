int sensorPin = 4;
volatile long pulse;
float volume;

void setup() {
  pinMode(sensorPin, INPUT);
  Serial.begin(115200);
  attachInterrupt(digitalPinToInterrupt(sensorPin), increase, RISING);
}

void loop() {
  volume = 2.120*pulse; 
  Serial.print(volume);
  Serial.println(" mL");
  delay(500);
}
void increase() {
  pulse++;
}
