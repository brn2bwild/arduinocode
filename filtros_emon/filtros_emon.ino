#include <Wire.h>
#include <Adafruit_ADS1015.h>

Adafruit_ADS1115 ads;

int sample = 0;
int last_sample = 0;
double a = 0;

long shifted_filter = -10000;

const float multiplicador = 0.125F;
uint16_t muestraV;
double sumV, offsetV, vFiltrado, sqV, Vrms;

void setup(){
  Serial.begin(9600);
  ads.setGain(GAIN_ONE);
  ads.begin();
}

void loop(){
  double voltaje = calcvoltaje(30);
  Serial.println(voltaje);
  delay(1000);
}

double calcvoltaje(unsigned int muestras){

  for(int i = 0; i < muestras; i++){
    muestraV = ads.readADC_Differential_0_1();
    offsetV = offsetV + (muestraV - offsetV) / 1024;
    vFiltrado = muestraV - offsetV;

    sqV = vFiltrado * vFiltrado;
    sumV += sqV;
  }

  Vrms = sqrt(sumV/muestras)*multiplicador;

  sumV = 0;

  return Vrms;
}
// void loop()
// {
//   // Generate a test signal
//   last_sample = sample;
//   a+=0.1; sample = 36768 + sin(a) * 1000;

//   long shiftedFCL = shifted_filter + (long)((sample - last_sample)<<8);
//   shifted_filter = shiftedFCL - (shiftedFCL>>8);
//   long filtered_value = (shifted_filter+128)>>8;

//   Serial.print(sample);
//   Serial.print(' ');
//   Serial.println(filtered_value);
//   delay(1);
// }
