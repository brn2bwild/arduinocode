#define pinValvula      A3

int sensorPin = 2;

float flujoInmediato;
unsigned int milimetros;
unsigned int milimetrosTotales;

float calibracion = 5.8;

volatile byte contador;

unsigned long tiempoAnterior;

void setup() {
  pinMode(sensorPin, INPUT);
  pinMode(pinValvula, OUTPUT);

  Serial.begin(9600);

  contador          = 0;
  flujoInmediato    = 0.0;
  milimetros        = 0;
  milimetrosTotales = 0;
  tiempoAnterior    = 0;

  attachInterrupt(0, contadorPulsos, RISING);
}

void loop() {
  if((millis() - tiempoAnterior) > 1000){

    detachInterrupt(0);

    flujoInmediato = ((1000.0 / (millis() - tiempoAnterior)) * contador) / calibracion;

    tiempoAnterior = millis();

    milimetros = (flujoInmediato / 60) * 1000;

    milimetrosTotales += milimetros;

    unsigned int frac;

    Serial.print("Flujo actual: ");
    Serial.print(int(flujoInmediato));
    Serial.print(".");
    frac = (flujoInmediato - int(flujoInmediato)) * 10;
    Serial.print(frac, DEC);
    Serial.print("L/min");
    Serial.print(" Mililitros totales: ");
    Serial.print(milimetros);
    Serial.print("mL/Sec");
    Serial.print(" Liquido total: ");
    Serial.print(milimetrosTotales);
    Serial.println("mL");

    contador = 0;

    attachInterrupt(0, contadorPulsos, FALLING);
  }
}

void contadorPulsos(){
  contador++;
}
