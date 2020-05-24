#include "Broker.h"

void * cacheBroker;
int CACHESIZE;

void inicializarCache() {
	/*
	 * La cache tiene que ser un espacio fijo, reservado al momento de ejecutar el broker. Tiene que ser un espacio tipo void
	 * porque no se sabe que se va a guardar ahi, pero tambien tiene que estar inicializado para poder diferenciar que espacios
	 * estan vacios. Hay que averiguar como hacer esto bien, puse un caracter para llenar.
	 */
	CACHESIZE = config_get_int_value(config, "TAMANO_MEMORIA");
	cacheBroker = malloc(CACHESIZE);
	memset(cacheBroker, 'f', CACHESIZE);
}

//----------------------- [CACHE] -------------------------//


int chequearSiAlcanza(int sizeMensaje, void * posicionActual,
		int memoriaRecorrida) {
	/* Chequea la disponibilidad de tantas posiciones contiguas como sea el tamaño del mensaje. Si en el proceso encuentra alguna
	 * ocupada retorna 0 (no alcanza el espacio), si termina sin encontrar ninguna ocupada retorna 1 (espacio suficiente).
	 */
	for (int offset = 1; offset < sizeMensaje; offset++) {
		if (memoriaRecorrida == CACHESIZE
				|| *(char *) (posicionActual + offset) != 'f') //LAZY EVALUATION POWA
			return 0;
		memoriaRecorrida++;
	}
	return 1;

}

void * buscarEspacio(int sizeMensaje, void *posicionInicial) {
	/* Busca secuencialmente algun byte marcado como libre, cuando lo encuentra llama a otra funcion para ver si alcanza el
	 * espacio para guardar el mensaje
	 *
	 */
	int espacioSuficiente = 0;
	int offset = -1;
	while (!espacioSuficiente) {
		offset++;
		if (offset == CACHESIZE)
			return NULL;

		if (*(char *) (posicionInicial + offset) == 'f') {
			espacioSuficiente = chequearSiAlcanza(sizeMensaje,
					posicionInicial + offset, offset);
		}


	}
	return posicionInicial + offset;
}


int tamanioDelMensaje(int offset, int *cacheExcedida) {
	/* Partiendo desde una posición ocupada, cuenta todas las posiciónes contiguas llenas para determinar el tamaño del mensaje
	 *
	 */
	int tamanio = 0;
	while (*(char *) (cacheBroker + offset) != 'f') {
		tamanio++;
		offset++;
		if (offset == CACHESIZE) {
			*cacheExcedida = 1;
			break;
		}
	}
	return tamanio;
}

void * buscarProximoMensaje(int offset, int * tamanio, int *cacheExcedida) {
	/* Partiendo desde una posición vacia, busca secuencialmente hasta encontrar una posición ocupada
	 *
	 */
	while (*(char *) (cacheBroker + offset) != 'f') {
		offset++;
		if (offset == CACHESIZE) {
			*cacheExcedida = 1;
			return NULL;
		}
	}
	*tamanio = tamanioDelMensaje(offset, cacheExcedida);

	return cacheBroker + offset;

}



void compactarMemoria() {
	/* Libera los "huecos" de la memoria cache, haciendo que todos los mensajes esten pegados uno detras del otro.
	 *
	 */
	int tamanio;
	int cacheExcedida = 0;
	void * posicion;
	for (int offset = 0; offset < CACHESIZE; offset++) {
		if (*(char *) (cacheBroker + offset) == 'f') {
			posicion = buscarProximoMensaje(offset, &tamanio, &cacheExcedida);
			if (cacheExcedida == 0) {
				memcpy(cacheBroker + offset, posicion, tamanio);
				memset(posicion, 'f', tamanio);
			} else {
				break;
			}
		}
	}
}

void eliminarMensaje(){
	//Seleccionar victima segun criterio definido en config

}

void * cachearConBuddySystem(void * mensaje, int sizeMensaje){
    void * particion;
    return particion;
}

/* Guarda el mensaje en memoria cache mediante algoritmo first fit
 * 1) Busca ponerlo en un espacio libre.
 * 2) Si no puede, compacta la memoria y vuelve a probar.
 * 3) Si no puede, borra el mensaje mas viejo y vuelve a probar.
 * 4) Si no puede, repite 2 y 3 hasta que lo mete.
 *
 * - El algoritmo deberia hacer una n cantidad de busquedas de espacio antes de compactar (??????????)
 * - Hay que validar el caso extremo en el que el mensaje que se quiere guardar no entre en toda cache, en esta función o en otra
 * - Hay que ver como poner las distintos criterios de victima
 */

void * usarBestFit(){
    void * particion;
    return particion;
}

void * usarFirstFit(void * mensaje, int sizeMensaje){
	void * particion;
	particion = buscarEspacio(sizeMensaje, cacheBroker);
	while (particion == NULL) {
		compactarMemoria();
		particion = buscarEspacio(sizeMensaje, cacheBroker);
		if (particion == NULL) {
			eliminarMensaje();
			particion = buscarEspacio(sizeMensaje, cacheBroker);
		}
	}
	memcpy(particion, mensaje, sizeMensaje);
	return particion;

}

