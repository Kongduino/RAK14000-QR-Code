#include <Arduino.h>
//#include <Adafruit_TinyUSB.h>
#include <SX126x-RAK4630.h>
#include <qrcode.h>
// https://github.com/ricmoo/qrcode/

#include <Adafruit_GFX.h>
#include <Adafruit_EPD.h>
#include "images.h"
#include "Format.h"
#include <ArduinoJson.h>

void showQRCode(char *, boolean);

// LoRa
static RadioEvents_t RadioEvents;
// Define LoRa parameters
#define RF_FREQUENCY 868000000 // Hz
#define TX_OUTPUT_POWER 22 // dBm
#define LORA_BANDWIDTH 0 // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 10 // [SF7..SF12]
#define LORA_CODINGRATE 1 // [1: 4/5, 2: 4/6,  3: 4/7,  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8 // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0 // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define RX_TIMEOUT_VALUE 100000
#define TX_TIMEOUT_VALUE 3000

char buffer[256];
// #define BUZZER_CONTROL WB_IO3

void OnRxTimeout(void) {
  digitalWrite(LED_BLUE, HIGH);
  Serial.println("\nRx Timeout!");
  delay(500);
  digitalWrite(LED_BLUE, LOW);
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void OnRxError(void) {
  Serial.println("Rx Error!");
  digitalWrite(LED_BLUE, HIGH);
  delay(200);
  digitalWrite(LED_BLUE, LOW);
  delay(200);
  digitalWrite(LED_BLUE, HIGH);
  delay(200);
  digitalWrite(LED_BLUE, LOW);
  delay(200);
  digitalWrite(LED_BLUE, HIGH);
  delay(200);
  digitalWrite(LED_BLUE, LOW);
  delay(200);
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void OnRxDone(uint8_t *payload, uint16_t ix, int16_t rssi, int8_t snr) {
  digitalWrite(LED_GREEN, HIGH); // Turn on Green LED
  Serial.println(F("################################"));
  sprintf(buffer, "Message: %s", (char*)payload);
  Serial.println(buffer);
  hexDump((char*)payload, strlen((char*)payload));
  Serial.println(F("################################"));
  sprintf(buffer, "RSSI: %-d, SNR: %-d", rssi, snr);
  Serial.println(buffer);
  // Serial.print("Deserializing...");
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, (char*)payload);
  // Serial.println(" done!");
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    digitalWrite(LED_GREEN, LOW); // Turn off Green LED
    Radio.Rx(RX_TIMEOUT_VALUE);
    return;
  } // else Serial.println("deserializeJson() successful!");
  const char* UUID = doc["to"];
  if (!UUID) {
    Serial.println("No `to` field!");
    return;
  }
  Serial.printf("My UUID: `%s`\nRecipient: %s\n", myPlainTextUUID, UUID);

  uint8_t cp0, cp1;
  cp0 = strcmp(myPlainTextUUID, UUID);
  cp1 = strcmp("*", UUID);
  if (cp0 == 0 || cp1 == 0) {
    // tone(BUZZER_CONTROL, 393);
    // delay(300);
    // noTone(BUZZER_CONTROL);
    // delay(300);
    // tone(BUZZER_CONTROL, 393);
    // delay(300);
    // noTone(BUZZER_CONTROL);
    const char* msg = doc["msg"];
    sprintf(buffer, "Message: %s", msg);
    Serial.println(buffer);
    showQRCode(buffer, false);
    display.drawBitmap(192, 0, rak_img, 150, 56, EPD_BLACK);
    testdrawtext(125, 100, (char*)msg, EPD_BLACK, 1);
    display.display(true);
  } else Serial.printf(" --> Not for me! [%s vs %s]", myPlainTextUUID, UUID);
  digitalWrite(LED_GREEN, LOW); // Turn off Green LED
  Radio.Rx(RX_TIMEOUT_VALUE);
}

QRCode qrcode;
uint8_t version = 3;

