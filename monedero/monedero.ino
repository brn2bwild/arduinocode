int monederoPin = 2;
int sensorPin = 3;
int botoninicioPin = 7;
int botonaguaPin = 8;
int botoncloroPin = 9;
int ledPin = 13;

float total;
unsigned long tiempoAnterior;
volatile unsigned long tiempoUltpulso = 0;
int contadorMonedero;
float descontar;
float restante;

float calibracion = 4.5;
float flujoInmediato;
unsigned int milimetros;
unsigned int milimetrosServidos;
int contadorFlujo;

int seleccion;
unsigned int milimetrosaServir;
int milimetrosRestantes;

bool despacho, pausa, encendido;

void setup() {
  pinMode(monederoPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(botoninicioPin, INPUT_PULLUP);
  pinMode(botonaguaPin, INPUT_PULLUP);
  pinMode(botoncloroPin, INPUT_PULLUP);
  Serial.begin(9600);

  
  tiempoAnterior = 0;
  tiempoUltpulso = 0;
  total = 0.0;
  contadorMonedero = 0;
  flujoInmediato = 0.0;
  contadorFlujo = 0;
  milimetros = 0;
  milimetrosServidos = 0;
  milimetrosRestantes = 0;
  tiempoAnterior = 0;
  seleccion = 0;
  milimetrosaServir = 0;
  despacho = 0;
  pausa = 0;

  digitalWrite(ledPin, LOW);

  attachInterrupt(0, contadorMonedas, RISING);
  attachInterrupt(1, contadorSensor, RISING);
}

void contadorMonedas(){
  contadorMonedero++;
  tiempoUltpulso = millis();
}

void contadorSensor(){
  contadorFlujo++;
}

void loop() {
	milimetrosRestantes = milimetrosaServir - milimetrosServidos;

	if(!digitalRead(botoninicioPin) && despacho == 1 && pausa == 0){
		digitalWrite(ledPin, LOW);
		delay(100);
		encendido = 0;
		pausa = 1;
	}else if(!digitalRead(botoninicioPin) && despacho == 1 && pausa == 1){
		digitalWrite(ledPin, HIGH);
		delay(100);
		encendido = 1;
		pausa = 0;
	}

	digitalWrite(ledPin, encendido);

  if(milimetrosRestantes > 0 && despacho == 1 && pausa == 0){
  	encendido = 1;
  }else if(milimetrosRestantes <= 0 && despacho == 1){
    encendido = 0;
    milimetrosRestantes = 0;
    total = 0.0;
    despacho = 0;
    restante = 0.0;
    descontar = 0.0;
    milimetrosServidos = 0;
    seleccion = 0;
  }

  if(!digitalRead(botoncloroPin) && despacho == 0){
    seleccion = 1;
    milimetrosaServir = (total / 10) * 1000;
    despacho = 1;
    delay(100);
  }else if(!digitalRead(botonaguaPin) && despacho == 0){
    seleccion = 2;
    milimetrosaServir = (total / 5) * 1000;
    despacho = 1;
    delay(100);
  }



  if(seleccion == 1){
  	descontar = (float(milimetrosServidos) / 1000) * 10;
  	restante = total - descontar;
  } else if(seleccion == 2){
  	descontar = (float(milimetrosServidos) / 1000) * 5;
  	restante = total - descontar;
  }

  if(contadorMonedero > 0 && millis() - tiempoUltpulso > 200){
    switch(contadorMonedero){
      case 1:
        total += 0.5;
        contadorMonedero = 0;
        break;
      case 2:
        total += 1.0;
        contadorMonedero = 0;
        break;
      case 4:
        total += 2.0;
        contadorMonedero = 0;
        break;
      case 5:
        total += 5.0;
        contadorMonedero = 0;
        break;
      case 6:
        total += 10.0;
        contadorMonedero = 0;
        break;
      default:
        contadorMonedero = 0;
        break;
    }

    //Serial.print("pulsos: ");
    //Serial.print(contadorMonedero);
    //Serial.print("Total: ");
    //Serial.println(total);
  }

  if((millis() - tiempoAnterior) > 1000){
    detachInterrupt(1);

    flujoInmediato = ((1000.0 / (millis() - tiempoAnterior)) * contadorFlujo) / calibracion;

    tiempoAnterior = millis();

    milimetros = (flujoInmediato / 60) * 1000;

    milimetrosServidos += milimetros;

    //unsigned int frac;

    //Serial.print("Flujo: ");
    //Serial.print(int(flujoInmediato));
    //Serial.print(".");
    //frac = (flujoInmediato - int(flujoInmediato)) * 10;
    //Serial.print(frac, DEC);
    //Serial.print("L/min");
    //Serial.print(" Milimetros: ");
    //Serial.print(milimetros);
    //Serial.print("mL/Sec");

  	Serial.print("Dinero pagado: $");
  	Serial.print(total,2);
  	Serial.print(", Dinero restante: $");
  	Serial.print(restante,2);
  	Serial.print(", Producto: ");
  	if(seleccion == 1){
  		Serial.print("Cloro");
  	}else if(seleccion == 2){
  		Serial.print("Suavizante");
  	}
    Serial.print(", L. Servido: ");
    Serial.print(milimetrosServidos);
    Serial.print("mL");
    Serial.print(", L. Faltante: ");
    Serial.print(milimetrosRestantes);
    Serial.print(", Descuento: ");
    Serial.println(descontar);

    contadorFlujo = 0;

    attachInterrupt(1, contadorSensor, RISING);
  }
}
