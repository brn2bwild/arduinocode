#define pinPotenciometro A7
#define pinLed  13
void setup() {
  pinMode(pinLed, OUTPUT);
}

void loop() {
  int valor_potenciometro = analogRead(pinPotenciometro);
  int valor_PWM = map(valor_potenciometro, 0, 1023, 0, 255);
  analogWrite(pinLed, valor_PWM);
}
