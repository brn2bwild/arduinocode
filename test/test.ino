int led = 13;
//unsigned int contador = 65500;

void setup() {
  Serial.begin(9600);
  pinMode(led, OUTPUT);
}

void parpadear(){
  digitalWrite(led, 1);
  delay(100);
  digitalWrite(led, 0);
  delay(100);
}

int direccion = 0;
int velocidad = 0;

void loop() {
  for(direccion = 0; direccion < 19; direccion++){
    velocidad = direccion;
    Serial.print(direccion); 
    Serial.print(",");
    Serial.println(velocidad);
    delay(500);
  }
  for(direccion = 20; direccion > 0; direccion--){
    velocidad = direccion;
    Serial.print(direccion); 
    Serial.print(",");
    Serial.println(velocidad);
    delay(500);
  }
}
