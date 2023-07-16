//----------------------------------------
//        Defining Header Files
//----------------------------------------

#include <SPI.h>
#include "LoRa_STM32.h"
//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306_STM32.h>

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

#define OLED_RESET 4  
Adafruit_SSD1306 display(OLED_RESET);

//----------------------------------------
//      Defining PIN Config Variables
//----------------------------------------

const int sensorPin = PA11;
const int IN1 = PB8;
const int IN2 = PB9;

//----------------------------------------
//      Defining LoRa Variables
//----------------------------------------

String Incoming = "";
String Message = "";
String DeviceID = "Slave01";
byte LocalAddress = 0x02;       //--> address of this device (Slave 1).
bool executed = false;
bool valveOn = false;
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

//-----------------------------------------------
// Function to generate Pulse for Solenoid valve
//-----------------------------------------------

void positivePulse()
{
  DEBUG_PRINTLN("Generating Positive Pulse.!!");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
}

void negativePulse()
{
  DEBUG_PRINTLN("Genrating Negative Pulse.!!");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void pulseZero()
{
  DEBUG_PRINTLN("Energy = 0");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, HIGH);
}

void ValveON() {
  valveOn = true;
  DEBUG_PRINTLN("Valve Turned ON");
  positivePulse();
  delay(2000);
  pulseZero();
  delay(2000);
}

void ValveOFF()
{
  valveOn = false;
  DEBUG_PRINTLN("Valve Turned OFF");
  negativePulse();
  delay(2000);
  pulseZero();
  delay(2000);
}

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
//  getReadings() for Flow Calculations
//----------------------------------------
void getReadings() {
  if (totalCredits > 0)
  {
    executed = false;
    if ((millis() - oldTime) > 1000) {                                         // Only calculate flow rate once per second
      detachInterrupt(digitalPinToInterrupt(sensorPin));                       // Disable interrupt to prevent further pulse count
      flowRate = ((1000.0 / (millis() - oldTime)) * pulse) / FLOW_CALIBRATION; // Calculate flow rate in liters per minute
      oldTime = millis();                                                      // Update oldTime
      flowLitres =  2.520 * pulse1 / 1000;                                     // Calculate the flow in liters since the last calculation
      flowingVolume = flowLitres;
      if(flowRate == 0)
      {
        flowingVolume = 0;
      }
      totalLitres = totalLitresOld + flowLitres;                               // Update the total volume consumed by adding the flow since the last calculation
      flowCredits = flowLitres * cost;                                         // Calculate the credits consumed based on the flow and cost per liter
      totalCredits = creditsOld - flowCredits;                                 // Update the available credits by subtracting the credits consumed
      pulse = 0;                                                               // Reset the pulse count and reattach the interrupt
      attachInterrupt(digitalPinToInterrupt(sensorPin), increase, RISING);
    }
    if (valveOn == false)
    {
      ValveON();      // TurnOn the Solenoid Valve For Water Flow
    }
    if (flowRate > 0)
    {
      DisplayData();
      sendLoRaData();                                                        // if Flow is detected, sends the Data to Server Node Over LoRa Network
      DEBUG_PRINTLN("Sending Data over LoRa");
    }
    else
    {
      DisplayData();                                                          // Update the Data to Integrated Display
    }
  }
  else {
    // No credits available, reset values and send LoRa data
    totalCredits = 0;
    totalLitres = 0;
    flowLitres = 0;
    flowCredits = 0;
    pulse = 0; pulse1 = 0;
    oldTime = millis();
    // Send LoRa data only once when there are no available credits
    if (!executed) {
      sendLoRaData();
      DisplayNoData();                // Display "No Data" message on the Integrate Display
      executed = true;
    }
    ValveOFF(); // Turn OFF the Solenoid Valve to prevent Water Flow
    getDataFromLoRa();              // Request updated data from the master device over LoRa
    onReceive(LoRa.parsePacket());  // Check for any incoming LoRa packets
    DEBUG_PRINTLN("Waiting for packets");
    delay(1000);                    // Delay for stability and wait for incoming packets
  }
}

//---------------------------------------------------------------------------------
//           Function To send the Update Data to Master Node
//---------------------------------------------------------------------------------
void sendLoRaData() {
  Message = String(DeviceID) + "/" + String(totalLitres) + "&" + String(totalCredits) + "#" + String(cost);
  sendMessage(Message, Destination_Master);
}
//-----------------------------------------------------------------------------------
//           Function To Update Pulse COunt Used In Interrupt
//-----------------------------------------------------------------------------------
void increase() {
  pulse++;
  pulse1++;
}
//------------------------------------------------------------------------------------
//           Function To Request updated data from the master device over LoRa
//------------------------------------------------------------------------------------
void getDataFromLoRa()
{
  sendMessage("Hi", Destination_Master);
}
//------------------------------------------------------------------------------------
//           Function that Diplays the Data over Integrated Screen
//------------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------------
//           Function To Display "No Data" message on the Integrated Screen
//------------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------------
//                              Setup() to Initialise
//------------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  DEBUG_PRINTLN("Start LoRa init...");
  if (!LoRa.begin(433E6)) {
    DEBUG_PRINTLN("LoRa init failed. Check your connections.");
    while (true);
  }
  DEBUG_PRINTLN("LoRa init succeeded.");
  pinMode(sensorPin, INPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(sensorPin), increase, RISING);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  delay(2000);
  display.clearDisplay();
  oldTime = millis();
  getDataFromLoRa();
}
//------------------------------------------------------------------------------------
//                              Setup() to Iterate
//------------------------------------------------------------------------------------
void loop() {
  onReceive(LoRa.parsePacket());
  getReadings();
}
