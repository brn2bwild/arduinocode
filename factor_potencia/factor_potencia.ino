#include <EmonLib.h>

EnergyMonitor emon;

void setup() {
  Serial.begin(9600);
  emon.voltage(0, 243.26, 1.7);
  emon.current(1, 111.1);  
}

void loop() {
  emon.calcVI(20, 2000);
  Serial.println(emon.powerFactor);
}
