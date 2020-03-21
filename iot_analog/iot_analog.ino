#define pinSensor	0
#define pinLed		2
#define Offset		0.00

unsigned long int valorTotal;

void setup(){
	pinMode(pinLed, OUTPUT);
	Serial.begin(9600);
	Serial.println("Listo");
}

void loop(){
	int buf[10];
	for(int i=0; i<10; i++){
	    buf[i] = analogRead(pinSensor);
	    delay(10);
	}

	for(int i=0; i<9; i++){
	    for(int j=i+1; j<10; j++){
	        if(buf[i]>buf[j]){
	        	int temp = buf[i];
	        	buf[i] = buf[j];
	        	buf[j] = temp;
	        }
	    }
	}

	valorTotal = 0;

	for(int i=2; i<8; i++){
	    valorTotal += buf[i];
	}

	float valorPh = (float)valorTotal*3.3/1024/6;

	valorPh = 3.5*valorPh+Offset;

	Serial.print("Valor del Ph: ");
	Serial.println(valorPh);
	digitalWrite(pinLed, HIGH);
	delay(800);
	digitalWrite(pinLed, LOW);
}