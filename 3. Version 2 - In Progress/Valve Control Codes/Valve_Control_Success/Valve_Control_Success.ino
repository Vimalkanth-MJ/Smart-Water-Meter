
const int IN1 = 21;
const int IN2 = 22;

void positivePulse()
{
  Serial.println("Energy ++");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
} 

void negativePulse()
{
  Serial.println("Energy --");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void pulseZero()
{
  Serial.println("Energy = 0");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, HIGH);
}

void pulseZeroDuplicate()
{
  Serial.println("Energy = 0");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}


void setup() {
  Serial.begin(115200);
  Serial.println("Initialising...");
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
}

void loop() {
positivePulse();
delay(2000);
pulseZero();
delay(2000);
negativePulse();
delay(2000);
pulseZero();
delay(2000);
}
