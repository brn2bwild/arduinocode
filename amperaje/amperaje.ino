#define pinSensor A0

int mVAmp = 66;

double voltaje = 0, vrms = 0, ampsrms = 0, watts = 0;

void setup(){
    Serial.begin(9600);
}

void loop(){
    voltaje = obtenerVPP();

    vrms = (voltaje/2.0) * 0.707;
    ampsrms = (vrms * 1000) / mVAmp;
    if(ampsrms < 0 )
        ampsrms = 0;
    watts = ampsrms * 127.0;
    Serial.print(ampsrms,1);
    Serial.print("Arms - ");
    Serial.print(watts,0);
    Serial.println("W"); 
}

float obtenerVPP(){
     float resultado;

     int valorLeido;
     int valorMax = 0;
     int valorMin = 1024;

     uint32_t tiempo_inicio = millis();

     while((millis() - tiempo_inicio) < 1000){
         valorLeido = analogRead(pinSensor);
         if(valorLeido > valorMax){
             valorMax = valorLeido;
         }
         if(valorLeido < valorMin){
             valorMin = valorLeido;
         }
     }

     resultado = ((valorMax - valorMin) * 5.0) / 1024.0;

     return (resultado-0.01);
}