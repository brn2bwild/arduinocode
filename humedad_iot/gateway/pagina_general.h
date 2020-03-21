const char generalPAGE[] PROGMEM =  R"=====(
<meta name="viewport" content="width=device-width, initial-scale=1" />
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<a href="/"  class="btn btn--s"><</a>&nbsp;&nbsp;<strong>Configuración general</strong>
<hr>
<form action="" method="post">
<table align="center">
<tr><td align="center">Nombre:</td><td><input type="text" id="devicename" name="devicename" value="" maxlength="8"></td></tr>
<tr><td align="center">Clave:</td><td><input type="password" id="clave" name="clave" value="" maxlength="4"></td></tr>
<tr><td align="center">Servidor:</td><td><input type="text" id="server" name="server" value="" maxlength="32"></td></tr>
<tr><td align="center">Puerto:</td><td><input type="text" id="port" name="port" value="" maxlength="5"></td></tr>
<tr><td align="center">Usuario:</td><td><input type="text" id="user" name="user" value="" maxlength="32"></td></tr>
<tr><td align="center">Password:</td><td><input type="password" id="pass" name="pass" value="" maxlength="32"></td></tr>
</table>
<br>
<input type="submit" style="width:150px" class="btn btn--m btn--blue" value="Guardar">
</form>
<script>
 
window.onload = function ()
{
	load("style.css","css", function() 
	{
		load("microajax.js","js", function() 
		{
				setValues("/admin/valoresgenerales");
		});
	});
}
function load(e,t,n){if("js"==t){var a=document.createElement("script");a.src=e,a.type="text/javascript",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}else if("css"==t){var a=document.createElement("link");a.href=e,a.rel="stylesheet",a.type="text/css",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}}
</script>
)=====";


// Functions for this Page
void send_devicename_value_html(){
	String values ="";
	values += "devicename|" + (String) config.dispID + "|input\n";
	Server.send ( 200, "text/plain", values);
	Serial.println(__FUNCTION__); 
	
}

void send_general_html(){
	if (Server.args() > 0 ){  // Save Settings
		// String temp = "";
		for ( uint8_t i = 0; i < Server.args(); i++ ) {
			if (Server.argName(i) == "devicename") urldecode(Server.arg(i), config.dispID, sizeof(config.dispID));
			if (Server.argName(i) == "clave"){//config.clave = Server.arg(i).toInt();
				if(Server.argName(i).length() == 0){memset(config.clave, 0, sizeof(config.clave));}
				else{urldecode(Server.arg(i), config.clave, sizeof(config.clave));}
			}
			if (Server.argName(i) == "server") urldecode(Server.arg(i), config.serverMQTT, sizeof(config.serverMQTT));
			if (Server.argName(i) == "port") config.puertoMQTT = Server.arg(i).toInt();
			if (Server.argName(i) == "user"){
				if(Server.argName(i).length() == 0){memset(config.userMQTT, 0, sizeof(config.userMQTT));}
				else{urldecode(Server.arg(i), config.userMQTT, sizeof(config.userMQTT));}
			}
			if (Server.argName(i) == "pass"){
				if(Server.argName(i).length() == 0){memset(config.passMQTT, 0, sizeof(config.passMQTT));}
				else{urldecode(Server.arg(i), config.passMQTT, sizeof(config.passMQTT));}
			}
		}
		guardarConfig();
	}
	Server.send ( 200, "text/html", generalPAGE); 
	Serial.println(__FUNCTION__); 
}

void send_general_configuration_values_html(){
	String values ="";
	values += "devicename|" + (String) config.dispID + "|input\n";
	values += "clave|" + (String) config.clave + "|input\n";
    values += "server|" +  (String) config.serverMQTT + "|input\n";
    values += "port|" +  (String) config.puertoMQTT + "|input\n";
	values += "user|" +  (String) config.userMQTT + "|input\n";
	values += "pass|" +  (String) config.passMQTT + "|input\n";
 
	Server.send ( 200, "text/plain", values);
	Serial.println(__FUNCTION__); 
}