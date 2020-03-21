#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <FS.h>

#define pinLedEstado    2 //pines a utilizarse con el sonoff
#define pinBoton        5  //pines a utilizarse con el sonoff
#define pinLedTCP       0 // pines a utilizarse con el nodemcu
#define pinLedHTTP      4 // pines a utilizarse con el nodemcu

#define intervaloVerificacionServidor 15000
#define intervaloParpadeoNowifi       200
#define intervaloParpadeoNoserver1    500
#define intervaloParpadeoNoserver2    1000

#define serverControlador 0
#define serverSistema     1

unsigned long tiempoverficacionServidor = 0;
unsigned long tiempoParpadeoAnterior = 0;

uint8_t MAC_AP[WL_MAC_ADDR_LENGTH];
char url_destino[120], servidor_sistema[120];
int puerto_controlador, puerto_sistema;
bool estadoWiFi;

String strServer1, strServer2;

IPAddress ipEstatica(192,168,4,1);
IPAddress ipMascara(255,255,255,0);

char pagina_web[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html>
<head>
<style>
  html,body{
    margin: 0 auto;
    font-family: sans-serif;
    box-sizing: border-box;
    text-align: center;
  }
  div{
    display: inline-block;
    width: 80%;
    text-align: center;
  }
  label{
    width: 100%;
    float: left;
    text-align: left;
    font-size: 15px;
  }
  .input{
    width: 100%;
    font-size: 20px;
    height: 25px;
    border: 1.5px solid #000;
    border-radius: 5px;
    text-align: center;
    margin: 0 0 0.5% 0;
  }
  .boton{
    font-weight: 600;
    font-size: 1.4rem;
    height: 55px;
    width: 50%;
    cursor: pointer;
    border-radius: 5px;
    margin: 1% 0 0 0;
  }
</style>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<meta charset='utf-8'>  
<title>Repetidor</title>
</head>
<body>
  <button class="boton" onclick="toggle()" id="toggle">Credenciales</button>
  <div id="credenciales" style="display: none;">
    <h1>Repetidor</h1>
    <h2>Introduzca las credenciales</h2>
    <label>RED WIFI:</label>
    <input class='input' id='ssid' maxlength="50">
    <label>PASSWORD WIFI:</label>
    <input type="password" class='input' id='pass' maxlength="50">
    <label>SERVER TCP:</label>
    <input class='input' id='servertcp' maxlength="120">
    <label>PUERTO TCP:</label>
    <input class="input" id="puertotcp" maxlength="10">
    <label>SERVER DESTINO:</label>
    <input class='input' id='serverdest' maxlength="120">
    <label>PUERTO DESTINO:</label>
    <input class="input" id="puertodest" maxlength="10">
    <label>URL DESTINO:</label>
    <input class='input' id='urldestino' maxlength="120">
    <button class='boton' onclick="guardar()">GUARDAR</button>
  </div>
  <div id="datos" style="margin-top: 3%;">
    <h1>Servidor TCP:</h1><h1 id="lservertcp">Desconectado</h1>
    <h1>Puerto TCP:</h1><h1 id="lpuertotcp"></h1>
    <h1>Servidor Destino:</h1><h1 id="lserverdest">Desconectado</h1>
    <h1>Puerto Destino:</h1><h1 id="lpuertodest"></h1>
    <button class='boton' onclick="reset()" style="margin-top: 3%;">RESET</button>
  </div>
</body>
</form>
<script type="text/javascript">
  let timerId = setTimeout(function tick() {
    estado_servers();
    timerId = setTimeout(tick, 5000); // (*)
  }, 5000);

  function estado_servers(){
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function(){
      if(this.readyState == 4 && this.status == 200){
        if(xhr.responseText != null){
          json = JSON.parse(xhr.responseText);
          document.getElementById('lservertcp').innerHTML = json.estadotcp;
          document.getElementById('lpuertotcp').innerHTML = json.puertotcp;
          document.getElementById('lserverdest').innerHTML = json.estadodest;
          document.getElementById('lpuertodest').innerHTML = json.puertodest;
        }
      }
    }
    xhr.open("POST", "/estado_servers", true);
    xhr.send();
  }

  function toggle(){
    var x = document.getElementById('credenciales');
    var y = document.getElementById('datos');
    if(x.style.display == "none"){
      x.style.display = "inline-block";
      y.style.display = "none";
      document.getElementById('toggle').innerHTML = "Estado";
    }else{
      x.style.display = "none";
      y.style.display = "inline-block";
      document.getElementById('toggle').innerHTML = "Credenciales";
    }
  }

  function guardar(){
    var ssid = document.getElementById('ssid').value;
    var pass = document.getElementById('pass').value;
    var servertcp = document.getElementById('servertcp').value;
    var puertotcp = document.getElementById('puertotcp').value;
    var serverdest = document.getElementById('serverdest').value;
    var puertodest = document.getElementById('puertodest').value;
    var urldestino = document.getElementById('urldestino').value;
      
    var data = {ssid:ssid, pass:pass, servertcp:servertcp, puertotcp:puertotcp, serverdest:serverdest, puertodest:puertodest, urldestino:urldestino};

    var xhr = new XMLHttpRequest();

    xhr.onreadystatechange = function(){
      if(this.readyState == 4 && this.status == 200){
        if(xhr.responseText != null){
          console.log(xhr.responseText);
        }
      }
    }

    xhr.open("POST", "/config", true);
    xhr.send(JSON.stringify(data));
    alert("Datos cargados, reiniciar dipositivo en 3s");
  };

  function reset(){
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function(){
      if(this.readyState == 4 && this.status == 200){
        if(xhr.responseText != null){
          console.log(xhr.responseText);
        }
      }
    }

    xhr.open("POST", "/reset", true);
    xhr.send();
  }
</script>
</html>
)=====";

