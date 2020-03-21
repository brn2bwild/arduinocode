#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Tesalia";
const char* pass = "1123581321";

HTTPClient http;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("Conectado al WiFi");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if(WiFi.status() == WL_CONNECTED){
    float voltaje = random(0,200);
    float corriente = random(0,200);
    http.begin("http://demo.thingsboard.io/api/v1/bU67EYnNl5n8sgkn72LN/telemetry"); //cambiar el token con el de la cuenta
    http.addHeader("Content-Type","application/json");
    int respuestaHTTP = http.POST("{\"voltaje\":"+String(voltaje)+",\"corriente\":"+String(corriente)+"}");
    if(respuestaHTTP > 0){
      String respuesta = http.getString();
      Serial.print(respuestaHTTP);
      Serial.print("-");
      Serial.println(respuesta);
    }else{
      Serial.println("Error al hacer la consulta");
    }
    http.end();
  }
  delay(10000);
}
