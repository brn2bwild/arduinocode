float valor_calibracion = 0.945;
void setup(){
	pinMode(2, OUTPUT);
	Serial.begin(115200);
	Serial.setTimeout(2000);

	while(!Serial);

	Serial.print("\n\nVoltaje ADC: ");
    float voltaje = (((analogRead(0)*3.3)/1024)*valor_calibracion);
    Serial.println(voltaje);
    Serial.print("Porcentaje de batería: ");
    float porcentaje = (voltaje*100.0)/2.31;
    Serial.println(porcentaje);

	digitalWrite(2, LOW);
	delay(1000);
	digitalWrite(2, HIGH);
	ESP.deepSleep(5e6);
}

void loop(){
}