void * cachearConParticionesDinamicas(void * mensaje, int sizeMensaje){
	if(strcmp("FF",config_get_string_value(config,"ALGORITMO_PARTICION_LIBRE"))==0)
       return usarFirstFit(mensaje,sizeMensaje);
	else
      return usarBestFit(mensaje,sizeMensaje);
}


void cachearMensaje(uint32_t idMensaje, uint32_t idCorrelativo, cola colaMensaje, uint32_t sizeMensaje, void * mensaje){

   registroCache * nuevoRegistro = malloc(sizeof(registroCache));


   nuevoRegistro->idMensaje=idMensaje;
   nuevoRegistro->idCorrelativo=idCorrelativo;
   nuevoRegistro->colaMensaje=colaMensaje;
   nuevoRegistro->procesosALosQueSeEnvio=list_create();
   nuevoRegistro->procesosQueConfirmaronRecepcion=list_create();
   nuevoRegistro->sizeMensaje=sizeMensaje;


   if(strcmp("BS",config_get_string_value(config, "ALGORITMO_MEMORIA"))==0)
	   nuevoRegistro->posicionEnMemoria=cachearConBuddySystem(mensaje, sizeMensaje);
   else
	   nuevoRegistro->posicionEnMemoria=cachearConParticionesDinamicas(mensaje, sizeMensaje);

   list_add(registrosDeCache,nuevoRegistro);
}



bool elSuscriptorNoEstaEnLaLista( t_list * lista, uint32_t idSuscriptor){
   bool esDistinto(void * suscriptor){
        uint32_t * sus = (uint32_t *) suscriptor;
        return *sus != idSuscriptor;
   }
   return list_all_satisfy(lista,&esDistinto);
}

t_list * getListaDeRegistrosFiltrados(suscriptor * nuevoSuscriptor, cola codSuscripcion)
{
	bool estaEnLaColaYNoSeConfirmoAun(void * registro){
	     registroCache * reg = (registroCache *) registro;
         return reg->colaMensaje==codSuscripcion &&
        		 elSuscriptorNoEstaEnLaLista(reg->procesosQueConfirmaronRecepcion, nuevoSuscriptor->clientID);
	}
	return list_filter(getListaSuscriptoresByNum(codSuscripcion),&estaEnLaColaYNoSeConfirmoAun);
}




void enviarMensajes(t_list * mensajesAEnviar, suscriptor * suscriptor )
{
	void enviarMensajeAlSuscriptor(void * registro){
	    registroCache * reg = (registroCache *) registro;
	    estructuraMensaje mensajeAEnviar;
	    int socketSuscriptor, statusEnvio, ack=0;

	    mensajeAEnviar.colaMensajeria=reg->colaMensaje;
	    mensajeAEnviar.id=reg->idMensaje;
	    mensajeAEnviar.idCorrelativo=reg->idCorrelativo;
	    mensajeAEnviar.sizeMensaje=reg->sizeMensaje;

	    mensajeAEnviar.mensaje=malloc(mensajeAEnviar.sizeMensaje);
	    memcpy(mensajeAEnviar.mensaje,reg->posicionEnMemoria,mensajeAEnviar.sizeMensaje);

	    socketSuscriptor = getSocketActualDelSuscriptor(suscriptor->clientID,reg->colaMensaje);

	    statusEnvio=enviarMensajeASuscriptor(mensajeAEnviar, socketSuscriptor);

		if(statusEnvio>=0)
			agregarAListaDeEnviados(mensajeAEnviar.id,suscriptor->clientID);

		recv(socketSuscriptor,&ack, sizeof(uint32_t),MSG_WAITALL);

		if(ack==1)
			agregarAListaDeConfirmados(mensajeAEnviar.id,suscriptor->clientID);

		free(mensajeAEnviar.mensaje);
	}

	    list_iterate(mensajesAEnviar, &enviarMensajeAlSuscriptor);
}

void enviarMensajesCacheados(suscriptor * nuevoSuscriptor, cola codSuscripcion) {
	/*TODO - DONE
	 *	- Filtrar los registros pertenecientes a la cola del codSuscripcion, en las cuales no figure registrado el ID del
	 *    nuevoSuscriptor en la lista de cofirmados. (DONE)
	 *  - Por cada nodo de la lista resultante, llenar una instancia de estructuraMensaje con los datos registrados, y enviar
	 *    el mensaje al suscriptor con la funcion enviarMensajeASuscriptor.
	 *  - Segun el resultado del send, agregar o no a la lista de enviados.
	 *  - Esperar ack del suscriptor.
	 *  - Segun resultado del recv, agregar o no a la lista de confirmados.
	 *
	 */
    t_list * mensajesAEnviar = getListaDeRegistrosFiltrados(nuevoSuscriptor,codSuscripcion);
    enviarMensajes(mensajesAEnviar,nuevoSuscriptor);


}








