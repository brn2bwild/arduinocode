var estado_encendido = false;
var estado_apagado = false;
var estado_sistema = false;

clienteMQTT = new Paho.MQTT.Client("localhost", 9001, "webAPI");

clienteMQTT.onConnectionLost = onConnectionLost;
clienteMQTT.onMessageArrived = onMessageArrived;

clienteMQTT.connect({onSuccess:onConnect});

function onConnect(){
    console.log("Conectado al servidor MQTT");
    clienteMQTT.subscribe("sistema/estado/nodorf01");
    mensaje = new Paho.MQTT.Message("ON");
    mensaje.destinationName = "sistema/estado/web";
    clienteMQTT.send(mensaje);
}

function onConnectionLost(objetoRespuesta){
    if(objetoRespuesta.errorCode != 0){
        console.log("Error conexión perdida: " + objetoRespuesta.errorMessage);
    }
}

function onMessageArrived(mensaje){
    console.log("Mensaje recibido: " + mensaje.payloadString);
}

function ActualizarHora(){
    var fecha = new Date();
    var segundos = fecha.getSeconds();
    var minutos = fecha.getMinutes();
    var horas = fecha.getHours();

    var hora_programada_on = document.getElementById("hora-prog-encendido");
    var prog_hora_on = hora_programada_on.value.split(":")[0];
    var prog_min_on = hora_programada_on.value.split(":")[1];

    var hora_programada_off = document.getElementById("hora-prog-apagado");
    var prog_hora_off = hora_programada_off.value.split(":")[0];
    var prog_min_off = hora_programada_off.value.split(":")[1];

    var encendido_activo = document.getElementById("activar-encendido").checked;
    var apagado_activo = document.getElementById("activar-apagado").checked;

    if(prog_hora_on == horas && prog_min_on == minutos && estado_encendido == false && encendido_activo){
        estado_encendido = true;
        establecerEstado(1);
    }
    if(prog_hora_off == horas && prog_min_off == minutos && estado_apagado == false && apagado_activo){
        estado_apagado = true;
        establecerEstado(0);
    }

    if(minutos > prog_min_on && estado_encendido == true){
        estado_encendido = false;
    }

    if(minutos > prog_min_off && estado_apagado == true){
        estado_apagado = false;
    }

    if(document.getElementById("control-estado").checked && estado_sistema != true){
        estado_sistema = true;
        establecerEstado(1);
    }else if(document.getElementById("control-estado").checked == false && estado_sistema == true){
        estado_sistema = false;
        establecerEstado(0);
    }

    meridiano = (horas < 12) ? "a.m." : "p.m.";
    horas = (horas == 0) ? 12 : horas;
    horas = (horas > 12) ? horas - 12 : horas;

    horas = checarDatoHora(horas);
    minutos = checarDatoHora(minutos);
    segundos = checarDatoHora(segundos);

    document.getElementById("hora-actual").innerHTML = horas + " : " + minutos + " : " + segundos + " " + meridiano;

    setTimeout(ActualizarHora, 500);
}

function establecerEstado(estado){
    if(estado == 1){
        mensaje = new Paho.MQTT.Message("ON");
        mensaje.destinationName = "sistema/comando/nodorf01";
        clienteMQTT.send(mensaje);
    }else{
        mensaje = new Paho.MQTT.Message("OFF");
        mensaje.destinationName = "sistema/comando/nodorf01";
        clienteMQTT.send(mensaje);
    }
}

function checarDatoHora(i){
    if(i < 10){ i = "0" + i;}
    return i;
}

function programarHora(){
    var hora = document.getElementById("hora-prog");
    evento_realizado = false;
}

setTimeout(ActualizarHora, 500);