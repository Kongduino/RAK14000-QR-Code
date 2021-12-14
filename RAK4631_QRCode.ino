#include <Arduino.h>
#include <qrcode.h>
// https://github.com/ricmoo/qrcode/

#include <Adafruit_GFX.h>
#include <Adafruit_EPD.h>

#include "images.h"

#define POWER_ENABLE   WB_IO2
#define EPD_MOSI       MOSI
#define EPD_MISO       -1 // not use
#define EPD_SCK        SCK
#define EPD_CS         SS
#define EPD_DC         WB_IO1
#define SRAM_CS        -1 // not use
#define EPD_RESET      -1 // not use
#define EPD_BUSY       WB_IO4
Adafruit_SSD1680 display(250, 122, EPD_MOSI, EPD_SCK, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_MISO, EPD_BUSY);

QRCode qrcode;
uint8_t version = 3;

void setup(void) {
  pinMode(POWER_ENABLE, INPUT_PULLUP);
  digitalWrite(POWER_ENABLE, HIGH);
  Serial.begin(115200);
  time_t timeout = millis();
  while (!Serial) {
    if ((millis() - timeout) < 5000) {
      delay(100);
    } else {
      break;
    }
  }
  Serial.println("Epaper-QRCode test");
  display.begin();
  display.clearBuffer();
  display.drawBitmap(192, 60, rak_img, 150, 56, EPD_BLACK);
  display.drawBitmap(190, 0, lora_img, 60, 40, EPD_BLACK);
  display.display(true);
  uint16_t sz = qrcode_getBufferSize(version);
  uint8_t qrcodeData[sz];
  qrcode_initText(&qrcode, qrcodeData, version, 0, "HELLO WORLD");
  uint8_t myWidth = qrcode.size;
  uint16_t qrc_wd = myWidth / 2;
  if (myWidth * 2 != qrc_wd) qrc_wd += 1;
  uint16_t qrc_hg = myWidth * 4;
  uint16_t qrc_sz = qrc_wd * qrc_hg, ix = 0;
  unsigned char qrc[qrc_sz];
  // Text version: look at the Serial Monitor :-)
  Serial.print("\n\n\n\n");
  for (uint8_t y = 0; y < myWidth; y++) {
    // Left quiet zone
    Serial.print("        ");
    // Each horizontal module
    uint16_t px = ix;
    for (uint8_t x = 0; x < myWidth; x += 2) {
      // Print each module (UTF-8 \u2588 is a solid block)
      Serial.print(qrcode_getModule(&qrcode, x, y) ? "\u2588\u2588" : "  ");
      uint8_t c = 0;
      if (qrcode_getModule(&qrcode, x, y)) c = 0xF0;
      if (x + 1 < myWidth && qrcode_getModule(&qrcode, x + 1, y)) {
        c += 0x0F;
        Serial.print("\u2588\u2588");
      } else Serial.print("  ");
      qrc[ix++] = c;
    }
    memcpy(qrc + ix, qrc + px, qrc_wd);
    px = ix;
    ix += qrc_wd;
    memcpy(qrc + ix, qrc + px, qrc_wd);
    px = ix;
    ix += qrc_wd;
    memcpy(qrc + ix, qrc + px, qrc_wd);
    px = ix;
    ix += qrc_wd;
    Serial.print("\n");
  }
  // Bottom quiet zone
  Serial.print("\n\n\n\n");
  display.drawBitmap(0, 0, qrc, qrc_wd * 8, qrc_hg, EPD_BLACK);
  display.display(true);
}

void loop() {
}
