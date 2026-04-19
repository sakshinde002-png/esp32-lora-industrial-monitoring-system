#include <SPI.h>
#include <LoRa.h>

#define SS 5
#define RST 14
#define DIO0 26

#define INPUT_PIN 13
#define SLAVE_ID "SLAVE1"

void setup() {

  Serial.begin(115200);

  pinMode(INPUT_PIN, INPUT_PULLDOWN);

  // 🔥 MOST IMPORTANT FIX
  SPI.begin(18, 19, 23, 5);   // SCK, MISO, MOSI, SS

  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }

  Serial.println("Slave Ready");
}

void loop() {

  int state = digitalRead(INPUT_PIN);

  Serial.print("GPIO 13 State: ");
  Serial.println(state ? "HIGH" : "LOW");

  String message = String(SLAVE_ID) + ":" + String(state);

  Serial.print("Sending via LoRa: ");
  Serial.println(message);

  LoRa.beginPacket();
  LoRa.print(message);   // 🔥 println 
  LoRa.endPacket();

  Serial.println("Packet Sent");
  Serial.println("--------------------");

  delay(1000);
}