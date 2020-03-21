const char datosPAGE[] PROGMEM = R"=====(
<meta name="viewport" content="width=device-width, initial-scale=1" />
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<a href="/"  class="btn btn--s"><</a>&nbsp;&nbsp;<strong>Datos sensores</strong>
<hr>
<form action="" method="get">
<table style="width:350px" align="center">
<tr><td align="right">Humedad:</td><td><span id="humedad"></span></td></tr>
<tr><td align="right">Batería:</td><td><span id="bateria"></span></td></tr>
</table>
</form>
<script>
function GetState()
{
  setValues("/admin/valoressensores");
}
window.onload = function ()
{
  load("style.css","css", function() 
  {
    load("microajax.js","js", function() 
    {
          setValues("/admin/valoressensores");
    });
  });
}

let timer = setTimeout(function timer(){
    GetState();
    timer = setTimeout(timer, 3000);
},3000);

function load(e,t,n){if("js"==t){var a=document.createElement("script");a.src=e,a.type="text/javascript",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}else if("css"==t){var a=document.createElement("link");a.href=e,a.rel="stylesheet",a.type="text/css",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}}
</script>
)=====";

void send_data_html(){
    String values ="";
    
    // sondasTemp.requestTemperatures();

    values += "humedad|" + (String)obtenerHumedad() + "|div\n";
    values += "bateria|" + (String)obtenerVBAT() + "|div\n";
    // values += "sondaC|" + (String)sondasTemp.getTempCByIndex(2) + "|div\n";
  
    Server.send ( 200, "text/plain", values);
    Serial.println(__FUNCTION__); 
}