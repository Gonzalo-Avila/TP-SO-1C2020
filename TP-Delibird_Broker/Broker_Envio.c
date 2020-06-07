#include "Broker.h"

/*void eliminarMensaje(estructuraMensaje* estMsj){
	t_list* colaMensaje = getColaByNum(estMsj->colaMensajeria);
	//TODO_OLD -> no se uso al final, se quitan todos de una en vez de elemento por element
	// - Usar get, take o alguna funcion de las commons de listas para eliminar ese nodo
}*/

void agregarAListaDeEnviados(uint32_t idMsg, uint32_t idProceso){
	log_debug(logger, "agregarAListaDeEnviados para clientID %d y idMsg %d ", idProceso, idMsg);
    bool esElRegistroQueBusco(void * registro){
        registroCache * reg = (registroCache *) registro;
        return reg->idMensaje==idMsg;
    }
	registroCache * registroAActualizar = list_find(registrosDeCache,(void*)esElRegistroQueBusco);
	if(registroAActualizar!=NULL){
		uint32_t* idProcesoAAgregar = malloc(sizeof(uint32_t));
		*idProcesoAAgregar=idProceso;

		bool tieneElMismoID(void* idP){
			uint32_t* idProc = (uint32_t*) idP;
			return *idProc == *idProcesoAAgregar;
		}
		if(!list_any_satisfy(registroAActualizar->procesosALosQueSeEnvio,(void*) tieneElMismoID)){
			list_add(registroAActualizar->procesosALosQueSeEnvio,idProcesoAAgregar);
			log_debug(logger, "Se agrego a clientID %d lista de enviados de mensaje con id %d ", *idProcesoAAgregar, idMsg);
		}else{
			free(idProcesoAAgregar);
		}
	}else{
		log_debug(logger, "No se encontro registro para agregarAListaDeEnviados");
	}
}

void agregarAListaDeConfirmados(uint32_t idMsg, uint32_t idProceso){
	log_debug(logger, "agregarAListaDeConfirmados para clientID %d y idMsg %d ", idProceso, idMsg);
    bool esElRegistroQueBusco(void * registro){
        registroCache * reg = (registroCache *) registro;
        return reg->idMensaje==idMsg;
    }
	registroCache * registroAActualizar = list_find(registrosDeCache,(void*)esElRegistroQueBusco);
	if(registroAActualizar!=NULL){
		uint32_t* idProcesoAAgregar = malloc(sizeof(uint32_t));
		*idProcesoAAgregar=idProceso;
		list_add(registroAActualizar->procesosQueConfirmaronRecepcion,idProcesoAAgregar);
		log_debug(logger, "Se agrego a clientID %d lista de confirmados de mensaje con id %d ", *idProcesoAAgregar, idMsg);
	}else{
		log_debug(logger, "No se encontro registro para agregarAListaDeConfirmados");
	}
}

void imprimirListasIDs(uint32_t idMsg){

	bool esElRegistroQueBusco(void * registro){
	        registroCache * reg = (registroCache *) registro;
	        return reg->idMensaje==idMsg;
	}
	registroCache * registroAActualizar = list_find(registrosDeCache,(void*)esElRegistroQueBusco);

	int i = 1;
	void imprimirElemento(void * elemento){
		uint32_t* cid = (uint32_t*)elemento;
		log_debug(logger, "Elemento #%d: %d", i, *cid);
		i++;
	}

	log_debug(logger, "procesosALosQueSeEnvio");
	list_iterate(registroAActualizar->procesosALosQueSeEnvio, imprimirElemento);
	i = 1;
	log_debug(logger, "procesosQueConfirmaronRecepcion");
	list_iterate(registroAActualizar->procesosQueConfirmaronRecepcion, imprimirElemento);
}

void enviarEstructuraMensajeASuscriptor(void* estMensaje) {
	//TODO_OLD
	// - Hacer send() del mensaje al suscriptor
	// - Evaluar retorno del send()
	// - Cambiar estado "enviado" en cache
	// - Evaluar retorno del recv()
	// - Cambiar estado "confirmado" en cache
	// - Quitar mensaje de cola
	//
	// Nota: no quita un elemento a la vez, sino que envía todos y despues limpia la lista

	estructuraMensaje* estMsj = (estructuraMensaje*) estMensaje;
	int estadoDeEnvio, socketDelSuscriptor;
	uint32_t ack=0;

	log_debug(logger, "Se está enviando un mensaje al suscriptor %d",estMsj->clientID);
    log_debug(logger,"Se esta enviando el mensaje\nID: %d\nSuscriptor: %d\nID Correlativo: %d\nCola: %d\nSize: %d\nMensaje chorizeado: %s",
    		estMsj->id, estMsj->clientID,estMsj->idCorrelativo,estMsj->colaMensajeria,estMsj->sizeMensaje,(char*)(estMsj->mensaje));

    socketDelSuscriptor = getSocketActualDelSuscriptor(estMsj->clientID, estMsj->colaMensajeria);

	estadoDeEnvio=enviarMensajeASuscriptor(*estMsj,socketDelSuscriptor);
	if(estadoDeEnvio>=0)
	{
		agregarAListaDeEnviados(estMsj->id,estMsj->clientID);
		estMsj->estado=ESTADO_ENVIADO;
		recv(socketDelSuscriptor,&ack, sizeof(uint32_t),MSG_WAITALL);
		if(ack==1){
			agregarAListaDeConfirmados(estMsj->id,estMsj->clientID);
			estMsj->estado=ESTADO_CONFIRMADO;
		}else{
			desuscribir(estMsj->clientID, estMsj->colaMensajeria);
		}
	}else{
		desuscribir(estMsj->clientID, estMsj->colaMensajeria);
	}

	imprimirListasIDs(estMsj->id);

}

bool esMensajeNuevo(void* mensaje) {
	estructuraMensaje* estMsj = (estructuraMensaje*) mensaje;
	bool esNuevo = false;
	if (estMsj->estado == ESTADO_NUEVO) {
		esNuevo = true;
	}
	return esNuevo;
}

void destructorNodos(void * nodo){

	estructuraMensaje * estMsj = (estructuraMensaje *) nodo;
	free(estMsj->mensaje);
}
void atenderColas() {
	// Se filtran los mensajes que tienen estado nuevo y se envian, segun tipo
	while (1) {
		sem_wait(&habilitarEnvio);
		sem_wait(&mutexColas);
		for (int numCola = 0; numCola < 6; numCola++) {
			if (list_size(getColaByNum(numCola)) > 0) {

				list_iterate(getColaByNum(numCola), &enviarEstructuraMensajeASuscriptor);
				list_clean(getColaByNum(numCola));
				//list_clean_and_destroy_elements(getColaByNum(numCola), &destructorNodos);
			}
		}
		sem_post(&mutexColas);

	}
}

