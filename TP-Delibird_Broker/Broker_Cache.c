#include "Broker.h"

void * cacheBroker;
int CACHESIZE;

void crearRegistroInicial() {

	registroParticion * regParticion = malloc(sizeof(registroParticion));
	regParticion->nroParticion = 0;
	regParticion->posInicialFisica = cacheBroker;
	regParticion->tamanioParticion = CACHESIZE;
	regParticion->estado = LIBRE;
	regParticion->idMensaje = -1;

	list_add(registrosDeParticiones, regParticion);

}

int BSCacheSize(int size) {
	int sizeAux = 1;
	while (sizeAux < size) {
		if (sizeAux * 2 > size) {
			return sizeAux;
		}

		else {
			sizeAux = sizeAux * 2;
		}

	}
	return sizeAux;
}

int adaptarCacheSize(int size) {
	if (strcmp("BS", config_get_string_value(config, "ALGORITMO_MEMORIA")) == 0)
		size = BSCacheSize(size);
	return size;
}

void inicializarCache() {

	CACHESIZE = adaptarCacheSize(
			config_get_int_value(config, "TAMANO_MEMORIA"));
	cacheBroker = malloc(CACHESIZE);
	crearRegistroInicial();

}

int XOR(int a, int b) {
	return a ^ b;
}

void * cachearConBuddySystem(void * mensaje, int sizeMensaje) {
	// TODO
	void * particion = 0;
	return particion;
}

void * usarBestFit(void * mensaje, int sizeMensaje) {
	/* TODO
	 * -
	 * - Filtrar en registrosDeCache TODOS los que:
	 *		- Estado = LIBRE
	 *		- Tamanio >= sizeMensaje
	 * - Si no encontro ==> Vaciar/Compactar Particion Segun Algoritmo
	 * - Si encontro ==>
	 * 		- Tomar el de menor "tamanio"
	 * 		- Copiar "mensaje" en posInicialFisica del registro elegido
	 * 		- Crear registro nuevo con datos de mensaje
	 * 		- Crear registro nuevo con particion libre restante
	 * 		- Reasignar nroParticion a todos los registros en "registrosDeCache"
	 */

	void * particion = 0;
	return particion;
}

void * usarFirstFit(void * mensaje, int sizeMensaje) {
	/* TODO
	 * -
	 * - Buscar en registrosDeCache el PRIMERO que:
	 *		- Estado = LIBRE
	 *		- Tamanio >= sizeMensaje
	 * - Si no encontro ==> Vaciar/Compactar Particion Segun Algoritmo
	 * - Si encontro ==>
	 * 		- Copiar "mensaje" en posInicialFisica del registro elegido
	 * 		- Crear registro nuevo con datos de mensaje
	 * 		- Crear registro nuevo con particion libre restante
	 * 		- Reasignar nroParticion a todos los registros en "registrosDeCache"
	 */

	void * particion = 0;
	return particion;

}

bool hayEspacioLibrePara(int sizeMensaje) {

	return true;
}

void vaciarParticion() {

}

void compactarCache() {

}

void limpiarCache() {
	void destroyer(void* elem){
	}
	list_clean_and_destroy_elements(registrosDeParticiones,(void* ) destroyer);
	crearRegistroInicial();
}

bool hayMensajes(){
	bool estaOcupado(void* regParticion){
		registroParticion* regPart = (registroParticion*) regParticion;
		return regPart->estado == OCUPADO;
	}
	return list_any_satisfy(registrosDeParticiones, (void *)estaOcupado);
}

void asegurarEspacioLibrePara(int sizeMensaje) {

	int cantBusquedas = config_get_int_value(config, "FRECUENCIA_COMPACTACION");
	if (cantBusquedas != -1) {
		int i = 0;
		while (!hayEspacioLibrePara(sizeMensaje)) {
			vaciarParticion();
			if (hayEspacioLibrePara(sizeMensaje)) {
				break;
			}
			i++;
			if (i == cantBusquedas) {
				compactarCache();
				i = 0;
			}
		}
	} else {
		while (!hayEspacioLibrePara(sizeMensaje)) {
			vaciarParticion();
			if (hayEspacioLibrePara(sizeMensaje)) {
				break;
			}
			if (!hayMensajes()) {
				limpiarCache();
			}
		}
	}
}

