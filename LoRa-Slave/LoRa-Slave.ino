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
byte Destination_Master = 0x01; //--> destination to send to Master (ESP32).
//----------------------------------------
//      Defining Variables
//----------------------------------------
int sensorPin = 4;
int relayPin = 15;
volatile long pulse, pulse1;
float cost,volume,credits,flowRate, currentVolume;
unsigned long oldTime;
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

  //---------------------------------------- read packet header bytes:
  int recipient = LoRa.read();        //--> recipient address
  byte sender = LoRa.read();          //--> sender address
  byte incomingLength = LoRa.read();  //--> incoming msg length
  //----------------------------------------

  // Clears Incoming variable data.
  Incoming = "";

  //---------------------------------------- Get all incoming data.
  while (LoRa.available()) {
    Incoming += (char)LoRa.read();
    //----------------------------------------
  }

  //---------------------------------------- Check length for error.
  if (incomingLength != Incoming.length()) {
    Serial.println();
    Serial.println("error: message length does not match length");
    return; //--> skip rest of function
  }
  //----------------------------------------

  //---------------------------------------- Checks whether the incoming data or message for this device.
  if (recipient != LocalAddress) {
    Serial.println();
    Serial.println("This message is not for me.");
    return; //--> skip rest of function
  }
  //----------------------------------------

  //---------------------------------------- if message is for this device, or broadcast, print details:
  Serial.println();
  Serial.println("Received from: 0x" + String(sender, HEX));
  //Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + Incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  //Serial.println("Snr: " + String(LoRa.packetSnr()));
  //----------------------------------------

  // Calls the Processing_incoming_data() subroutine.
  Processing_incoming_data();
}
void Processing_incoming_data()
{
  int pos1 = Incoming.indexOf('/');
  int pos2 = Incoming.indexOf('&');
  int pos3 = Incoming.indexOf('#');
  String readingID = Incoming.substring(0, pos1);
  if (readingID == DeviceID)
  {
    volume = atof(Incoming.substring(pos1 + 1, pos2).c_str());
    credits = atof(Incoming.substring(pos2 + 1, pos3).c_str());
    cost = atof(Incoming.substring(pos3 + 1, Incoming.length()).c_str());
    Serial.printf("Cost per Litre: %.2f ", cost);
    Serial.printf("Available Credits: %.2f ", credits);
    Serial.printf("Volume Consumed: %.2f ", volume);
  }
  getReadings();
}
//----------------------------------------
//           void getReadings()
//----------------------------------------
void getReadings() {
  if ((millis() - oldTime) > 1000) { // Only calculate flow rate once per second
    detachInterrupt(digitalPinToInterrupt(sensorPin));
    flowRate = ((1000.0 / (millis() - oldTime)) * pulse) / FLOW_CALIBRATION;
    currentVolume =  2.126 * pulse1 / 1000;
    Serial.printf("Flow rate: %.2f L/min\n", flowRate);
    Serial.printf("Total volume: %.3f L\n", volume);
    if(flowRate>0)
    {
    volume+=currentVolume;
    sendLoRaData();
    }
    pulse = 0; // Reset pulse count
    oldTime = millis(); // Update oldTime
    attachInterrupt(digitalPinToInterrupt(sensorPin), increase, RISING);
  }
  delay(500);
}
//----------------------------------------
//           void sendLoRaData()
//----------------------------------------
void sendLoRaData() {
  Message = String(DeviceID) + "/" + String(volume) + "&" + String(credits)+ "#" + String(cost);
  sendMessage(Message,Destination_Master);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print("LoRa packet sent!");
  display.setCursor(0, 20);
  display.print("Total Volume:");
  display.setCursor(72, 20);
  display.print(volume);
  display.setCursor(0, 30);
  display.print("Flow Rate:");
  display.setCursor(54, 30);
  display.print(flowRate);
  display.setCursor(0, 40);
  display.print("Avl Credits:");
  display.setCursor(54, 40);
  display.print(credits);
}
void increase() {
  pulse++;
  pulse1++;
}
void setup() {
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Display init Failed"));
    for (;;); 
  }
  display.display();
  delay(2000); 
  display.clearDisplay();  
  LoRa.setPins(ss, rst, dio0);
  Serial.println("Start LoRa init...");
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                                     
  }
  Serial.println("LoRa init succeeded.");
  display.setCursor(0, 10);
  display.print("LoRa Initializing OK!");
  display.display();
  delay(2000);
  display.clearDisplay();                             
}


void loop() {
  onReceive(LoRa.parsePacket());
}
