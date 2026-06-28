#include <Wire.h>
#include <math.h>
#include <Arduino.h>

// 2.4GHz radio module
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#include "i2c.h"
#include "sam.h"

void gyroInterface();
void getAngle(int Ax, int Ay, int Az);
int getCalibrateVals(uint8_t addr, uint8_t reg, int samples);

// MPU5060 gyroscope module
const uint8_t ACCEL_XOUT_H = 0x3B;
const uint8_t ACCEL_YOUT_H = 0x3D;
const uint8_t GYRO_XOUT_H = 0x43;
const uint8_t GYRO_YOUT_H = 0x45;
const uint8_t PWR_MGMT_1 = 0x6B;
const int MPU1 = 0x68;    
const int MPU2 = 0x69;
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;
int16_t AcX1, AcY1, AcZ1, Tmp1, GyX1, GyY1, GyZ1;
int16_t AcX2, AcY2, AcZ2, Tmp2, GyX2, GyY2, GyZ2;
int AcXcal1, AcYcal1, AcZcal1, GyXcal1, GyYcal1, GyZcal1, tcal1;
int AcXcal2, AcYcal2, AcZcal2, GyXcal2, GyYcal2, GyZcal2, tcal2;
double t, tx, tf, pitch, roll;

// 2.4GHz radio module
// PB8: CE
// PB9: CSN
RF24 radio(6, 7); // CE, CSN
const byte address[6] = "00001";

void setup() {
  delay(3000);
  Serial.begin(9600);
  while (!Serial);
  
  // Gyro Modules
  i2c_setup();
  i2c_reg_write(MPU1, PWR_MGMT_1, 0x0);
  i2c_reg_write(MPU2, PWR_MGMT_1, 0x0);

  // Radio Module
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();

  /*
  Serial.print("MPU1: ");
  Serial.println(getCalibrateVals(MPU1, GYRO_XOUT_H, 10000));
  Serial.print("MPU2: ");
  Serial.println(getCalibrateVals(MPU2, GYRO_XOUT_H, 10000));
  */

  GyXcal1 = 1047;
  GyXcal2 = -97;
  GyYcal1 = -319;
  GyYcal2 = -60;
  AcXcal1 = 1270;
  AcXcal2 = -617;
  AcYcal1 = 1257;
  AcYcal2 = -413;
}

void loop() {
  gyroInterface();
  
  /*
  uint8_t buff[2];
  i2c_write_read(MPU1, GYRO_XOUT_H, buff, 2);
  GyX1 = (buff[0] << 8 | buff[1]) - GyXcal1;

  i2c_write_read(MPU2, GYRO_XOUT_H, buff, 2);
  GyX2 = (buff[0] << 8 | buff[1]) - GyXcal2;

  char pbuf[30];
  sprintf(pbuf, "MPU1: %6d | MPU2: %6d", GyX1, GyX2);
  Serial.println(pbuf);
  */

  delay(100);
  
}

void gyroInterface() {
  uint8_t buff[4];
  
  // MPU 1
  i2c_write_read(MPU1, ACCEL_XOUT_H, buff, 4);
  AcX1 = (buff[0] << 8 | buff[1]) - AcXcal1;
  AcY1 = (buff[2] << 8 | buff[3]) - AcYcal1;
  i2c_write_read(MPU1, GYRO_XOUT_H, buff, 4);
  GyX1 = (buff[0] << 8 | buff[1]) - GyXcal1;
  GyY1 = (buff[2] << 8 | buff[3]) - GyYcal1;

  // MPU 2
  i2c_write_read(MPU2, ACCEL_XOUT_H, buff, 4);
  AcX2 = (buff[0] << 8 | buff[1]) - AcXcal2;
  AcY2 = (buff[2] << 8 | buff[3]) - AcYcal2;
  i2c_write_read(MPU2, GYRO_XOUT_H, buff, 4);
  GyX2 = (buff[0] << 8 | buff[1]) - GyXcal2;
  GyY2 = (buff[2] << 8 | buff[3]) - GyYcal2;

  char pbuf[60];
  sprintf(pbuf, "AcX1: %6d | AcY1: %6d | GyX1: %6d | GyY1: %6d", AcX1, AcY1, GyX1, GyX2);
  Serial.println(pbuf);

  // Combining readings together
  //int16_t comb_AcX = (AcX1 - AcX2) / 2; // subracting one from the other since the sensors are flipped 180 degrees from eachother on the board
  //int16_t comb_AcY = (AcY1 - AcY2) / 2;

  //int16_t movement = sqrt(comb_AcX * comb_AcX + comb_AcY * comb_AcY);
  //Serial.println(AcX);
  //radio.write(&movement, sizeof(movement));
  //Serial.println(movement);
}

void getAngle(int Ax, int Ay, int Az) {
  double x = Ax;
  double y = Ay;
  double z = Az;
  pitch = atan(x / sqrt((y * y) + (z * z)));
  roll = atan(y / sqrt((x * x) + (z * z)));
  pitch = pitch * (180.0 / 3.14);
  roll = roll * (180.0 / 3.14) ;
}

// Gets bias values of sensors
int getCalibrateVals(uint8_t addr, uint8_t reg, int samples) {
  int      vals = 0;
  uint8_t  buff[2];
  int16_t value;

  for(int i = 0; i < samples; i++){
    i2c_write_read(addr, reg, buff, 2);
    value = buff[0] << 8 | buff[1];
    vals += value;
  }

  return vals/samples;
}