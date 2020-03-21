const char adminPAGE[] PROGMEM = R"=====(
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1"/>
    <strong>Administración</strong>
    <hr>
    <a href="info.html" style="width:45%" class="btn btn--m btn--blue">Información general</a><br>
    <a href="datos.html" style="width:45%" class="btn btn--m btn--blue">Datos dispositivo</a><br>
    <a href="general.html" style="width:45%" class="btn btn--m btn--blue">Configuración General</a><br>
    <a href="config.html" style="width:45%" class="btn btn--m btn--blue">Configuración de red</a><br>
    <script>
    window.onload = function(){
        load("style.css", "css", function(){
            load("microajax.js", "js", function(){

            });
        });
    }
    function load(e,t,n){if("js"==t){var a=document.createElement("script");a.src=e,a.type="text/javascript",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}else if("css"==t){var a=document.createElement("link");a.href=e,a.rel="stylesheet",a.type="text/css",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}}
    </script>
)=====";