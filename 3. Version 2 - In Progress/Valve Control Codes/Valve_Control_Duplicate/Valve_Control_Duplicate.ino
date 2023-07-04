const int IN1 = 21;
const int IN2 = 22;
const int MOSFET_INPUT = 16;

void positivePulse()
{
  Serial.println("Generating Positive Pulse.!!");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
}

void negativePulse()
{
  Serial.println("Genrating Negative Pulse.!!");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void pulseZero()
{
  Serial.println("Energy = 0");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, HIGH);
}

void setup() {
  Serial.begin(115200);
  delay(5000);
  Serial.println("Initialising...");
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(MOSFET_INPUT, OUTPUT);
}

void ValveON() {
  Serial.println("Circuit Turned ON...");
  digitalWrite(MOSFET_INPUT, HIGH);
  delay(2000);
  positivePulse();
  delay(2000);
  pulseZero();
  delay(2000);
  Serial.println("Circuit Turned OFF...");
  digitalWrite(MOSFET_INPUT, LOW);
}

void ValveOFF()
{
  Serial.println("Circuit Turned ON...");
  digitalWrite(MOSFET_INPUT, HIGH);
  delay(2000);
  negativePulse();
  delay(2000);
  pulseZero();
  delay(2000);
    Serial.println("Circuit Turned OFF...");
  digitalWrite(MOSFET_INPUT, LOW);
}

void loop()
{
  ValveON();
  delay(2000);
  ValveOFF();
  delay(2000);
}
