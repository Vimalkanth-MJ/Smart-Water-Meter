//----------------------------------------
//        Defining Header Files
//----------------------------------------
#include <SPI.h>
#include <LoRa.h>
//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//----------------------------------------
//      Defining Display Variables
//----------------------------------------
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//----------------------------------------
//      Defining LoRa Variables
//----------------------------------------
#define ss 5
#define rst 14
#define dio0 2
String Incoming = "";
String Message = "";
String DeviceID = "Slave01";
byte LocalAddress = 0x02;       //--> address of this device (Slave 1).
bool executed = false;
bool msgRecieved = false;
byte Destination_Master = 0x01; //--> destination to send to Master (ESP32).
//----------------------------------------
//      Defining Variables
//----------------------------------------
int sensorPin = 4;
int relayPin = 26;
volatile long pulse, pulse1;
float cost, creditsOld, totalCredits, flowCredits;
float flowRate, totalLitres, totalLitresOld, flowLitres;
unsigned long oldTime;
unsigned long previousTime = 0;
const float FLOW_CALIBRATION = 7.5;

void sendMessage(String Outgoing, byte Destination) {
  LoRa.beginPacket();             //--> start packet
  LoRa.write(Destination);        //--> add destination address
  LoRa.write(LocalAddress);       //--> add sender address
  LoRa.write(Outgoing.length());  //--> add payload length
  LoRa.print(Outgoing);           //--> add payload
  LoRa.endPacket();               //--> finish packet and send it
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;  //--> if there's no packet, return

  //read packet header bytes:
  int recipient = LoRa.read();        //--> recipient address
  byte sender = LoRa.read();          //--> sender address
  byte incomingLength = LoRa.read();  //--> incoming msg length

  Incoming = "";

  //Get all incoming data.
  while (LoRa.available()) {
    Incoming += (char)LoRa.read();
  }

  //Check length for error.
  if (incomingLength != Incoming.length()) {
    Serial.println();
    Serial.println("error: message length does not match length");
    return; //--> skip rest of function
  }

  //Checks whether the incoming data or message for this device.
  if (recipient != LocalAddress) {
    Serial.println();
    Serial.println("This message is not for me.");
    return; //--> skip rest of function
  }

  Serial.println();
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Message: " + Incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Processing_incoming_data();
  msgRecieved = true;
}

void Processing_incoming_data()
{
  int pos1 = Incoming.indexOf('/');
  int pos2 = Incoming.indexOf('&');
  int pos3 = Incoming.indexOf('#');
  String readingID = Incoming.substring(0, pos1);
  if (readingID == DeviceID)
  {
    totalLitresOld = atof(Incoming.substring(pos1 + 1, pos2).c_str());
    creditsOld = atof(Incoming.substring(pos2 + 1, pos3).c_str());
    cost = atof(Incoming.substring(pos3 + 1, Incoming.length()).c_str());
    Serial.println("Incoming LoRa Data=====================================================================");
    Serial.printf("Cost per Litre: %.2f \n", cost);
    Serial.printf("Available Credits: %.2f \n", creditsOld);
    Serial.printf("Volume Consumed: %.2f \n", totalLitresOld);
    Serial.println();
    Serial.println("============================================================================");
    totalCredits = creditsOld;
  }
}
//----------------------------------------
//           void getReadings()
//----------------------------------------
void getReadings() {
  if (totalCredits > 0)
  {
    if ((millis() - oldTime) > 1000) { // Only calculate flow rate once per second
      detachInterrupt(digitalPinToInterrupt(sensorPin));
      flowRate = ((1000.0 / (millis() - oldTime)) * pulse) / FLOW_CALIBRATION;
      oldTime = millis(); // Update oldTime
      flowLitres =  2.126 * pulse1 / 1000;
      totalLitres = totalLitresOld + flowLitres;
      flowCredits = flowLitres * cost;
      totalCredits = creditsOld - flowCredits;
      digitalWrite(relayPin, LOW);
      Serial.printf("Flow rate: %.2f L/min\n", flowRate);
      Serial.printf("Total volume: %.3f L\n", totalLitres);
      Serial.printf("Avl. Credits: %.2f L\n", totalCredits);
      Serial.println();
      Serial.println();
      Serial.println();
      DisplayData();
      if (flowRate > 0)
      {
        sendLoRaData();
        Serial.println("Sending Data over LoRa");
      }
      pulse = 0; // Reset pulse count
      attachInterrupt(digitalPinToInterrupt(sensorPin), increase, RISING);
    }
  }
  else {
    totalCredits = 0;
    totalLitres = 0;
    flowLitres = 0;
    flowCredits = 0;
    pulse = 0; pulse1 = 0;
    oldTime = millis();
    if (!executed) {
      sendLoRaData();
      executed = true;
    }
    digitalWrite(relayPin, HIGH);
    DisplayNoData();
    getDataFromLoRa();
    onReceive(LoRa.parsePacket());
    delay(500);
    Serial.println("Waiting for packets");
  }
}

//----------------------------------------
//           void sendLoRaData()
//----------------------------------------
void sendLoRaData() {
  Message = String(DeviceID) + "/" + String(totalLitres) + "&" + String(totalCredits) + "#" + String(cost);
  sendMessage(Message, Destination_Master);
}

void increase() {
  pulse++;
  pulse1++;
}

void getDataFromLoRa()
{
  sendMessage("Hi", Destination_Master);
}

void DisplayData()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.print("Total Volume:");
  display.setCursor(85, 10);
  display.print(flowLitres);
  display.setCursor(120, 10);
  display.print("L");
  display.setCursor(0, 30);
  display.print("Flow Rate:");
  display.setCursor(65, 30);
  display.print(flowRate);
  display.setCursor(100, 30);
  display.print("L/m");
  display.setCursor(0, 50);
  display.print("Avl Credits:");
  display.setCursor(85, 50);
  display.print(totalCredits);
  display.display();
}
void DisplayNoData()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 20);
  display.print("Avl Credits:");
  display.setCursor(85, 20);
  display.print(totalCredits);
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print("Please Recharge.!");
  display.display();
}

void setup() {
  Serial.begin(115200);
  LoRa.setPins(ss, rst, dio0);
  Serial.println("Start LoRa init...");
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);
  }
  Serial.println("LoRa init succeeded.");
  pinMode(sensorPin, INPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  attachInterrupt(digitalPinToInterrupt(sensorPin), increase, RISING);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Display init Failed"));
    for (;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  oldTime = millis(); // Initialize oldTime variable
  getDataFromLoRa();
}

void loop() {
  onReceive(LoRa.parsePacket());
  if (msgRecieved == true)
  {
    Serial.println("Sleeping");
    Serial.printf("Flow rate: %.2f L/min\n", flowRate);
    Serial.printf("cost: %.2f L/min\n", cost);
    Serial.printf("creditsOld: %.2f L/min\n", creditsOld);
    Serial.printf("totalCredits: %.2f L/min\n", totalCredits);
    Serial.printf("flowCredits: %.2f L/min\n", flowCredits);
    Serial.printf("totalLitres: %.2f L/min\n", totalLitres);
    Serial.printf("totalLitresOld: %.2f L/min\n", totalLitresOld);
    Serial.printf("flowLitres: %.2f L/min\n", flowLitres);
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
    msgRecieved = false;
    delay(5000);
  }
  getReadings();
}
