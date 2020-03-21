#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <LiquidCrystal.h>

float temperature;
float humidity;
float pressure;

#define ALTITUDE 216.0 // Altitude in Sparta, Greece

Adafruit_BME280 bme; // I2C

//LiquidCrystal lcd(8,9,4,5,6,7); 

void setup(void) {
  Serial.begin(115200);

  //lcd.begin(16, 2);
  //lcd.print("Reading sensor");

  bool status;
    
    // default settings
    status = bme.begin(0x77);  //The I2C address of the sensor I use is 0x76
    if (!status) {
        //lcd.clear();
        //lcd.print("Error. Check");
        //lcd.setCursor(0,1);
        //lcd.print("connections");
        Serial.println("error");
        while (1);
    }
}

void loop() {
  
 delay(2000);

 getPressure();
 getHumidity();
 getTemperature();
 
 //lcd.clear(); 
 
 //Printing Temperature
 String temperatureString = String(temperature,1);
 Serial.println(temperatureString);
 //lcd.print("T:"); 
 //lcd.print(temperatureString);
 //lcd.print((char)223);
 //lcd.print("C ");
 
 //Printing Humidity
 String humidityString = String(humidity,0); 
 //lcd.print("H: ");
 //lcd.print(humidityString);
 //lcd.print("%");
 
 //Printing Pressure
 //lcd.setCursor(0,1);
 //lcd.print("P: ");
 String pressureString = String(pressure,2);
 //lcd.print(pressureString);
 //lcd.print(" hPa");
}

float getTemperature()
{
  temperature = bme.readTemperature();
}

float getHumidity()
{
  humidity = bme.readHumidity();
}

float getPressure()
{
  pressure = bme.readPressure();
  pressure = bme.seaLevelForAltitude(ALTITUDE,pressure);
  pressure = pressure/100.0F;
}

