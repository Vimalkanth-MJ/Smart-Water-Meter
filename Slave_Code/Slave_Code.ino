int sensorPin = 4;
volatile long pulse;
float volume;

void setup() {
  pinMode(sensorPin, INPUT);
  Serial.begin(115200);
  attachInterrupt(digitalPinToInterrupt(sensorPin), increase, RISING);
}

void loop() {
  volume = 2.120*pulse/1000; 
  Serial.printf("Total volume:%f L\n",volume);
  delay(500);
}
void increase() {
  pulse++;
}