WiFiClient clientControlador;
WiFiClient clientSistema;
ESP8266WebServer server;

void conectar_AP(){
  WiFi.mode(WIFI_AP);
  WiFi.softAPmacAddress(MAC_AP);

  String macDisp = String(MAC_AP[WL_MAC_ADDR_LENGTH - 2], HEX) + String(MAC_AP[WL_MAC_ADDR_LENGTH - 1], HEX);
  macDisp.toUpperCase();

  String nombreAP = "Repetidor-" + macDisp;

  Serial.println(nombreAP);

  char nombreAPChar[nombreAP.length() + 1];

  memset(nombreAPChar, 0, nombreAP.length() + 1);

  for(int i = 0; i < nombreAP.length(); i++){
    nombreAPChar[i] = nombreAP.charAt(i);
  }

  const char* passAPchar = "012345678";

  WiFi.softAPConfig(ipEstatica, ipEstatica, ipMascara);
  WiFi.softAP(nombreAPChar, passAPchar);
  digitalWrite(pinLedEstado, LOW);
}

bool conectar_WiFi(){
  WiFi.softAPdisconnect(true);
  WiFi.disconnect();
  delay(300);

  if(SPIFFS.exists("/config.json")){
    const char *_ssid = "", *_pass = "";
    File configFile = SPIFFS.open("/config.json","r");
    if(configFile){
      size_t size = configFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      configFile.close();

      DynamicJsonBuffer jsonBuffer;
      JsonObject &jObject = jsonBuffer.parseObject(buf.get());
      if(jObject.success()){
        _ssid = jObject["ssid"];
        _pass = jObject["pass"];
      
        WiFi.mode(WIFI_STA);
        WiFi.begin(_ssid, _pass);

        Serial.print("\nIntentando conectar a red WiFi: ");
        Serial.println(_ssid);
        unsigned long tiempoInicio = millis();
        while(WiFi.status() != WL_CONNECTED){
          delay(500);
          Serial.print(".");
          digitalWrite(pinLedEstado, !digitalRead(pinLedEstado));
          if((unsigned long)(millis() - tiempoInicio >= 10000)) break;
        }
      }
    }
  }

  if(WiFi.status() == WL_CONNECTED){
    return true;
  }else{
    return false;
  }
}

