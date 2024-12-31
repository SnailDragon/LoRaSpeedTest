#include <Arduino.h>
#include <Wire.h>

#include "RadioLib.h"
#include "SSD1306Wire.h"

#define OLED_DISPLAY_ADDR 0x3c

#define RADIO_CS 18
#define RADIO_DIO0 26
#define RADIO_RST 23

SSD1306Wire display(OLED_DISPLAY_ADDR, SDA, SCL);

SX1278 radio = new Module(RADIO_CS, RADIO_DIO0, RADIO_RST);
uint8_t radio_state = 1;

// interrupt
volatile bool receivedFlag = false;

void setFlag(void) { receivedFlag = true; }

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);  // delay for debug
  Serial.println("\n");

  // set up display
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  // set up radio
  radio_state = radio.begin(434.0, 125.0, 9, 7, 18, 10, 8, 0);
  if (radio_state == RADIOLIB_ERR_NONE) {
    Serial.println("Success, SX1278 is connected");
  } else {
    Serial.println("Failed with code" + String(radio_state));
    while (1) delay(1000);
  }
  Serial.println("Starting...");

  radio.setPacketReceivedAction(setFlag);
  radio_state = radio.startReceive();
  if (radio_state == RADIOLIB_ERR_NONE) {
    Serial.println("Receiving..");
  } else {
    Serial.println("Receive failed with code, " + String(radio_state));
    while (1) delay(1000);
  }

  // ui
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 10, "It: 0 R: 0");
  display.drawString(0, 22, "Rate: _ bps (B: _)");
  display.drawString(0, 34, "Avg: _");
  display.drawString(0, 46, "Packet: ");
  display.display();
}

uint32_t it = 0;
uint32_t bits_received = 0;
unsigned long last_calc = 0;
int packet_bps = 0;
int best_bps = 0;
float average_bps = 0;
uint32_t reading_count = 0;
uint32_t received_count = 0;
String packet;
void loop() {
  if (it % 10000 == 0) Serial.println(String(it) + "\t" + String(radio_state));
  // put your main code here, to run repeatedly:

  // radio
  if (receivedFlag) {
    receivedFlag = false;
    received_count++;

    // size_t num_bytes = radio.getPacketLength();
    // uint8_t byte_arr[num_bytes];
    // radio.readData(byte_arr, num_bytes);
    bits_received += radio.getPacketLength();
    radio.readData(packet);

    Serial.println("[" + String(bits_received) + "] " + packet);
  }

  if (millis() - last_calc > 1000) {
    if (bits_received != 0) {
      packet_bps = bits_received;
      best_bps = max(packet_bps, best_bps);
      average_bps =
          (average_bps * reading_count + packet_bps) / (reading_count + 1);
      reading_count++;
      bits_received = 0;

      // ui
      display.clear();
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(0, 10,
                         "It: " + String(it) + " R: " + String(received_count));
      display.drawString(
          0, 22,
          "Rate: " + String(packet_bps) + "bps (B: " + String(best_bps) + ")");
      display.drawString(0, 34, "Avg: " + String(average_bps));
      display.drawString(0, 46, "Packet: " + packet);
      display.display();
    }
    last_calc = millis();
  }

  it++;
}
