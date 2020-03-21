const char generalPAGE[] PROGMEM =  R"=====(
<meta name="viewport" content="width=device-width, initial-scale=1" />
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<a href="/"  class="btn btn--s"><</a>&nbsp;&nbsp;<strong>Configuración general</strong>
<hr>
<form action="" method="post">
<table align="center">
<tr><td align="center">Nombre:</td><td><input type="text" id="devicename" name="devicename" value=""></td></tr>
<tr><td align="center">Servidor LAN:</td><td><input type="text" id="serverlan" name="serverlan" value=""></td></tr>
<tr><td align="center">Puerto LAN:</td><td><input type="text" id="portlan" name="portlan" value=""></td></tr>
<tr><td align="center">Usuario LAN:</td><td><input type="text" id="userlan" name="userlan" value=""></td></tr>
<tr><td align="center">Password LAN:</td><td><input type="text" id="passlan" name="passlan" value=""></td></tr>
<tr><td align="center">Servidor WAN:</td><td><input type="text" id="serverwan" name="serverwan" value=""></td></tr>
<tr><td align="center">Puerto WAN:</td><td><input type="text" id="portwan" name="portwan" value=""></td></tr>
<tr><td align="center">Usuario WAN:</td><td><input type="text" id="userwan" name="userwan" value=""></td></tr>
<tr><td align="center">Password WAN:</td><td><input type="text" id="passwan" name="passwan" value=""></td></tr>
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
	if (Server.args() > 0 ){
		for ( uint8_t i = 0; i < Server.args(); i++ ) {
			if (Server.argName(i) == "devicename") urldecode(Server.arg(i), config.dispID);
			if (Server.argName(i) == "serverlan") urldecode(Server.arg(i), config.serverMQTTLAN);
			if (Server.argName(i) == "portlan") config.puertoMQTTLAN = Server.arg(i).toInt();
			if (Server.argName(i) == "userlan"){
				if(Server.argName(i).length() == 0){memset(config.userMQTTLAN, 0, sizeof(config.userMQTTLAN));}
				else{urldecode(Server.arg(i), config.userMQTTLAN);}
			}
			if (Server.argName(i) == "passlan"){
				if(Server.argName(i).length() == 0){memset(config.passMQTTLAN, 0, sizeof(config.passMQTTLAN));}
				else{urldecode(Server.arg(i), config.passMQTTLAN);}
			}
			if (Server.argName(i) == "serverwan") urldecode(Server.arg(i), config.serverMQTTWAN);
			if (Server.argName(i) == "portwan") config.puertoMQTTWAN = Server.arg(i).toInt();
			if (Server.argName(i) == "userwan"){
				if(Server.argName(i).length() == 0){memset(config.userMQTTWAN, 0, sizeof(config.userMQTTWAN));}
				else{urldecode(Server.arg(i), config.userMQTTWAN);}
			}
			if (Server.argName(i) == "passwan"){
				if(Server.argName(i).length() == 0){memset(config.passMQTTWAN, 0, sizeof(config.passMQTTWAN));}
				else{urldecode(Server.arg(i), config.passMQTTWAN);}
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
    values += "serverlan|" +  (String) config.serverMQTTLAN + "|input\n";
    values += "portlan|" +  (String) config.puertoMQTTLAN + "|input\n";
	values += "userlan|" +  (String) config.userMQTTLAN + "|input\n";
	values += "passlan|" +  (String) config.passMQTTLAN + "|input\n";
	values += "serverwan|" +  (String) config.serverMQTTWAN + "|input\n";
    values += "portwan|" +  (String) config.puertoMQTTWAN + "|input\n";
	values += "userwan|" +  (String) config.userMQTTWAN + "|input\n";
	values += "passwan|" +  (String) config.passMQTTWAN + "|input\n";
 
	Server.send ( 200, "text/plain", values);
	Serial.println(__FUNCTION__); 
}