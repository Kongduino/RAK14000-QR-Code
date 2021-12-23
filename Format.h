#include <Wire.h>
#include "Adafruit_EEPROM_I2C.h" // Click here to get the library: http://librarymanager/All#Adafruit_EEPROM_I2C

#define EEPROM_ADDR 0x50 // the default address
#define MAXADD 262143 // max address in byte
Adafruit_EEPROM_I2C i2ceeprom;

#define POWER_ENABLE WB_IO2
#define EPD_MOSI     MOSI
#define EPD_MISO     -1 // not used
#define EPD_SCK      SCK
#define EPD_CS       SS
#define EPD_DC       WB_IO1
#define SRAM_CS      -1 // not used
#define EPD_RESET    -1 // not used
#define EPD_BUSY     WB_IO4
Adafruit_SSD1680 display(250, 122, EPD_MOSI, EPD_SCK, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_MISO, EPD_BUSY);

// Uncomment if you want to initialize the EEPROM
#define _INITIALIZE_

char myName[9] = {0};
char myText[33] = {0};

#ifdef _INITIALIZE_
void initEEPROM() {
  uint16_t addr = 0x0000;
  // Set a name here if your want, max 8 bytes
  strcpy(myName, "Arthur");
  uint16_t num = i2ceeprom.writeObject(addr, myName);
  Serial.println("Wrote myName: " + String(num) + " bytes");
  addr += 9;
  char firstMessage[15] = "First message!";
  uint8_t ln = 15;
  num = i2ceeprom.writeObject(addr, ln);
  Serial.println("Wrote ln: " + String(num) + " byte");
  addr += 1;
  for (uint8_t ix = 0; ix < ln; ix++) i2ceeprom.write(addr++, (uint8_t)myText[ix]);
  Serial.println("Wrote firstMessage: " + String(ln) + " bytes");
  Serial.println(firstMessage);
}
#endif

void readEEPROM() {
  uint16_t addr = 0x0000;
  i2ceeprom.readObject(addr, myName);
  Serial.print("myName: ");
  Serial.println(myName);
  addr += 9;
  uint8_t ln = i2ceeprom.read(addr);
  addr += 1;
  Serial.print("len: ");
  Serial.println(ln);
  memset(myText, 0, 33);
  for (uint8_t ix = 0; ix < ln; ix++) myText[ix] = (char)i2ceeprom.read(addr++);
  Serial.print("myText: ");
  Serial.println(myText);
}

/**
   @brief Write a text on the display
   @param x x position to start
   @param y y position to start
   @param text text to write
   @param text_color color of text
   @param text_size size of text
*/
void testdrawtext(int16_t x, int16_t y, char *text, uint16_t text_color, uint32_t text_size) {
  display.setCursor(x, y);
  display.setTextColor(text_color);
  display.setTextSize(text_size);
  display.setTextWrap(true);
  display.print(text);
}
