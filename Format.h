#include "ble_gap.h"
#include <bluefruit.h>

BLEUart g_BleUart;

// Uncomment if you want to initialize the EEPROM
#define _INITIALIZE_
// Uncomment if you want to use the EEPROM
#define _USE_EEPROM_

#ifdef _USE_EEPROM_
#include <Wire.h>
#include "Adafruit_EEPROM_I2C.h" // Click here to get the library: http://librarymanager/All#Adafruit_EEPROM_I2C

#define EEPROM_ADDR 0x50 // the default address
#define MAXADD 262143 // max address in byte
Adafruit_EEPROM_I2C i2ceeprom;
#endif

#define EPD_MOSI  MOSI
#define EPD_MISO  -1 // not used
#define EPD_SCK   SCK
#define EPD_CS    SS
#define EPD_DC    WB_IO1
#define SRAM_CS   -1 // not used
#define EPD_RESET -1 // not used
#define EPD_BUSY  WB_IO4
Adafruit_SSD1680 display(250, 122, EPD_MOSI, EPD_SCK, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_MISO, EPD_BUSY);

char myUUID[5] = {'_'};
char myPlainTextUUID[11] = "0102030405060708";

void hexDump(char *, uint16_t);

#ifdef _USE_EEPROM_
#ifdef _INITIALIZE_
void initEEPROM() {
  ble_gap_addr_t ble_addr;
  sd_ble_gap_addr_get(&ble_addr);
  uint8_t px = 1, py = 3, ix;
  myUUID[1] = ble_addr.addr[2];
  myUUID[2] = ble_addr.addr[1];
  myUUID[3] = ble_addr.addr[0];
  Serial.printf(
    "UUID: %02x %02x %02x %02x %02x %02x %02x %02x\n",
    myUUID[0], myUUID[1], myUUID[2], myUUID[3], myUUID[4], myUUID[5], myUUID[6], myUUID[7]
  );
  uint16_t addr = 0x0000;
  // Set a name here if your want, max 8 bytes
  uint16_t num = i2ceeprom.writeObject(addr, myUUID);
  Serial.println("Wrote myUUID: " + String(num) + " bytes");
}
#endif

void readEEPROM() {
  uint16_t addr = 0x0000;
  i2ceeprom.readObject(addr, myUUID);
  Serial.println("myUUID:");
  hexDump(myUUID, 8);
}
#endif

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

void hexDump(char *buf, uint16_t len) {
  char alphabet[17] = "0123456789abcdef";
  Serial.print(F("   +------------------------------------------------+ +----------------+\n"));
  Serial.print(F("   |.0 .1 .2 .3 .4 .5 .6 .7 .8 .9 .a .b .c .d .e .f | |      ASCII     |\n"));
  for (uint16_t i = 0; i < len; i += 16) {
    if (i % 128 == 0)
      Serial.print(F("   +------------------------------------------------+ +----------------+\n"));
    char s[] = "|                                                | |                |\n";
    uint8_t ix = 1, iy = 52;
    for (uint8_t j = 0; j < 16; j++) {
      if (i + j < len) {
        uint8_t c = buf[i + j];
        s[ix++] = alphabet[(c >> 4) & 0x0F];
        s[ix++] = alphabet[c & 0x0F];
        ix++;
        if (c > 31 && c < 128) s[iy++] = c;
        else s[iy++] = '.';
      }
    }
    uint8_t index = i / 16;
    if (i < 256) Serial.write(' ');
    Serial.print(index, HEX); Serial.write('.');
    Serial.print(s);
  }
  Serial.print(F("   +------------------------------------------------+ +----------------+\n"));
}
