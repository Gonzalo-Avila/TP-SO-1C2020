#include "Broker.h"


void enviarEstructuraMensajeASuscriptor(void* estMensaje) {
	//TODO
	// - Hacer send() del mensaje al suscriptor
	// - Evaluar retorno del send()
	// - Cambiar estado "enviado" en cache
	// - Evaluar retorno del recv()
	// - Cambiar estado "confirmado" en cache
	// - Quitar mensaje de cola

	estructuraMensaje* estMsj = (estructuraMensaje*) estMensaje;
	log_debug(logger, "Se está enviando un mensaje al suscriptor %d",estMsj->clientID);
    log_debug(logger,"Se esta enviando el mensaje\nID: %d\nSuscriptor: %d\nID Correlativo: %d\nCola: %d\nSize: %d\nMensaje chorizeado: %s",
    		estMsj->id, estMsj->clientID,estMsj->idCorrelativo,estMsj->colaMensajeria,estMsj->sizeMensaje,(char*)(estMsj->mensaje));
	enviarMensajeASuscriptor(*estMsj, getSocketActualDelSuscriptor(estMsj->clientID, estMsj->colaMensajeria));
	estMsj->estado=ESTADO_ENVIADO;

}

bool esMensajeNuevo(void* mensaje) {
	estructuraMensaje* estMsj = (estructuraMensaje*) mensaje;
	bool esNuevo = false;
	if (estMsj->estado == ESTADO_NUEVO) {
		esNuevo = true;
	}
	return esNuevo;
}


void atenderColas() {
	// Se filtran los mensajes que tienen estado nuevo y se envian, segun tipo
	while (1) {
		sem_wait(&habilitarEnvio);
		sem_wait(&mutexColas);
		for (int numCola = 0; numCola < 6; numCola++) {
			if (list_size(getColaByNum(numCola)) > 0) {
				//t_list* mensajesNuevos = list_filter(getColaByNum(numCola), &esMensajeNuevo);
				//list_iterate(mensajesNuevos, &enviarEstructuraMensajeASuscriptor);
				list_iterate(getColaByNum(numCola), &enviarEstructuraMensajeASuscriptor);
			}
		}
		sem_post(&mutexColas);

	}
}

