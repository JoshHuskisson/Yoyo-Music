#include <Arduino.h>

#include "spi_driver.h"
#include "nrf24.h"

#include "i2c.h"
#include "sam.h"

const uint8_t payload_size = 2;
uint16_t sensor_data = 0;

const int channel = 1;

void setup() {
  Serial.begin(9600);
  //while (!Serial);

  nrf24_init_rx(payload_size);
  
}

void loop() {
  //if (nrf24_receive(&sensor_data, sizeof(sensor_data))) {
    //int midi_note = map(sensor_data, -34000, 34000, 45, 57);
    //usbMIDI.sendNoteOn(midiNote, 99, channel);
  //}
  nrf24_receive(&sensor_data, sizeof(sensor_data));
  Serial.println(sensor_data);
  delay(100);

}
