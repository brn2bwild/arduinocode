const char networkPAGE[] PROGMEM = R"=====(
<meta name="viewport" content="width=device-width, initial-scale=1" />
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<a href="/"  class="btn btn--s"><</a>&nbsp;&nbsp;<strong>Configuración de red</strong>
<hr>
Conectarse al router con éstos datos:<br>
<form action="" method="get">
<table style="width:310px;" align="center">
<tr><td align="center">SSID:</td><td><input type="text" id="ssid" name="ssid" value=""></td></tr>
<tr><td align="center">Password:</td><td><input type="text" id="password" name="password" value=""></td></tr>
</table>
<br>
<input type="submit" style="width:150px" class="btn btn--m btn--blue" value="Guardar">
</form>
<hr>
<strong>Estado de la conexión:</strong><div id="connectionstate" align="center">N/A</div>
<hr>
<strong>Redes:</strong><br>
<table style="width:310px" align="center">
<tr><td align="center"><div id="networks" style="text-align: center;">Escaneando...</div></td></tr>
<tr><td align="center"><a href="javascript:GetState()" style="width:150px" class="btn btn--m btn--blue">Refrescar</a></td></tr>
</table>
<script>
function GetState()
{
	setValues("/admin/estadoconexion");
}
function selssid(value)
{
	document.getElementById("ssid").value = value; 
}
window.onload = function ()
{
	load("style.css","css", function() 
	{
		load("microajax.js","js", function() 
		{
					setValues("/admin/valores");
					setTimeout(GetState,3000);
		});
	});
}
function load(e,t,n){if("js"==t){var a=document.createElement("script");a.src=e,a.type="text/javascript",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}else if("css"==t){var a=document.createElement("link");a.href=e,a.rel="stylesheet",a.type="text/css",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}}
</script>
)=====";

const char esperaPAGE[] PROGMEM = R"=====(
<meta http-equiv="refresh" content="5; URL=config.html" charset=utf-8">
Espere un momento, guardando la configuración...
)=====";


//
//  SEND HTML PAGE OR IF A FORM SUMBITTED VALUES, PROCESS THESE VALUES
// 

void send_network_configuration_html(){
	
	if (Server.args() > 0 ){  // Save Settings
		// String temp = "";
		// config.configurado = false;
		for ( uint8_t i = 0; i < Server.args(); i++ ) {
			if(Server.argName(i) == "ssid") urldecode(Server.arg(i), config.ssid);
			if(Server.argName(i) == "password") urldecode(Server.arg(i), config.password);
			// if(Server.argName(i) == "server") urldecode(Server.arg(i), config.serverMQTT);
            // if(Server.argName(i) == "id") urldecode(Server.arg(i), config.dispID);
            // if(Server.argName(i) == "dhcp") config.configurado = true;
		}
		Server.send ( 200, "text/html", esperaPAGE );
		guardarConfig();
		configurarWiFi();		
	}
	else
	{
		Server.send ( 200, "text/html", networkPAGE); 
	}
	Serial.println(__FUNCTION__); 
}

void send_network_configuration_values_html(){
	String values ="";

	values += "ssid|" + (String) config.ssid + "|input\n";
	values += "password|" +  (String) config.password + "|input\n";
	// values += "server|" +  (String) config.serverMQTT + "|input\n";
	// values += "id|" +  (String) config.dispID + "|input\n";
	// values += "dhcp|" +  (String) (config.configurado ? "checked" : "") + "|chk\n";
	Server.send ( 200, "text/plain", values);
	Serial.println(__FUNCTION__); 
}

void send_connection_state_values_html(){

	String state = "N/A";
	String Networks = "";
	if (WiFi.status() == 0) state = "Idle";
	else if (WiFi.status() == 1) state = "NO SSID DISPONIBLE";
	else if (WiFi.status() == 2) state = "ESCANEO COMPLETADO";
	else if (WiFi.status() == 3) state = "CONECTADO";
	else if (WiFi.status() == 4) state = "CONEXIÓN FALLIDA";
	else if (WiFi.status() == 5) state = "CONEXIÓN PERDIDA";
	else if (WiFi.status() == 6) state = "DESCONECTADO";

	int n = WiFi.scanNetworks();

	if (n == 0){
		 Networks = "<font color='#FF0000'>No se encontraron redes</font>";
	}else{		
		Networks = "Se encontraron " +String(n) + " Redes<br>";
		Networks += "<table border='0' cellspacing='0' cellpadding='3'>";
		Networks += "<tr bgcolor='#DDDDDD' ><td><strong>Nombre</strong></td><td><strong>Señal</strong></td><td><strong>Seg</strong></td><tr>";
		for (int i = 0; i < n; ++i){
			int quality=0;
			if(WiFi.RSSI(i) <= -100){
					quality = 0;
			}else if(WiFi.RSSI(i) >= -50){
					quality = 100;
			}else{
				quality = 2 * (WiFi.RSSI(i) + 100);
			}
			Networks += "<tr><td><a href='javascript:selssid(\""  +  String(WiFi.SSID(i))  + "\")'>"  +  String(WiFi.SSID(i))  + "</a></td><td>" +  String(quality) + "%</td><td>" +  String((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*")  + "</td></tr>";
		}
		Networks += "</table>";
	}
	String values ="";
	values += "connectionstate|" +  state + "|div\n";
	values += "networks|" +  Networks + "|div\n";
	Server.send ( 200, "text/plain", values);
	Serial.println(__FUNCTION__); 
}
