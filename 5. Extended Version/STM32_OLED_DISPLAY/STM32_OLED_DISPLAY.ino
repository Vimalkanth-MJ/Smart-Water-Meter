/*
 * 
 * Wiring: SCLK  .. B6
 *         SDA   .. B7      
 *         
 */

#include <Wire.h>
#include <Adafruit_SSD1306_STM32.h>

#define OLED_RESET 4  
Adafruit_SSD1306 display(OLED_RESET);

void setup() {


  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x64)

  // Clear the buffer.
  display.clearDisplay();

}


void loop() {
    // Draw a test
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("OLED  with STM32F103C");
   display.setCursor(0,12);
   display.println("-------Welcome-------");
  display.println("  Vimalkanth");
  display.println("   and Automation");
  display.println("PHOENIX!!!!");
  display.setTextSize(2);
  display.setCursor(4,50);
  display.print(millis()/1000);
  display.display();
  delay(1000);
  display.clearDisplay();

}
