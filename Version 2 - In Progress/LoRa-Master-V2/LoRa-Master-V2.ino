#include <SPI.h>
#include <M5LoRa.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "Link_Guest";
char pass[] = "easyaccess";

#define ss 5
#define rst 17
#define dio0 2

#define BLYNK_TEMPLATE_ID "TMPL3oljYgoST"
#define BLYNK_TEMPLATE_NAME "Smart Water Meter"
#define BLYNK_AUTH_TOKEN "vmtVlMjWs0_b4UBgggEqUP3TCvpa3ZcL"
#define BLYNK_PRINT Serial
BlynkTimer timer;

String Incoming = "";
String Message = "";

byte LocalAddress = 0x01;
byte Destination_ESP32_Slave_1 = 0x02;
byte Destination_ESP32_Slave_2 = 0x03;

unsigned long previousMillis_SendMSG = 0;
const long interval_SendMSG = 1000;

byte Slv = 0;

float cost1, volume1, credits1;
float cost2, volume2, credits2;
float remainingVolume1, remainingVolume2;

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V1, V3, V4, V5);
}

BLYNK_WRITE(V1)
{
  credits1 = param.asFloat();
  sendDataToClient();
  computeRemainingVolume();

}
BLYNK_WRITE(V3)
{
  volume1 = param.asFloat();
  sendDataToClient();
}
BLYNK_WRITE(V4)
{
  remainingVolume1 = param.asFloat();
}
BLYNK_WRITE(V5)
{
  cost1 = param.asFloat();
  computeRemainingVolume();
  sendDataToClient();
}

void computeRemainingVolume()
{
  remainingVolume1 = credits1 / cost1;
  Blynk.virtualWrite(V4, remainingVolume1);
}

void sendDataToClient()
{
  Message = "Slave01/" + String(volume1) + "&" + String(credits1) + "#" + String(cost1);
  Serial.println(" : " + Message);
  sendMessage(Message, Destination_ESP32_Slave_1);
}

void sendDataToBlynk()
{
  Blynk.virtualWrite(V1, credits1);
  Blynk.virtualWrite(V3, volume1);
  Blynk.virtualWrite(V4, remainingVolume1);
  Blynk.virtualWrite(V5, cost1);
}

void ResetToDefaultData()
{
  cost1 = 10, volume1 = 0, credits1 = 100, cost2 = 10, volume2 = 0, credits2 = 100;
  sendDataToBlynk();
}

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
  }
  //----------------------------------------

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
  if (Incoming.startsWith("Hi")) {
    if (sender == Destination_ESP32_Slave_1)
    {
      Message = "";
      Message = "Slave01/" + String(volume1) + "&" + String(credits1) + "#" + String(cost1);
      Serial.println(" : " + Message);
      sendMessage(Message, Destination_ESP32_Slave_1);
    }
    else
    {
      Message = "";
      Message = "Slave02/" + String(volume2) + "&" + String(credits2) + "#" + String(cost2);
      Serial.println(" : " + Message);
      sendMessage(Message, Destination_ESP32_Slave_2);
    }
  }
  else {
    int pos1 = Incoming.indexOf('/');
    int pos2 = Incoming.indexOf('&');
    int pos3 = Incoming.indexOf('#');
    String readingID = Incoming.substring(0, pos1);
    if (readingID == "Slave01")
    {
      volume1 = atof(Incoming.substring(pos1 + 1, pos2).c_str());
      credits1 = atof(Incoming.substring(pos2 + 1, pos3).c_str());
      cost1 = atof(Incoming.substring(pos3 + 1, Incoming.length()).c_str());
      remainingVolume1 = credits1 / cost1;
      Serial.printf("Available Credits: %.2f ", credits1);
      Serial.printf("Volume Consumed: %.2f ", volume1);
      Serial.printf("Cost per Litre: %.2f ", cost1);
      if (credits1 == 0)
      {
        volume1 = 0;
      }
    }
    else if (readingID == "Slave02")
    {
      volume2 = atof(Incoming.substring(pos1 + 1, pos2).c_str());
      credits2 = atof(Incoming.substring(pos2 + 1, pos3).c_str());
      cost2 = atof(Incoming.substring(pos3 + 1, Incoming.length()).c_str());
      remainingVolume2 = credits2 / cost2;
      Serial.printf("Available Credits: %.2f ", credits2);
      Serial.printf("Volume Consumed: %.2f ", volume2);
      Serial.printf("Cost per Litre: %.2f ", cost2);
    }
    else
    {
    }
    sendDataToBlynk();
  }
}


void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  LoRa.setPins(ss, rst, dio0);

  Serial.println("Start LoRa init...");
  if (!LoRa.begin(433E6)) {             // initialize ratio at 915 or 433 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }
  Serial.println("LoRa init succeeded.");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(1000L, sendDataToBlynk);
}

void loop() {
  onReceive(LoRa.parsePacket());
  Blynk.run();
  timer.run();
}