void * cachearConParticionesDinamicas(void * mensaje, int sizeMensaje) {

	asegurarEspacioLibrePara(sizeMensaje);

	if (strcmp("FF",
			config_get_string_value(config, "ALGORITMO_PARTICION_LIBRE")) == 0)
		return usarFirstFit(mensaje, sizeMensaje);
	else
		return usarBestFit(mensaje, sizeMensaje);
}

void *asignarParticion(estructuraMensaje mensaje) {

	void* posicionMemoriaParticion;
	if (strcmp("BS", config_get_string_value(config, "ALGORITMO_MEMORIA")) == 0)
		posicionMemoriaParticion = cachearConBuddySystem(mensaje.mensaje,
				mensaje.sizeMensaje);
	else
		posicionMemoriaParticion = cachearConParticionesDinamicas(
				mensaje.mensaje, mensaje.sizeMensaje);

	return posicionMemoriaParticion;
}

void crearRegistroCache(estructuraMensaje mensaje, void* posInicialMemoria) {

	registroCache * nuevoRegistro = malloc(sizeof(registroCache));

	nuevoRegistro->idMensaje = mensaje.id;
	nuevoRegistro->idCorrelativo = mensaje.idCorrelativo;
	nuevoRegistro->colaMensaje = mensaje.colaMensajeria;
	nuevoRegistro->procesosALosQueSeEnvio = list_create();
	nuevoRegistro->procesosQueConfirmaronRecepcion = list_create();
	nuevoRegistro->sizeMensaje = mensaje.sizeMensaje;
	nuevoRegistro->posicionEnMemoria = posInicialMemoria;

	list_add(registrosDeCache, nuevoRegistro);
}

void cachearMensaje(estructuraMensaje mensaje) {

	if (mensaje.sizeMensaje <= CACHESIZE) {
		void* posicionMemoriaParticion = asignarParticion(mensaje);
		crearRegistroCache(mensaje, posicionMemoriaParticion);
	} else {
		log_info(logger,
				"No se pudo cachear el mensaje con ID %d por ser mas grande que la cache",
				mensaje.id);
	}

}

bool elSuscriptorNoEstaEnLaLista(t_list * lista, uint32_t idSuscriptor) {
	bool esDistinto(void * suscriptor) {
		uint32_t * sus = (uint32_t *) suscriptor;
		return *sus != idSuscriptor;
	}
	return list_all_satisfy(lista, &esDistinto);
}

t_list * getListaDeRegistrosFiltrados(suscriptor * nuevoSuscriptor,
		cola codSuscripcion) {
	bool estaEnLaColaYNoSeConfirmoAun(void * registro) {
		registroCache * reg = (registroCache *) registro;
		return reg->colaMensaje == codSuscripcion
				&& elSuscriptorNoEstaEnLaLista(
						reg->procesosQueConfirmaronRecepcion,
						nuevoSuscriptor->clientID);
	}
	return list_filter(getListaSuscriptoresByNum(codSuscripcion),
			&estaEnLaColaYNoSeConfirmoAun);
}

void enviarMensajes(t_list * mensajesAEnviar, suscriptor * suscriptor) {
	void enviarMensajeAlSuscriptor(void * registro) {
		registroCache * reg = (registroCache *) registro;
		estructuraMensaje mensajeAEnviar;
		int socketSuscriptor, statusEnvio, ack = 0;

		mensajeAEnviar.colaMensajeria = reg->colaMensaje;
		mensajeAEnviar.id = reg->idMensaje;
		mensajeAEnviar.idCorrelativo = reg->idCorrelativo;
		mensajeAEnviar.sizeMensaje = reg->sizeMensaje;

		mensajeAEnviar.mensaje = malloc(mensajeAEnviar.sizeMensaje);
		memcpy(mensajeAEnviar.mensaje, reg->posicionEnMemoria,
				mensajeAEnviar.sizeMensaje);

		socketSuscriptor = getSocketActualDelSuscriptor(suscriptor->clientID,
				reg->colaMensaje);

		statusEnvio = enviarMensajeASuscriptor(mensajeAEnviar,
				socketSuscriptor);

		if (statusEnvio >= 0)
			agregarAListaDeEnviados(mensajeAEnviar.id, suscriptor->clientID);

		recv(socketSuscriptor, &ack, sizeof(uint32_t), MSG_WAITALL);

		if (ack == 1)
			agregarAListaDeConfirmados(mensajeAEnviar.id, suscriptor->clientID);

		free(mensajeAEnviar.mensaje);
	}

	list_iterate(mensajesAEnviar, &enviarMensajeAlSuscriptor);
}

