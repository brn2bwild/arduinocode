void setup() {
  Serial.begin(9600);
  pinMode(A0, INPUT);
  analogReference(INTERNAL);
}

void loop() {
  int valor[20];
  int sumatoria = 0;
  int aux;
  
  for(int i = 0; i < 20; i++){
    valor[i] = analogRead(A4);
    delay(5);
  }

  for(int i = 0; i < 20; i++){
    if(valor[i+1] > valor[i]){
      aux = valor[i];
      valor[i] = valor[i+1];
      valor[i+1] = aux; 
    }
  }

  for(int j = 2; j < 18; j++) sumatoria += valor[j];

  //float temperatura = -0.6377 * (sumatoria / 16) + 372.31;
  
  Serial.println(sumatoria/16);
  delay(1000);
}