bool conectar_Servidor(int servidor){
  if(SPIFFS.exists("/config.json")){
    const char *_server = "", *_urldestino = "";
    int _puerto;
    File configFile = SPIFFS.open("/config.json","r");
    if(configFile){

      size_t size = configFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      configFile.close();

      DynamicJsonBuffer jsonBuffer;
      JsonObject &jObject = jsonBuffer.parseObject(buf.get());
      if(jObject.success()){

        if(servidor == serverControlador){
          _server = jObject["servertcp"];
          _puerto = int(jObject["puertotcp"]);

          Serial.print("Servidor TCP: ");
          Serial.println(_server);
          Serial.print("Puerto: ");
          Serial.println(_puerto);
          puerto_controlador = int(_puerto);

          if(clientControlador.connect(_server,_puerto)){
            Serial.println("Status: OK");
            return true;
          }else{
            Serial.println("Status: FALLO");
            return false;
          }
        }else if(servidor == serverSistema){
          _server = jObject["serverdest"];
          _puerto = jObject["puertodest"];
          _urldestino = jObject["urldestino"];
          snprintf(url_destino, 120, "%s", _urldestino);
          snprintf(servidor_sistema, 120, "%s", _server);
          puerto_sistema = int(_puerto);

          Serial.print("Servidor Destino: ");
          Serial.println(_server);
          Serial.print("Puerto: ");
          Serial.println(_puerto);
          if(clientSistema.connect(_server,_puerto)){
            Serial.println("Status: OK");
            return true;
          }else{
            Serial.println("Status: ERROR");
            return false;
          }
        }
      }
    }
  }else{
    Serial.println("Error al leer memoria FLASH");
    return false;
  }
}


void guardarDatoswifi(){
  String data = server.arg("plain");
  DynamicJsonBuffer jBuffer;
  JsonObject& jObject = jBuffer.parseObject(data);

  File configFile = SPIFFS.open("/config.json","w");
  jObject.printTo(configFile);
  configFile.close();

  server.send(200, "application/json", "{\"estado\":\"guardando datos\"}");
  delay(500);
}

void resetDispositivo(){
  server.send(200,"application/json","{\"estado\":\"reiniciando\"}");
  Serial.println("Reiniciando dispositivo.");
  delay(1000);
  ESP.restart();
}

void estado_servers(){
  char jsonConsulta[100];
  if(clientControlador.connected() && clientSistema.connected()){
    snprintf(jsonConsulta, 100, "{\"estadotcp\":\"Conectado\",\"puertotcp\":\"%i\",\"estadodest\":\"Conectado\",\"puertodest\":\"%i\"}", puerto_controlador, puerto_sistema);
  }else if(clientControlador.connected() && !clientSistema.connected()){
    snprintf(jsonConsulta, 100, "{\"estadotcp\":\"Conectado\",\"puertotcp\":\"%i\",\"estadodest\":\"Desconectado\",\"puertodest\":\"%i\"}", puerto_controlador, puerto_sistema);
  }else if(!clientControlador.connected() && clientSistema.connected()){
    snprintf(jsonConsulta, 100, "{\"estadotcp\":\"Desconectado\",\"puertotcp\":\"%i\",\"estadodest\":\"Conectado\",\"puertodest\":\"%i\"}", puerto_controlador, puerto_sistema);
  }else{
    snprintf(jsonConsulta, 100, "{\"estadotcp\":\"Desconectado\",\"puertotcp\":\"%i\",\"estadodest\":\"Desconectado\",\"puertodest\":\"%i\"}", puerto_controlador, puerto_sistema);
  }
  server.send(200,"application/json",jsonConsulta);
}

void enviarHttp(String url){
  //digitalWrite(pinLedHTTP, HIGH);
  Serial.print("Query: ");
  Serial.println(url);
  HTTPClient http;
  http.begin(url);
  int codigoRespuesta = http.GET();
  Serial.print("http code: ");
  Serial.println(codigoRespuesta);
  http.end(); 
}

