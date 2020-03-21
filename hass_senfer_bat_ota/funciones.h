#ifndef FUNCIONES_H
#define FUNCIONES_H

void StringToEEPROM(char arreglo[], int array_size, int dir_inicio){
    for(int i = 0; i < array_size + 1; i++) EEPROM.write(dir_inicio + i, arreglo[i]);
}

void EEPROMToString(char arreglo[], int array_size, int dir_inicio){
  char rChar;
  memset(arreglo, 0, array_size);

  for(int i = 0; i < array_size + 1; i++){
    rChar = EEPROM.read(dir_inicio + i);
    if(rChar == 0) break;
    arreglo[i] = EEPROM.read(dir_inicio + i);
  }
   
    // String retString = "";

    // while(1){
    //     rChar = EEPROM.read(dir_inicio + contador);
    //     if(rChar == 0) break;
    //     if(contador > 30) break;
    //     contador++;

    //     retString.concat(rChar);
    // }

    // return retString;
}

void IntToEEPROM(unsigned int valor, int dir_inicio){
  byte byteL = (valor & 0xFF);
  byte byteH = ((valor >> 8) & 0xFF);

  EEPROM.write(dir_inicio, byteL);
  EEPROM.write(dir_inicio + 1, byteH);
}

void EEPROMToInt(unsigned int destino, int dir_inicio){
  destino = (((EEPROM.read(dir_inicio) << 0 ) & 0xFF) + ((EEPROM.read(dir_inicio + 1) << 8 ) & 0xFFFF));
}

void obtenerMAC(char arreglo[], int array_size){
    uint8_t macWiFi[6];
    WiFi.macAddress(macWiFi);
    snprintf(arreglo, array_size, "%02X:%02X:%02X:%02X:%02X:%02X", macWiFi[0], macWiFi[1], macWiFi[2], macWiFi[3], macWiFi[4], macWiFi[5]);
}

int obtenerVBAT(){
    int valorADC[10];
    int aux = 0;
    int sumatoria = 0;

    for (byte i = 0; i < 10; i++){
      valorADC[i] = analogRead(A0);
      delay(5);
    }

    for(byte j = 0; j < 9; j++){
      if(valorADC[j+1] > valorADC[j]){
        aux = valorADC[j];

        valorADC[j] = valorADC[j+1];
        valorADC[j+1] = aux;
      }
    }

    for(byte k = 2; k < 8; k++) sumatoria += valorADC[k];

    // return (int)((((0.0055 * (sumatoria / 6.0)) - 0.0276) / 4.3) * 100);
    return (int)((((0.002 * (sumatoria / 6.0)) + 2.6902) / 4.3) * 100);
}

unsigned char h2int(char c)
{
  if (c >= '0' && c <= '9') {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}

void urldecode(String input, char arreglo[]){ // (based on https://code.google.com/p/avr-netino/)
  char c;

  for (byte t = 0; t < input.length(); t++){
    c = input[t];
    if (c == '+') c = ' ';
    if (c == '%') {

      t++;
      c = input[t];
      t++;
      c = (h2int(c) << 4) | h2int(input[t]);
    }

    arreglo[t] = c;
  }
}

boolean verificarRango(String valor){
  if (valor.toInt() < 0 || valor.toInt() > 255) {
    return false;
  }else{
    return true;
  }
}

#endif