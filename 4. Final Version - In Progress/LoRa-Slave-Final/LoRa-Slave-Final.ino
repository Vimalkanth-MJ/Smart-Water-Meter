//----------------------------------------
//        Defining Header Files
//----------------------------------------
#include <M5Stack.h>
#include <SPI.h>
#include <M5LoRa.h>
#include "Fonts.h"
#include "xbm.h"
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
//      Defining PIN Config Variables
//----------------------------------------
#define ss 5
#define rst 17
#define dio0 2
const int sensorPin = 16;
const int IN1 = 25;
const int IN2 = 26;
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
float flowRate, totalLitres, totalLitresOld, flowLitres, flowingVolume;
unsigned long oldTime;
unsigned long previousTime = 0;
int idleDisplay_Screen_Update = 0;
bool clearedDisplayWhileFlowingScreen = false;
bool clearedTotalWaterConsumptionScreen = false;
bool clearedDisplayCreditsScreen = false;
bool clearedDisplayRemainingVolumeScreen = false;
unsigned long idleDisplay_Screen_Update_Timer = millis();
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
      flowingVolume = 2.34 * pulse / 1000;
      flowLitres =  2.34 * pulse1 / 1000;                                     // Calculate the flow in liters since the last calculation
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
      displayWhileFlowing();
      sendLoRaData();                                                        // if Flow is detected, sends the Data to Server Node Over LoRa Network
      DEBUG_PRINTLN("Sending Data over LoRa");
    }
    else
    {
      idleDisplayData();                                                          // Update the Data to Integrated Display
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
      noCreditsDisplay();                // Display "No Data" message on the Integrate Display
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

void DisplayTotalWaterConsumption() {
  if (!clearedTotalWaterConsumptionScreen) {
    M5.Lcd.fillScreen(TFT_BLACK);
    clearedTotalWaterConsumptionScreen = true;
  }
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setFreeFont(FSSB9);
  M5.Lcd.drawString("TOTAL WATER CONSUMPTION", 20, 40, 1);
  M5.Lcd.setTextDatum(ML_DATUM);
  M5.Lcd.drawXBitmap(25, 80, waterConsumptionLogo, waterConsumptionLogoWidth, waterConsumptionLogoHeight, 0X07FF);
  M5.Lcd.setTextColor(0X03EF, TFT_BLACK);
  M5.Lcd.setFreeFont(FSSB24);
  M5.Lcd.drawString(String(totalLitres) + " L", 140, 130, 1);
}

void displayCredits()
{
  if (!clearedDisplayCreditsScreen) {
    M5.Lcd.fillScreen(TFT_BLACK);
    clearedDisplayCreditsScreen = true;
  }
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setFreeFont(FSSB12);
  M5.Lcd.drawString("REMAINING CREDITS", 30, 40, 1);
  M5.Lcd.setTextDatum(ML_DATUM);
  M5.Lcd.drawXBitmap(25, 80, creditsLogo, creditsLogoWidth, creditsLogoHeight, 0X2589);
  if (totalCredits < 5)
  {
    M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
  }
  else
  {
    M5.Lcd.setTextColor(0X03E0, TFT_BLACK);
  }
  M5.Lcd.setFreeFont(FSSB24);
  M5.Lcd.drawString(String(totalCredits), 140, 120, 1);
  M5.Lcd.setFreeFont(FSSB9);
  M5.Lcd.drawString("KES", 265, 130, 1);
}

void displayRemainingVolume()
{
  if (!clearedDisplayRemainingVolumeScreen) {
    M5.Lcd.fillScreen(TFT_BLACK);
    clearedDisplayRemainingVolumeScreen = true;
  }
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setFreeFont(FSSB12);
  M5.Lcd.drawString("REMAINING VOLUME", 35, 40, 1);
  M5.Lcd.setFreeFont(FSSB9);
  M5.Lcd.drawString("For Available Credits", 70, 70, 1);
  M5.Lcd.setTextDatum(ML_DATUM);
  M5.Lcd.drawXBitmap(40, 110, RcreditsLogo, creditsLogoWidth, RcreditsLogoHeight, 0X03EF);
  M5.Lcd.setTextColor(0X07FF, TFT_BLACK);
  M5.Lcd.setFreeFont(FSSB24);
  float remaining = totalCredits / cost;
  M5.Lcd.drawString(String(remaining), 145, 145, 1);
  M5.Lcd.setFreeFont(FSSB9);
  M5.Lcd.drawString("Liters", 245, 155, 1);
}

void displayWhileFlowing()
{
  clearedTotalWaterConsumptionScreen = false;
  clearedDisplayCreditsScreen = false;
  clearedDisplayRemainingVolumeScreen = false;

  if (!clearedDisplayWhileFlowingScreen) {
    M5.Lcd.fillScreen(TFT_BLACK);
    clearedDisplayWhileFlowingScreen = true;
  }

  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setFreeFont(FSSB12);
  M5.Lcd.drawString("FLOW RATE", 5, 25, 1);
  M5.Lcd.setTextDatum(ML_DATUM);
  M5.Lcd.drawXBitmap(30, 60, flowLogo, flowLogoWidth, flowLogoHeight, 0X03EF);
  M5.Lcd.setTextColor(0XFBE4, TFT_BLACK);
  M5.Lcd.setFreeFont(FSSB18);
  char flowRateData[5];
  dtostrf(flowRate, 5, 2, flowRateData);
  M5.Lcd.drawString(flowRateData, 20, 200, 1);
  M5.Lcd.setFreeFont(FSSB9);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.drawString("L/min", 100, 205, 1);
  M5.Lcd.drawFastVLine(165, 0, 255, 0XC618);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setFreeFont(FSSB12);
  M5.Lcd.drawString("FLOWING", 180, 30, 1);
  M5.Lcd.drawString("VOLUME", 185, 55, 1);
  M5.Lcd.setTextDatum(ML_DATUM);
  M5.Lcd.drawXBitmap(200, 70, flowVolumeLogo, flowVolumeLogoWidth, flowVolumeLogoHeight, 0X03EF);
  M5.Lcd.setTextColor(0XA254, TFT_BLACK);
  M5.Lcd.setFreeFont(FSSB18);
  M5.Lcd.drawString(String(flowingVolume), 200, 200, 1);
  M5.Lcd.setFreeFont(FSSB9);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.drawString("Liters", 275, 205, 1);
}

void noCreditsDisplay()
{
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
  M5.Lcd.setFreeFont(FF18);
  M5.Lcd.drawString("Available Credits : 0", 40, 5, 1);
  M5.Lcd.setTextDatum(MC_DATUM);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.drawString("Please Recharge Using QR", 10, 40, 1);
  M5.Lcd.qrcode("https://recharge.vimal.codes", 75, 60, 175, 6);
}

void idleDisplayData()
{
  clearedDisplayWhileFlowingScreen = false;
  if (millis() - idleDisplay_Screen_Update_Timer > 2000)
  {

    idleDisplay_Screen_Update++;

    if (idleDisplay_Screen_Update > 2)
    {
      idleDisplay_Screen_Update = 0;
      clearedTotalWaterConsumptionScreen = false;
      clearedDisplayCreditsScreen = false;
      clearedDisplayRemainingVolumeScreen = false;
    }
    idleDisplay_Screen_Update_Timer = millis();
  }
  if ( idleDisplay_Screen_Update == 0)
  {
    DisplayTotalWaterConsumption();
  }
  else if ( idleDisplay_Screen_Update == 1)
  {
    displayCredits();
  }
  else if ( idleDisplay_Screen_Update == 2)
  {
    displayRemainingVolume();
  }
}


void setup() {
  Serial.begin(115200);
  M5.begin();        // Init M5Core.
  M5.Power.begin();
  LoRa.setPins(ss, rst, dio0);
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
  oldTime = millis(); // Initialize oldTime variable
  getDataFromLoRa();
}

void loop() {
  onReceive(LoRa.parsePacket());
  getReadings();
}