void showQRCode(char *msg, bool showASCII) {
  display.clearBuffer();
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
  if (showASCII) Serial.print("\n\n\n\n");
  for (uint8_t y = 0; y < myWidth; y++) {
    // Left quiet zone
    Serial.print("        ");
    // Each horizontal module
    uint16_t px = ix;
    for (uint8_t x = 0; x < myWidth; x += 2) {
      // Print each module (UTF-8 \u2588 is a solid block)
      if (showASCII) Serial.print(qrcode_getModule(&qrcode, x, y) ? "\u2588\u2588" : "  ");
      uint8_t c = 0;
      if (qrcode_getModule(&qrcode, x, y)) c = 0xF0;
      if (x + 1 < myWidth && qrcode_getModule(&qrcode, x + 1, y)) {
        c += 0x0F;
        if (showASCII) Serial.print("\u2588\u2588");
      } else {
        if (showASCII) Serial.print("  ");
      }
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
    if (showASCII) Serial.print("\n");
  }
  // Bottom quiet zone
  if (showASCII) Serial.print("\n\n\n\n");
  display.drawBitmap(0, 0, qrc, qrc_wd * 8, qrc_hg, EPD_BLACK);
  display.display(true);
}

void setup() {
  time_t timeout = millis();
  while (!Serial) {
    if ((millis() - timeout) < 5000) {
      delay(100);
    } else {
      break;
    }
  }
  Serial.begin(115200);
  delay(300);
  Serial.println(F("====================================="));
  Serial.println("        QR Code EPD LoRa Test");
  Serial.println(F("====================================="));
  Serial.println("          Turning on modules");
  pinMode(WB_IO2, INPUT_PULLUP); // EPD
  digitalWrite(WB_IO2, HIGH);
  delay(300);
  //  pinMode(WB_IO6, INPUT_PULLUP);
  //  digitalWrite(WB_IO6, HIGH);
  //  delay(300);
  // pinMode(BUZZER_CONTROL, OUTPUT);
  // delay(300);
  Serial.println(F("====================================="));
  if (i2ceeprom.begin(EEPROM_ADDR)) {
    // you can put a different I2C address here, e.g. begin(0x51);
    Serial.println("Found I2C EEPROM");
  } else {
    Serial.println("I2C EEPROM not identified ... check your connections?\r\n");
    while (1) {
      delay(10);
    }
  }
  pinMode(PIN_SERIAL1_RX, INPUT);
  if (digitalRead(PIN_SERIAL1_RX) == HIGH) initEEPROM();
  readEEPROM();
  Serial.println("Epaper-QRCode test\n======================");
  display.begin();
  memset(buffer, 0, 32);
  memset(myPlainTextUUID, 0, (UUIDlen * 2 + 2));
  strcpy(buffer, "UUID: ");
  uint8_t addr = 6, ix = 0;
  char alphabet[17] = "0123456789ABCDEF";
  for (uint8_t i = 0; i < UUIDlen; i++) {
    char c = myUUID[i];
    buffer[addr++] = alphabet[c >> 4];
    buffer[addr++] = alphabet[c & 0x0f];
    myPlainTextUUID[ix++] = alphabet[c >> 4];
    myPlainTextUUID[ix++] = alphabet[c & 0x0f];
  }
  uint8_t ln = strlen(buffer);
  hexDump(buffer, ln);
  hexDump(myPlainTextUUID, UUIDlen * 2 + 1);
  showQRCode(buffer, true);
  display.drawBitmap(192, 0, rak_img, 150, 56, EPD_BLACK);
  testdrawtext(125, 60, buffer + 6, EPD_BLACK, 1);
  display.display(true);
  Serial.println(F("====================================="));
  Serial.println("             LoRa Setup");
  Serial.println(F("====================================="));
  // Initialize the Radio callbacks
#if defined NRF52_SERIES
  lora_rak4630_init();
#else
  lora_rak11300_init();
#endif
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
  Serial.println("Freq = " + String(RF_FREQUENCY / 1e6) + " MHz");
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
