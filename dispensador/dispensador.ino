#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x3F,20,4);

int tiempoScroll = 350;
int anchoPantalla = 20;

int inicioCadena, finalCadena = 0;
int cursorScroll = anchoPantalla;

String linea1 = "Agua purificada";
String linea2 = "MARCA";
String linea3 = "Acepta monedas";
String linea4 = "$10.00  $5.00  $2.00  $1.00  $0.50 ";

byte char1[8] = {
  B00000,
  B00000,
  B01111,
  B00100,
  B00100,
  B00100,
  B01000,
  B10000
};

byte char2[8] = {
  B00000,
  B00000,
  B11110,
  B00100,
  B00100,
  B00100,
  B00010,
  B00001
};

byte char3[8] = {
  B00011,
  B00110,
  B01000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000
};

byte char4[8] = {
  B11000,
  B01100,
  B00010,
  B00001,
  B00001,
  B00001,
  B00001,
  B00001
};

byte char5[8]{
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000
};

byte char6[8]{
  B00001,
  B00001,
  B00001,
  B00001,
  B00001,
  B00001,
  B00001,
  B00001
};

byte char7[8]{
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B11111,
  B00000
};
byte char8[8]{
  B00001,
  B00001,
  B00001,
  B00001,
  B00001,
  B00001,
  B11111,
  B00000
};

byte char9[8] = {
  B00011,
  B00110,
  B01000,
  B10000,
  B10000,
  B11111,
  B11111,
  B11111
};

byte char10[8] = {
  B11000,
  B01100,
  B00010,
  B00001,
  B00001,
  B11111,
  B11111,
  B11111
};
byte char11[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111,
  B11111
};

byte char12[8] = {
  B00011,
  B00111,
  B01111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

byte char13[8] = {
  B11000,
  B11100,
  B11110,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};
byte char14[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

byte char15[8] = {
  B00000,
  B00000,
  B01111,
  B00100,
  B00100,
  B00111,
  B01111,
  B11111
};

byte char16[8] = {
  B00000,
  B00000,
  B11110,
  B00100,
  B00100,
  B11100,
  B11110,
  B11111
};

void mensajeBienvenida(){
  lcd.setCursor(2,0);
  lcd.print(linea1);
  lcd.setCursor(7,1);
  lcd.print(linea2);
  lcd.setCursor(3,2);
  lcd.print(linea3);
}

void animacionLlenado(){
  lcd.setCursor(6,0);
  lcd.print("Llenando...");
  lcd.createChar(0,char1);
  lcd.createChar(1,char2);
  lcd.createChar(2,char3);
  lcd.createChar(3,char4);
  lcd.createChar(4,char5);
  lcd.createChar(5,char6);
  lcd.createChar(6,char7);
  lcd.createChar(7,char8);
  lcd.setCursor(1,0);
  lcd.write(byte(0));
  lcd.write(byte(1));

  lcd.setCursor(0,1);
  lcd.write(byte(2));
  lcd.write(254);
  lcd.write(254);
  lcd.write(byte(3));

  lcd.setCursor(0,2);
  lcd.write(byte(4));
  lcd.write(254);
  lcd.write(254);
  lcd.write(byte(5));

  lcd.setCursor(0,3);
  lcd.write(byte(6));
  lcd.write(95);
  lcd.write(95);
  lcd.write(byte(7));
  delay(500);

  lcd.setCursor(0,3);
  lcd.write(255);
  lcd.write(255);
  lcd.write(255);
  lcd.write(255);
  delay(500);

  lcd.setCursor(0,2);
  lcd.write(255);
  lcd.write(255);
  lcd.write(255);
  lcd.write(255);
  delay(500);

  lcd.setCursor(0,2);
  lcd.write(255);
  lcd.write(255);
  lcd.write(255);
  lcd.write(255);
  delay(500);

  lcd.createChar(2,char9);
  lcd.createChar(3,char10);
  lcd.createChar(4,char11);
  lcd.setCursor(0,1);
  lcd.write(byte(2));
  lcd.write(byte(4));
  lcd.write(byte(4));
  lcd.write(byte(3));
  delay(500);

  lcd.createChar(2,char12);
  lcd.createChar(3,char13);
  lcd.createChar(4,char14);
  lcd.setCursor(0,1);
  lcd.write(byte(2));
  lcd.write(byte(4));
  lcd.write(byte(4));
  lcd.write(byte(3));
  delay(500);

  lcd.createChar(0,char15);
  lcd.createChar(1,char16);
  lcd.setCursor(1,0);
  lcd.write(byte(0));
  lcd.write(byte(1));
  delay(500);
}

void setup() {
  lcd.init();
  lcd.backlight();
  mensajeBienvenida();
}

void loop() {
  delay(tiempoScroll);

  lcd.setCursor(cursorScroll, 3);
  lcd.print(linea4.substring(inicioCadena,finalCadena));

  if(inicioCadena == 0 && cursorScroll > 0){
    cursorScroll--;
    finalCadena++;
  }else if(inicioCadena == finalCadena){
    inicioCadena = finalCadena = 0;
    cursorScroll = anchoPantalla;
  }else if(finalCadena == linea4.length() && cursorScroll == 0){
    inicioCadena++;
  }else{
    inicioCadena++;
    finalCadena++;
  }
}
