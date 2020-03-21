#include <RH_ASK.h>
#include <SPI.h>
#include <Filters.h>
#include <EmonLib.h>

#define pinLed      13
#define pinSenAmp   A0
#define pinSenVol   A1
#define intervaloRetransmision  500  
#define intervaloVerificacion   10000

RH_ASK driver;
EnergyMonitor emon;

bool msj_matriz_recibido = false;
bool msj_dispositivo_recibido = false;
bool msj_retransmitido = false;

unsigned long tiempo_anterior_retransmision = 0;
unsigned long tiempo_anterior_verificacion = 0;

// char buffer_estado[28] = {"D;001;0:000.0;0:000.0;0:0.00"};

void setup(){
    Serial.begin(9600);
    if(!driver.init())
        Serial.println("Inicialización fallida");
    emon.voltage(pinSenVol, 234.46, 1.1);
    emon.current(pinSenAmp, 99.1);
}

void loop(){
    uint8_t buffer[3];
    uint8_t buffer_len = sizeof(buffer);

    if(driver.recv(buffer, &buffer_len)){
        int i;
        Serial.print("mensaje: ");
        Serial.println((char*)buffer);
        if(buffer[0] == 'M'){
            msj_matriz_recibido = true;
        }else if(buffer[0] == 'D'){
            msj_dispositivo_recibido = true;
        }

        if(char(buffer[2]) == '1'){
            digitalWrite(pinLed, HIGH);
        }else if(char(buffer[2]) == '0'){
            digitalWrite(pinLed, LOW);
        } 
    }

    if(msj_matriz_recibido){
        Serial.println("mensaje recibido de la matriz");
        buffer[0] = 'D';
        driver.send(buffer, sizeof(buffer));
        driver.waitPacketSent();
        Serial.println("Dato enviado");
        msj_retransmitido = false;
        msj_matriz_recibido = false;
    }

    if(msj_dispositivo_recibido && !msj_retransmitido){
        Serial.println("mensaje recibido de otro dispositivo");
        driver.send(buffer, sizeof(buffer));
        driver.waitPacketSent();
        Serial.println("Dato enviado");
        msj_retransmitido = true;
        msj_matriz_recibido = false;
    }   

    unsigned long ahora = millis();

    if((unsigned long) ahora - tiempo_anterior_retransmision >= intervaloRetransmision){
        tiempo_anterior_retransmision = millis();
        msj_retransmitido = false;
    }

    if((unsigned long) ahora - tiempo_anterior_verificacion >= intervaloVerificacion){
        tiempo_anterior_verificacion = millis();
        
        char buffer_estado[] = "D;001;-;-;-";
        emon.calcVI(20, 2000);

        float vrms = emon.Vrms;
        (vrms < 100.0) ? buffer_estado[6] = '1' : buffer_estado[6] = '0';

        float prms = emon.realPower;
        (prms < 5.0) ? buffer_estado[8] = '1' : buffer_estado[8] = '0';
    
        float fp = emon.powerFactor;
        (vrms < 0.75) ? buffer_estado[10] = '1' : buffer_estado[10] = '0';

        Serial.print(vrms);Serial.print("\t");Serial.print(prms);Serial.print("\t");Serial.println(fp);
        Serial.println((char*)buffer_estado);

        driver.send((uint8_t *)buffer_estado, strlen(buffer_estado));
        driver.waitPacketSent();
        Serial.println("Mensaje de estado enviado");
    }
}

float medirFP(){
    emon.calcVI(20, 2000);
    return emon.powerFactor;
}

float medirVoltaje(){
    emon.calcVI(20, 2000);
    return emon.Vrms;
    // float frecuencia = 60;                     // test signal frequency (Hz)
    // float ancho_ventana = 40.0/frecuencia;     // how long to average the signal, for statistist

    // float interseccion = -0.04; // to be adjusted based on calibration testing
    // float pendiente = 0.0405; // to be adjusted based on calibration testing
    // float voltaje_medido; // Voltage

    // int valorSensor = 0;

    // unsigned long intervaloPeriodo = 1000; //Refresh rate
    // unsigned long tiempo_anterior_voltaje = 0;
    
    // byte mediciones = 0;

    // RunningStatistics inputStats;
    // inputStats.setWindowSecs(ancho_ventana);

    // while(mediciones < 5){
    //     valorSensor = analogRead(pinSenVol);
    //     inputStats.input(valorSensor);

    //     if((unsigned long)(millis() - tiempo_anterior_voltaje) >= intervaloPeriodo){
    //         tiempo_anterior_voltaje = millis();
    //         voltaje_medido = interseccion + pendiente * inputStats.sigma();
    //         voltaje_medido = voltaje_medido * (67.8036);
    //         mediciones++;
    //     }
    // }
    // return(voltaje_medido);
}

float medirCorriente(){
    emon.calcVI(20, 2000);
    return emon.Irms;
}

// float medirCorriente(){
//     double voltaje = 0, vrms = 0, ampsrms = 0, watts = 0;
//     const int mVAmp = 66;

//     voltaje = obtenerVPP();

//     vrms = (voltaje/2.0) * 0.707;
//     ampsrms = (vrms * 1000) / mVAmp;
//     if(ampsrms < 0 )
//         ampsrms = 0;
//     watts = ampsrms * 127.0;

//     return(watts);
// }

// float obtenerVPP(){
//      float resultado;

//      int valorLeido;
//      int valorMax = 0;
//      int valorMin = 1024;

//      unsigned long tiempo_inicio = millis();

//      while((unsigned long)millis() - tiempo_inicio < 1000){
//          valorLeido = analogRead(pinSenAmp);
//          if(valorLeido > valorMax){
//              valorMax = valorLeido;
//          }
//          if(valorLeido < valorMin){
//              valorMin = valorLeido;
//          }
//      }

//      resultado = ((valorMax - valorMin) * 5.0) / 1024.0;

//      return (resultado-0.01);
// }