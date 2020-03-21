int arreglo[10] = {'h','o','l','a',' ','m','u','n','d','o'};
}

int resultado = 0;

void setup() { 
  Serial.begin(9600);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);

  pinMode(7, INPUT);
  pinMode(8, INPUT);
  pinMode(9, INPUT);

  for(int i = 0; i < 10 ; i++){
    resultado = resultado + arreglo[i];
  }

  Serial.println(resultado/10.0);

  for(int j = 0; j < 10; j++){
    Serial.print((char)arreglo[j]);
  }


  
}

void loop() {
  
}
