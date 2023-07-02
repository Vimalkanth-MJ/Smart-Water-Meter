int sensorPin = 4;
volatile long pulse,pulse1;
float volume;
unsigned long oldTime;
float flowRate;
const float FLOW_CALIBRATION = 7.5; // Calibration factor for flow sensor

void setup() {
  pinMode(sensorPin, INPUT);
  Serial.begin(115200);
  attachInterrupt(digitalPinToInterrupt(sensorPin), increase, RISING);
  oldTime = millis(); // Initialize oldTime variable
}

void loop() {
  if ((millis() - oldTime) > 1000) { // Only calculate flow rate once per second
    detachInterrupt(digitalPinToInterrupt(sensorPin));
    flowRate = ((1000.0 / (millis() - oldTime)) * pulse)/FLOW_CALIBRATION;
    volume =  2.126 * pulse1 / 1000; 
    Serial.printf("Flow rate: %.2f L/min\n", flowRate);
    Serial.printf("Total volume: %.3f L\n", volume);
    pulse = 0; // Reset pulse count
    oldTime = millis(); // Update oldTime
    attachInterrupt(digitalPinToInterrupt(sensorPin), increase, RISING);
  }
  delay(500);
}

void increase() {
  pulse++;
  pulse1++;
}
