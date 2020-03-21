const int potPin = A0;
const int salPin = 12;
const int ledPin = 13;

int valorPot = 0;
int salValor = 0;

void setup() {
  Serial.begin(9600);
  pinMode(salPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  valorPot = analogRead(potPin);
  salValor = map(valorPot, 0, 1023, 0, 1500);
  digitalWrite(salPin, HIGH);
  digitalWrite(ledPin, HIGH);
  delayMicroseconds(salValor);
  digitalWrite(salPin, LOW);
  digitalWrite(ledPin, LOW);
  delayMicroseconds(60000 - salValor);
  Serial.println(salValor);
  delay(200);
}
