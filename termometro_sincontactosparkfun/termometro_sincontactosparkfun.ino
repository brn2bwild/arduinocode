//#include <SPI.h>
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SparkFunMLX90614.h> // SparkFunMLX90614 Arduino library

#define OLED_RESET LED_BUILTIN
Adafruit_SSD1306 display(OLED_RESET);

IRTherm therm; // Create an IRTherm object to interact with throughout

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

typedef void (*Demo)(void);
unsigned long tiempoAnterior = 0;
int demoMode = 0;
const int intervaloLectura = 2000;
float newEmissivity = 0.98;

void setup()   {                
  //Serial.begin(9600);
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.clearDisplay();
  delay(500);
  
  //Serial.println("Inicializando sensor mlx90614");
  therm.begin(); // Initialize thermal IR sensor
  therm.setUnit(TEMP_C); // Set the library's units to Farenheit
  therm.setEmissivity(newEmissivity);
  // Alternatively, TEMP_F can be replaced with TEMP_C for Celsius or
  // TEMP_K for Kelvin.
}

void temperaturaObjeto(void) {
  //leerTemperaturas();
  if(therm.read()){
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.println("   Temp:");
    display.setTextSize(3);
    display.setCursor(5,20);
    display.print(therm.object()+2.5,1);
    display.print((char)247);
    display.println("C");
    display.display();
  }
}

Demo demos[] = {temperaturaObjeto};
int demoLength = (sizeof(demos) / sizeof(Demo));
long timeSinceLastModeSwitch = 0;

void loop() {
  unsigned long ahora = millis();

  if((unsigned long)(ahora - tiempoAnterior) >= intervaloLectura){
    display.clearDisplay();  // draw the current demo method
    demos[demoMode]();
    tiempoAnterior = ahora;
  }

//  if(!digitalRead(pinBotonArriba)){
//    delay(200);
//    demoMode = (demoMode + 1) % demoLength;
//  }
}
