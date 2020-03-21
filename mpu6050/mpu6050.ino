#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"

const int mpuAddress = 0x68;
MPU6050 mpu(mpuAddress);

int ax, ay, az;
int gx, gy, gz;

//const float accScale = 2.0 * 9.81 / 32768.0;
//const float gyroScale = 250.0 / 32768.0;

//void printInfo(){
//	Serial.print("m/s2; deg/s");
//	Serial.print(" ");
//	Serial.print(ax*accScale);
//	Serial.print(" ");
//	Serial.print(ay*accScale);
//	Serial.print(" ");
//	Serial.print(az*accScale);
//	Serial.print(" ");
//	Serial.print(gx*gyroScale);
//	Serial.print(" ");
//	Serial.print(gy*gyroScale);
//	Serial.print(" ");
//	Serial.println(gz*gyroScale);	
//}

void setup() {
	Serial.begin(9600);
	Wire.begin();
	mpu.initialize();
	Serial.println(mpu.testConnection() ? F("IMU iniciada correctamente") : F("Error al iniciar IMU"));
}

void loop() {
  mpu.getAcceleration(&ax, &ay, &az);
  double x = ax;
  double y = ay;
  double z = az;
  //mpu.getRotation(&gx, &gy, &gz);

  //float accelAngx = atan(ax /sqrt(pow(ay, 2) + pow(az,2)))*(180.0 / 3.14);
  //float accelAngy = atan(ay /sqrt(pow(ax, 2) + pow(az,2)))*(180.0 / 3.14);

  float roll = atan2(-x, z)*(180.0 / 3.14);
  float pitch = atan2(y, sqrt(x*x + z*z))*(180.0 / 3.14);

  Serial.print(F("Roll:"));
  Serial.print(roll);
  Serial.print(F(" Pitch:"));
  Serial.println(pitch);

  //printInfo();

  delay(100);
}