void enviarMensajesCacheados(suscriptor * nuevoSuscriptor, cola codSuscripcion) {
	/*TODO_OLD
	 *	- Filtrar los registros pertenecientes a la cola del codSuscripcion, en las cuales no figure registrado el ID del
	 *    nuevoSuscriptor en la lista de cofirmados. (DONE)
	 *  - Por cada nodo de la lista resultante, llenar una instancia de estructuraMensaje con los datos registrados, y enviar
	 *    el mensaje al suscriptor con la funcion enviarMensajeASuscriptor.
	 *  - Segun el resultado del send, agregar o no a la lista de enviados.
	 *  - Esperar ack del suscriptor.
	 *  - Segun resultado del recv, agregar o no a la lista de confirmados.
	 *
	 */
	t_list * mensajesAEnviar = getListaDeRegistrosFiltrados(nuevoSuscriptor,
			codSuscripcion);
	enviarMensajes(mensajesAEnviar, nuevoSuscriptor);

}

void * posicionInicial(registroCache* regCache) {

	return regCache->posicionEnMemoria;
}

void * posicionFinal(registroCache* regCache) {
	/* TODO
	 * - Calcular posicion final (inicial + tamanio)
	 */
	return 0;
}

int obtenerTamanioParticion(registroCache* regCache) {
	/* TODO
	 * - Calcular tamanio y devolver int
	 */
	return 0;
}

char* obtenerLRU(registroCache* regCache) {
	/* TODO
	 * - Sacar LRU
	 */
	return "X";
}

time_t getTime() {
	time_t currentTime = time(NULL);
	return currentTime;
}

char* timeToString(time_t time) {
	return ctime(&time);
}

void dumpCache() {
	/* TODO - DONE
	 * - Crear archivo o abrir archivo existente
	 * - Escribir titulo ("Dump: <timestamp>")
	 * - Tomar cada registro de "registrosDeCache"
	 * - Escribir datos de cada registro en el archivo
	 * - Cerrar
	 */
	log_info(logger, "Ejecutando dump de la cache...");

	FILE* cacheDumpFile = fopen("dumpCacheBroker.txt", "a"); //mode = a (Append en el final. Si no existe lo crea)
	int i = 0;

	if (cacheDumpFile == NULL) {
		log_error(logger,
				"Ocurrio un error con el archivo de dump de la cache");
		return;
	}

	fprintf(cacheDumpFile,
			"------------------------------------------------------------------------------------------------------------------\n\n");
	fprintf(cacheDumpFile, "Dump: %s \n\n", timeToString(getTime()));

	void escribirRegistro(void* registro) {
		registroCache * regCache = (registroCache*) registro;
		//fprintf(cacheDumpFile, "%-15s %-15s %-15s %-15s \n", "A", "B", "C", "D");
		i++;
		fprintf(cacheDumpFile, "Particion #%d : %-15s - %-15s ", i,
				(char *) posicionInicial(regCache->posicionEnMemoria),
				(char *) posicionFinal(regCache));
		fprintf(cacheDumpFile, "[%s]     ", "X");
		fprintf(cacheDumpFile, "Size: %-15d b",
				obtenerTamanioParticion(regCache));
		fprintf(cacheDumpFile, "LRU: %-15s", obtenerLRU(regCache));
		fprintf(cacheDumpFile, "Cola: %-15s",
				getCodeStringByNum(regCache->colaMensaje));
		fprintf(cacheDumpFile, "ID: %-15d", regCache->idMensaje);

		fprintf(cacheDumpFile, "\n");
	}

	list_iterate(registrosDeCache, escribirRegistro);

	fclose(cacheDumpFile);

}

