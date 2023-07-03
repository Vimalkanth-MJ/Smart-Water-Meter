int EN1 = 25;
int IN1 = 26;
int IN2 = 27;

void ValveON()
{
  Serial.println("Valve ON!");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  delay(2000);
  digitalWrite(IN1, LOW);
}

void ValveOFF()
{
  Serial.println("Valve OFF!");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  delay(2000);
  digitalWrite(IN2, LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(EN1, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  ValveOFF();
}

void loop() {
  ValveON();
  delay(10000);
  ValveOFF();
  delay(10000);
}
