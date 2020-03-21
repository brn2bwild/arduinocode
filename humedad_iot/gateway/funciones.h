#ifndef FUNCIONES_H
#define FUNCIONES_H


void StringToEEPROM(char arreglo[], int longitud, int dir_inicio){
    // for(int i = 0; i < array_size + 1; i++) EEPROM.write(dir_inicio + i, arreglo[i]);
    for(int i = 0; i < longitud; i++) EEPROM.write(dir_inicio + i, 0);
    delay(100);
    for(int j = 0; j < longitud; j++) EEPROM.write(dir_inicio + j, arreglo[j]);
    EEPROM.commit();
}

void EEPROMToString(char arreglo[], int longitud, int dir_inicio){
  char rChar;
  memset(arreglo, 0, longitud);

  // for(int i = 0; i < array_size + 1; i++){
  for(int i = 0; i < longitud; i++){
    rChar = EEPROM.read(dir_inicio + i);
    if(rChar == 0) break;
    arreglo[i] = rChar;
  }
}

void IntToEEPROM(unsigned int valor, int dir_inicio){
  byte byteL = (valor & 0xFF);
  byte byteH = ((valor >> 8) & 0xFF);

  EEPROM.write(dir_inicio, byteL);
  EEPROM.write(dir_inicio + 1, byteH);
  EEPROM.commit();
}

void EEPROMToInt(unsigned int destino, int dir_inicio){
  destino = (((EEPROM.read(dir_inicio) << 0 ) & 0xFF) + ((EEPROM.read(dir_inicio + 1) << 8 ) & 0xFFFF));
}

void obtenerMAC(char arreglo[], int array_size){
    uint8_t macWiFi[6];
    WiFi.macAddress(macWiFi);
    snprintf(arreglo, array_size, "%02X:%02X:%02X:%02X:%02X:%02X", macWiFi[0], macWiFi[1], macWiFi[2], macWiFi[3], macWiFi[4], macWiFi[5]);
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

void urldecode(String input, char arreglo[], byte lon_arreglo){ // (based on https://code.google.com/p/avr-netino/)
  memset(arreglo, 0, lon_arreglo);
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