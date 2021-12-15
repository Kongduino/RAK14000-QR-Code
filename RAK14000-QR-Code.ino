#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <SX126x-RAK4630.h>
#include <qrcode.h>
// https://github.com/ricmoo/qrcode/

#include <Adafruit_GFX.h>
#include <Adafruit_EPD.h>
#include "images.h"

#define POWER_ENABLE   WB_IO2
#define EPD_MOSI       MOSI
#define EPD_MISO       -1 // not used
#define EPD_SCK        SCK
#define EPD_CS         SS
#define EPD_DC         WB_IO1
#define SRAM_CS        -1 // not used
#define EPD_RESET      -1 // not used
#define EPD_BUSY       WB_IO4
Adafruit_SSD1680 display(250, 122, EPD_MOSI, EPD_SCK, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_MISO, EPD_BUSY);

// LoRa
static RadioEvents_t RadioEvents;
// Define LoRa parameters
#define RF_FREQUENCY 915000000 // Hz
#define TX_OUTPUT_POWER 22 // dBm
#define LORA_BANDWIDTH 0 // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 12 // [SF7..SF12]
#define LORA_CODINGRATE 1 // [1: 4/5, 2: 4/6,  3: 4/7,  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8 // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0 // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define RX_TIMEOUT_VALUE 100000
#define TX_TIMEOUT_VALUE 3000

char buffer[256];

void OnRxTimeout(void) {
  digitalWrite(LED_BLUE, HIGH);
  Serial.println("\nRx Timeout!");
  delay(500);
  digitalWrite(LED_BLUE, LOW);
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void OnRxError(void) {
  digitalWrite(LED_BLUE, HIGH);
  Serial.println("Rx Error!");
  delay(500);
  digitalWrite(LED_BLUE, LOW);
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void OnRxDone(uint8_t *payload, uint16_t ix, int16_t rssi, int8_t snr) {
  digitalWrite(LED_GREEN, HIGH); // Turn on Green LED
  Serial.println("################################");
  sprintf(buffer, "Message: %s", (char*)payload);
  Serial.println(buffer);
  Serial.println("################################");
  sprintf(buffer, "RSSI: %-d, SNR: %-d", rssi, snr);
  Serial.println(buffer);
  showQRCode((char*)payload);
  digitalWrite(LED_GREEN, LOW); // Turn off Green LED
  Radio.Rx(RX_TIMEOUT_VALUE);
}

QRCode qrcode;
uint8_t version = 3;

void showQRCode(char *msg) {
  display.clearBuffer();
  display.drawBitmap(192, 60, rak_img, 150, 56, EPD_BLACK);
  display.drawBitmap(190, 0, lora_img, 60, 40, EPD_BLACK);
  display.display(true);
  uint16_t sz = qrcode_getBufferSize(version);
  uint8_t qrcodeData[sz];
  qrcode_initText(&qrcode, qrcodeData, version, 0, msg);
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

void setup() {
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
  showQRCode("Hello World!");
  Serial.println("=====================================");
  Serial.println("             LoRa Setup");
  Serial.println("=====================================");
  // Initialize the Radio callbacks
  lora_rak4630_init();
  RadioEvents.TxDone = NULL;
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.TxTimeout = NULL;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;
  RadioEvents.CadDone = NULL;
  // Initialize the Radio
  Radio.Init(&RadioEvents);
  // Set Radio channel
  Radio.SetChannel(RF_FREQUENCY);
  Serial.println("Freq = " + String(RF_FREQUENCY) + " MHz");
  Serial.println("SF = " + String(LORA_SPREADING_FACTOR));
  Serial.println("BW = " + String(LORA_BANDWIDTH) + " KHz");
  Serial.println("CR = 4/" + String(LORA_CODINGRATE));
  Serial.println("Tx Power = " + String(TX_OUTPUT_POWER));
  // Set Radio TX configuration
  Radio.SetTxConfig(
    MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
    true, 0, 0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);
  SX126xSetTxParams(TX_OUTPUT_POWER, RADIO_RAMP_40_US);
  Radio.SetRxConfig(
    MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);
  Serial.println("Starting Radio.Rx");
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void loop() {
}
