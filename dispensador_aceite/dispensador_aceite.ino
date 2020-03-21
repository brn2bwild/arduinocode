#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

#define Cantidad_datos  6
#define pinValvula      A3
#define pinSensor       2

float flujo_inmediato;
unsigned long tiempo_ciclo = 0;
unsigned int flujo_hora;
unsigned int milimetros_actuales;
unsigned int milimetros_totales;
double flujoinmediato;
volatile int contador_flujo;

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS]={
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'},
};

char datos_servicio[Cantidad_datos];
char tecla;

byte rowPins[ROWS] = {10,9,8,7};
byte colPins[COLS] = {6,5,4,3};

byte contador_valores = 6;

float calibracion = 5.7;

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

LiquidCrystal_I2C lcd(0x3F, 16, 2);
void setup(){
    delay(1000);
    pinMode(pinValvula, OUTPUT);
    digitalWrite(pinValvula, HIGH);
    pinMode(pinSensor, INPUT_PULLUP);
    attachInterrupt(0, contarFlujo, RISING);
    lcd.backlight();
    lcd.init();
    Serial.begin(9600);
    memset(datos_servicio, ' ', sizeof(datos_servicio));
    
}

void loop(){
    lcd.setCursor(0, 0);
    lcd.print("Cant. a servir");
    lcd.setCursor(0, 1);
    lcd.print("mL:");

    tecla = keypad.getKey();

    if(tecla != NO_KEY){
        if(tecla >= 48 && tecla <= 57){
            contador_valores--;
            datos_servicio[contador_valores] = tecla;
            lcd.setCursor(contador_valores + 4, 1);
            lcd.print(datos_servicio[contador_valores]);
            if(contador_valores == 0){
                contador_valores = Cantidad_datos;
            }
        }
        if(tecla == 67){
            limpiarDatos();            
        }

        if(tecla == 65){
            
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Sirviendo");
            lcd.setCursor(0, 1);
            lcd.print("mL:");

            long cantidad_mL = atol(datos_servicio);

            lcd.setCursor(4, 1);
            for(char i = 0; i < Cantidad_datos; i++){
                lcd.print(datos_servicio[i]);
            }

            dispensarProducto(cantidad_mL);

            limpiarDatos();
        }

        if(tecla == 68){
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Limpieza");
            digitalWrite(pinValvula, LOW);
            delay(1000);
            do{
                tecla = keypad.getKey();
            }while(tecla == NO_KEY);
            digitalWrite(pinValvula, HIGH);
            limpiarDatos();
        }
    }
}

void contarFlujo(){
    contador_flujo++;
}

void dispensarProducto(long cantidad_servir){
    digitalWrite(pinValvula, LOW);
    while(cantidad_servir >= milimetros_totales){
        if((unsigned long)(millis() - tiempo_ciclo) > 1000.0){
            detachInterrupt(0);
            flujo_inmediato = ((1000.0 / (millis() - tiempo_ciclo)) * contador_flujo) / calibracion;
            tiempo_ciclo = millis();
            milimetros_actuales = (flujo_inmediato / 60) * 1000.0;
            milimetros_totales += milimetros_actuales;
            
            unsigned int fraccion;

            Serial.print("flujo actual: ");
            Serial.print(int(flujo_inmediato));
            Serial.print(" - ");
            fraccion = (flujo_inmediato - int(flujo_inmediato)) * 10;
            Serial.print(fraccion, DEC);
            Serial.print("L/min - ");
            Serial.print("milimetros actuales: ");
            Serial.print(milimetros_actuales);
            Serial.print("mL/s - ");
            Serial.print("milimetros totales: ");
            Serial.print(milimetros_totales);
            Serial.println("mL");

            contador_flujo = 0;

            attachInterrupt(0, contarFlujo, FALLING);
        }
    }
    milimetros_totales = 0;
    digitalWrite(pinValvula, HIGH);
}

void limpiarDatos(){
    memset(datos_servicio, ' ', sizeof(datos_servicio));
    contador_valores = Cantidad_datos;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cant. a servir");
    lcd.setCursor(0, 1);
    lcd.print("mL:");
}
