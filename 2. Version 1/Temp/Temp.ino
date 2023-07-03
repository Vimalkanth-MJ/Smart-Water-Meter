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
//     Enable/Disable the Debugging
//----------------------------------------

#define DEBUG // Enable debugging

#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

//----------------------------------------
//      Defining Display Variables
//----------------------------------------

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//----------------------------------------
//      Defining PIN Config Variables
//----------------------------------------

#define ss 5
#define rst 14
#define dio0 2
int sensorPin = 4;
int relayPin = 17;

//----------------------------------------
//      Defining LoRa Variables
//----------------------------------------

String Incoming = "";
String Message = "";
String DeviceID = "Slave01";
byte LocalAddress = 0x02;       //--> address of this device (Slave 1).
bool executed = false;
bool msgRecieved = false;
byte Destination_Master = 0x01; //--> destination to send to Master (ESP32).

//----------------------------------------
//  Defining Variables for Flow Sensor
//----------------------------------------

volatile long pulse, pulse1;
float cost, creditsOld, totalCredits, flowCredits;
float flowRate, totalLitres, totalLitresOld, flowLitres;
unsigned long oldTime;
unsigned long previousTime = 0;
const float FLOW_CALIBRATION = 7.5;

//----------------------------------------
//  Function for Sending LoRa Message
//----------------------------------------

void sendMessage(String Outgoing, byte Destination) {
  LoRa.beginPacket();             //--> start packet
  LoRa.write(Destination);        //--> add destination address
  LoRa.write(LocalAddress);       //--> add sender address
  LoRa.write(Outgoing.length());  //--> add payload length
  LoRa.print(Outgoing);           //--> add payload
  LoRa.endPacket();               //--> finish packet and send it
}

//----------------------------------------
//  Function to reicieve LoRa Message
//----------------------------------------

void onReceive(int packetSize) {
  checkRelayPIN();
  if (packetSize == 0) return;        //--> if there's no packet, return

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
    DEBUG_PRINTLN();
    DEBUG_PRINTLN("error: message length does not match length");
    return; //--> skip rest of function
  }

  //Checks whether the incoming data or message for this device.
  if (recipient != LocalAddress) {
    DEBUG_PRINTLN();
    DEBUG_PRINTLN("This message is not for me.");
    return; //--> skip rest of function
  }

  DEBUG_PRINTLN();
  DEBUG_PRINTLN("Received from: 0x" + String(sender, HEX));
  DEBUG_PRINTLN("Message: " + Incoming);
  DEBUG_PRINTLN("RSSI: " + String(LoRa.packetRssi()));
  Processing_incoming_data();
  msgRecieved = true;
}
//----------------------------------------
// Function to process the Incoming Data
//----------------------------------------
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
    DEBUG_PRINTLN("===============Incoming LoRa Data===============");
    DEBUG_PRINT("Cost per Litre: ");
    DEBUG_PRINTLN(cost);
    DEBUG_PRINT("Available Credits: ");
    DEBUG_PRINTLN(creditsOld);
    DEBUG_PRINT("Volume Consumed: ");
    DEBUG_PRINTLN(totalLitresOld);
    DEBUG_PRINTLN();
    DEBUG_PRINTLN("=================================================");
    totalCredits = creditsOld;
  }
}
//----------------------------------------
//
//----------------------------------------
void getReadings() {
  checkRelayPIN();
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
      digitalWrite(relayPin, HIGH);
      delay(10);
      DisplayData();
      if (flowRate > 0)
      {
        sendLoRaData();
        DEBUG_PRINTLN("Sending Data over LoRa");
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
    digitalWrite(relayPin, LOW);
    DisplayNoData();
    getDataFromLoRa();
    onReceive(LoRa.parsePacket());
    DEBUG_PRINTLN("Waiting for packets");
    delay(1000);
  }
  checkRelayPIN();

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
  DEBUG_PRINTLN("Start LoRa init...");
  if (!LoRa.begin(433E6)) {
    DEBUG_PRINTLN("LoRa init failed. Check your connections.");
    while (true);
  }
  DEBUG_PRINTLN("LoRa init succeeded.");
  pinMode(sensorPin, INPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  attachInterrupt(digitalPinToInterrupt(sensorPin), increase, RISING);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    DEBUG_PRINTLN(F("Display init Failed"));
    for (;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  oldTime = millis();
  getDataFromLoRa();
}
void checkRelayPIN()
{
  int relayStatus = digitalRead(relayPin);

  // Print the status to the serial monitor
  Serial.print("Relay Pin Status: ");
  Serial.println(relayStatus);
}
void loop() {
  onReceive(LoRa.parsePacket());
  getReadings();
  checkRelayPIN();
}
