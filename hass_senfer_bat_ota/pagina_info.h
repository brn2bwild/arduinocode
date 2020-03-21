const char infoPAGE[] PROGMEM = R"=====(
<meta name="viewport" content="width=device-width, initial-scale=1" />
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link rel="stylesheet" href="style.css" type="text/css" />
<script src="microajax.js"></script> 
<a href="/"  class="btn btn--s"><</a>&nbsp;&nbsp;<strong>Información general</strong>
<hr>
<table style="width:310px" align="center">
<tr><td align="right">SSID:</td><td><span id="x_ssid"></span></td></tr>
<tr><td align="right">IP:</td><td><span id="x_ip"></span></td></tr>
<tr><td align="right">Nombre:</td><td><span id="x_name"></span></td></tr>
<tr><td align="right">Servidor:</td><td><span id="x_server"></span></td></tr>
<tr><td align="right">Puerto:</td><td><span id="x_port"></span></td></tr>
<tr><td align="right">Mac :</td><td><span id="x_mac"></span></td></tr>
<tr><td colspan="2" align="center"><a href="javascript:GetState()" class="btn btn--m btn--blue">Refrescar</a></td></tr>
</table>
<script>
function GetState()
{
  setValues("/admin/valoresinfo");
}
window.onload = function ()
{
  load("style.css","css", function() 
  {
    load("microajax.js","js", function() 
    {
        GetState();
    });
  });
}
function load(e,t,n){if("js"==t){var a=document.createElement("script");a.src=e,a.type="text/javascript",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}else if("css"==t){var a=document.createElement("link");a.href=e,a.rel="stylesheet",a.type="text/css",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}}
</script>
)=====" ;

void send_information_values_html(){
  String values ="";

  values += "x_ssid|" + (String)WiFi.SSID() +  "|div\n";
  values += "x_ip|" +  (String)WiFi.localIP()[0] + "." +  (String) WiFi.localIP()[1] + "." +  (String) WiFi.localIP()[2] + "." + (String) WiFi.localIP()[3] +  "|div\n";
  values += "x_name|" +  (String)config.dispID +  "|div\n";
  values += "x_server|" +  (String)config.serverMQTT +  "|div\n";
  values += "x_port|" +  (String)config.puertoMQTT +  "|div\n";
  values += "x_mac|" + (String)config.MAC +  "|div\n";
  Server.send ( 200, "text/plain", values);
  Serial.println(__FUNCTION__); 
}