void setup() {
  pinMode(pinLedEstado, OUTPUT);
  pinMode(pinLedTCP, OUTPUT);
  pinMode(pinLedHTTP, OUTPUT);
  pinMode(pinBoton, INPUT);

  SPIFFS.begin();
  delay(500);

  Serial.begin(115200);

  estadoWiFi = conectar_WiFi();

  if(estadoWiFi){
    digitalWrite(pinLedEstado, LOW);
    Serial.println("\nWiFi conectado, intentando conectar con el servidor.");
    Serial.print("ID: ");
    Serial.println(WiFi.macAddress());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(pinLedEstado, HIGH);
    if(conectar_Servidor(serverControlador)){
      Serial.println("Dispositivo conectado con el servidor 1");
      if(conectar_Servidor(serverSistema)){
        Serial.println("Dispositivo conectado con el servidor 2");
      }else{
        Serial.println("Error al conectar con el servidor2");  
      }
    }else{
      Serial.println("Error al conectar con el servidor1");
    }
  }else{
    conectar_AP();
    Serial.print("IP del servidor AP: ");
    Serial.println(ipEstatica);
  }

  server.on("/",[](){server.send_P(200,"text/html",pagina_web);});
  server.on("/config",HTTP_POST,guardarDatoswifi);
  server.on("/reset",HTTP_POST,resetDispositivo);
  server.on("/estado_servers",HTTP_POST,estado_servers);
  server.begin();
}

void loop() {
  unsigned long ahora = millis();

  while(clientControlador.connected()){
    digitalWrite(pinLedTCP, HIGH);
    while(clientControlador.available()){
      String line = clientControlador.readStringUntil('\n');
      String url_consulta = String(url_destino) + line;
      //Serial.println(url_consulta);
      if(!clientSistema.connected()){
        Serial.println("Dispositivo desconectado del servidor de destino\nIntentando reconectar...");
        conectar_Servidor(serverSistema);
        if(clientSistema.connected()){
          enviarHttp(url_consulta);
        }else{
          digitalWrite(pinLedHTTP, LOW);
          Serial.println("No se pudo enviar la consulta al servidor de destino");
        }
      }else{
        enviarHttp(url_consulta);
      }
    }
    unsigned long ahora = millis();
    if(!clientSistema.connected()){
      if((unsigned long)ahora - tiempoParpadeoAnterior > intervaloParpadeoNoserver2){
        tiempoParpadeoAnterior = millis();
        digitalWrite(pinLedHTTP, !digitalRead(pinLedHTTP));
      }
    }else{
      digitalWrite(pinLedHTTP, HIGH);
    }
    // if(!digitalRead(pinBoton)){
    //   delay(5000);
    //   if(!digitalRead(pinBoton)){
    //     resetDispositivo();
    //   }
    // }
    server.handleClient();
  }

  if(estadoWiFi){
    if(!clientControlador.connected()){
      digitalWrite(pinLedHTTP, LOW);
      if((unsigned long) ahora - tiempoParpadeoAnterior > intervaloParpadeoNoserver1){
        tiempoParpadeoAnterior = millis();
        digitalWrite(pinLedTCP, !digitalRead(pinLedTCP));
      }
      if((unsigned long) ahora - tiempoverficacionServidor > intervaloVerificacionServidor){
        tiempoverficacionServidor = millis();
        Serial.println("Dispositivo desconectado del servidor TCP");
        Serial.println("Intentando reconectar");
        conectar_Servidor(serverControlador);
      }
    }
  }else if(!estadoWiFi){
    digitalWrite(pinLedTCP, LOW);
    digitalWrite(pinLedHTTP, LOW);
    if((unsigned long)ahora - tiempoParpadeoAnterior > intervaloParpadeoNowifi){
      tiempoParpadeoAnterior = millis();
      digitalWrite(pinLedEstado, !digitalRead(pinLedEstado));
    }
  }

  // if(!digitalRead(pinBoton)){
  //   delay(5000);
  //   if(!digitalRead(pinBoton)){
  //     resetDispositivo();
  //   }
  // }

  server.handleClient();
}
