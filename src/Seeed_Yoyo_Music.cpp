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

// MPU5060 gyroscope module
const int MPU1 = 0x68;    
const int MPU2 = 0x69;
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;
int AcXcal, AcYcal, AcZcal, GyXcal, GyYcal, GyZcal, tcal;
double t, tx, tf, pitch, roll;

// 2.4GHz radio module
RF24 radio(6, 7); // CE, CSN
const byte address[6] = "00001";

void setup() {
  // Gyro modules
  Wire.begin();
  Wire.beginTransmission(MPU1);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  Wire.beginTransmission(MPU2);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  // Radio Module
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
  
  Serial.begin(115200);
}
void loop() {
  gyroInterface();

}

void gyroInterface() {
  // MPU 1
  Wire.beginTransmission(MPU1);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU1, 4, true);
  AcXcal = -950;
  AcYcal = -300;
  AcZcal = 0;
  tcal = -1600;
  GyXcal = 480;
  GyYcal = 170;
  GyZcal = 210;
  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  int16_t AcX1 = AcX + AcXcal;
  int16_t AcY1 = AcY + AcYcal;
  //Serial.println(AcX);
  //radio.write(&AcX1, sizeof(AcX1));


  // MPU 2
  Wire.beginTransmission(MPU2);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU2, 4, true);
  AcXcal = -950;
  AcYcal = -300;
  AcZcal = 0;
  tcal = -1600;
  GyXcal = 480;
  GyYcal = 170;
  GyZcal = 210;
  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();

  int16_t AcX2 = AcX + AcXcal;
  int16_t AcY2 = AcY + AcYcal;
  int16_t comb_AcX = (AcX1 - AcX2) / 2; // subracting one from the other since the sensors are flipped 180 degrees from eachother on the board
  int16_t comb_AcY = (AcY1 - AcY2) / 2;
  int16_t movement = sqrt(comb_AcX * comb_AcX + comb_AcY * comb_AcY);
  //Serial.println(AcX);
  radio.write(&movement, sizeof(movement));
  Serial.println(movement);
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