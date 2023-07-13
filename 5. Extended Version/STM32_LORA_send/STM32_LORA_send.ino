#include <SPI.h>
#include "LoRa_STM32.h"

int counter = 0;

void setup() {
  Serial.begin(115200);
  pinMode(PC13, OUTPUT);
  Serial.println("Start LoRa init...");
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (1);
  }
  Serial.println("LoRa init succeeded.");
}

void loop() {
  LoRa.beginPacket();
  LoRa.print("Hi");
  LoRa.print(counter);
  LoRa.endPacket();
  counter++;
  digitalWrite(PC13, LOW);
  delay(100);
  digitalWrite(PC13, HIGH);
  delay(5000);
  Serial.println("Data Sent");
}
