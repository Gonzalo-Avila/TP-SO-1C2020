#include "Broker.h"

//Almacenamiento en cache
//--------------------------------------------------------------------------
void crearRegistroInicial(t_list * listaDeRegistrosDestino) {

	registroParticion * regParticion = malloc(sizeof(registroParticion));
	regParticion->nroParticion = 0;
	regParticion->posInicialFisica = cacheBroker;
	regParticion->posInicialLogica = 0;
	regParticion->tamanioParticion = CACHESIZE;
	regParticion->estado = LIBRE;
	regParticion->idMensaje = -1;

	list_add(listaDeRegistrosDestino, regParticion);

}

int BSCacheSize(int size) {
	int sizeAux = 1;
	while (sizeAux < size) {

		if (sizeAux * 2 > size)
			return sizeAux;
		else
			sizeAux = sizeAux * 2;

	}
	return sizeAux;
}

int adaptarCacheSize(int size) {
	if (algoritmoMemoria == BUDDY_SYSTEM)
		size = BSCacheSize(size);
	return size;
}

void setearAlgoritmos() {

	if (strcmp(config_get_string_value(config, "ALGORITMO_MEMORIA"), "BS") == 0)
		algoritmoMemoria = BUDDY_SYSTEM;
	else
		algoritmoMemoria = PARTICIONES_DINAMICAS;

	if (strcmp(config_get_string_value(config, "ALGORITMO_REEMPLAZO"), "LRU")
			== 0)
		algoritmoReemplazo = LRU;
	else
		algoritmoReemplazo = FIFO;

	if (strcmp(config_get_string_value(config, "ALGORITMO_PARTICION_LIBRE"),
			"FF") == 0)
		algoritmoParticionLibre = FIRST_FIT;
	else
		algoritmoParticionLibre = BEST_FIT;
}

void inicializarCache() {

	setearAlgoritmos();

	CACHESIZE = adaptarCacheSize(
			config_get_int_value(config, "TAMANO_MEMORIA"));
	minimoTamanioParticion = config_get_int_value(config,
			"TAMANO_MINIMO_PARTICION");
	cacheBroker = malloc(CACHESIZE);
	crearRegistroInicial(registrosDeParticiones);

}

int XOR(int a, int b) {
	return a ^ b;
}

void crearNuevoBuddy(t_list * listaDeParticiones, registroParticion * registro,
		int tamanioMensaje) {

	registro->tamanioParticion = registro->tamanioParticion / 2;

	registroParticion * registroNuevo = malloc(sizeof(registroParticion));
	registroNuevo->nroParticion = registro->nroParticion + 1; //Igualmente se va a reasignar despues
	registroNuevo->idMensaje = -1;
	registroNuevo->posInicialLogica = registro->posInicialLogica
			+ registro->tamanioParticion;
	registroNuevo->posInicialFisica = registro->posInicialFisica
			+ registro->tamanioParticion;
	registroNuevo->estado = LIBRE;
	registroNuevo->tamanioParticion = registro->tamanioParticion;
	list_add_in_index(listaDeParticiones, registro->nroParticion + 1,
			registroNuevo);
}

registroParticion * obtenerBuddy(registroParticion * particionLiberada) {
	bool esContiguaAnterior(void * particion) {
		registroParticion * reg = (registroParticion *) particion;

		return reg->nroParticion == particionLiberada->nroParticion - 1;
	}
	bool esContiguaPosterior(void * particion) {
		registroParticion * reg = (registroParticion *) particion;

		return reg->nroParticion == particionLiberada->nroParticion + 1;
	}

	registroParticion * posibleBuddy1 = list_find(registrosDeParticiones,
			(void *) esContiguaAnterior);
	registroParticion * posibleBuddy2 = list_find(registrosDeParticiones,
			(void *) esContiguaPosterior);

	if (particionLiberada->nroParticion != 0
			&& posibleBuddy1->tamanioParticion
					== particionLiberada->tamanioParticion
			&& posibleBuddy1->posInicialLogica
					== XOR(particionLiberada->posInicialLogica,
							posibleBuddy1->tamanioParticion)
			&& particionLiberada->posInicialLogica
					== XOR(posibleBuddy1->posInicialLogica,
							particionLiberada->tamanioParticion)) {
		return posibleBuddy1;
	}

	return posibleBuddy2;
}
void consolidar(registroParticion * particionLiberada) {
	registroParticion * buddy = obtenerBuddy(particionLiberada);
	if (buddy->estado == LIBRE) {
		//TODO - Mergear los buddys

	}

}

void asegurarQueHayaEspacio(int sizeMensaje) {

	int cantBusquedas = config_get_int_value(config, "FRECUENCIA_COMPACTACION");
	if (cantBusquedas != -1) {
		int i = 0;
		while (!hayEspacioLibrePara(sizeMensaje)) {
			registroParticion * particionLiberada = vaciarParticion();
			if (list_size(registrosDeParticiones) > 1)
				consolidar(particionLiberada);
			if (hayEspacioLibrePara(sizeMensaje)) {
				break;
			}
			i++;
			if (i == cantBusquedas) {
				compactarCacheSegunBS();
				i = 0;
			}
		}
	} else {
		while (!hayEspacioLibrePara(sizeMensaje)) {
			registroParticion * particionLiberada = vaciarParticion();
			if (list_size(registrosDeParticiones) > 1)
				consolidar(particionLiberada);
		}
	}
}
void * cachearConBuddySystem(estructuraMensaje mensaje) {
	// TODO
	asegurarQueHayaEspacio(mensaje.sizeMensaje);

	bool estaVaciaYAlcanza(void * particion) {
		registroParticion * reg = (registroParticion *) particion;
		return reg->estado == LIBRE
				&& reg->tamanioParticion >= mensaje.sizeMensaje;
	}

	t_list * particionesValidas = list_filter(registrosDeParticiones,
			(void *) estaVaciaYAlcanza);
	t_list * particionesOrdenadasPorTamanio = list_sorted(particionesValidas,
			(void *) compararPorMenorTamanio);
	registroParticion * registro = (registroParticion *) list_get(
			particionesOrdenadasPorTamanio, 0);

	while (registro->tamanioParticion >= 2 * mensaje.sizeMensaje
			&& registro->tamanioParticion > minimoTamanioParticion) {
		crearNuevoBuddy(registrosDeParticiones, registro, mensaje.sizeMensaje);
	}

	memcpy(registro->posInicialFisica, mensaje.mensaje, mensaje.sizeMensaje);
	registro->estado = OCUPADO;
	registro->idMensaje = mensaje.id;
	registro->tiempoArribo = time(NULL);
	registro->tiempoUltimoUso = time(NULL);

	reasignarNumerosDeParticion(registrosDeParticiones);

	return registro->posInicialFisica;

}

void * usarBestFit(estructuraMensaje mensaje) {
	/* TODO - Done
	 * -
	 * - Filtrar en registrosDeCache TODOS los que:
	 *		- Estado = LIBRE
	 *		- Tamanio >= sizeMensaje
	 *
	 * - Tomar el de menor "tamanio"
	 * - Copiar "mensaje" en posInicialFisica del registro elegido
	 * - Crear registro nuevo con datos de mensaje
	 * - Crear registro nuevo con particion libre restante
	 * - Reasignar nroParticion a todos los registros en "registrosDeCache"
	 */
	bool estaVaciaYAlcanza(void * particion) {
		registroParticion * reg = (registroParticion *) particion;
		return reg->estado == LIBRE
				&& reg->tamanioParticion >= mensaje.sizeMensaje;
	}

	t_list * particionesValidas = list_filter(registrosDeParticiones,
			(void *) estaVaciaYAlcanza);
	t_list * particionesOrdenadasPorTamanio = list_sorted(particionesValidas,
			(void *) compararPorMenorTamanio);
	registroParticion * registro = (registroParticion *) list_get(
			particionesOrdenadasPorTamanio, 0);

	if (registro->tamanioParticion > mensaje.sizeMensaje) {
		aniadirNuevoRegistroALista(registrosDeParticiones, registro,
				mensaje.sizeMensaje);
		reasignarNumerosDeParticion(registrosDeParticiones);
	}

	memcpy(registro->posInicialFisica, mensaje.mensaje, mensaje.sizeMensaje);
	registro->estado = OCUPADO;
	registro->idMensaje = mensaje.id;
	registro->tamanioParticion = mensaje.sizeMensaje;
	registro->tiempoArribo = time(NULL);
	registro->tiempoUltimoUso = time(NULL);

	return registro->posInicialFisica;
}

void reasignarNumerosDeParticion(t_list * listaAReasignar) {
	int numero = 0;
	void asignarNumero(void * particion) {
		registroParticion * registro = (registroParticion *) particion;
		registro->nroParticion = numero;
		numero++;
	}
	list_iterate(listaAReasignar, (void *) asignarNumero);
}

void aniadirNuevoRegistroALista(t_list * listaDeRegistros,
		registroParticion * registroAnterior, int sizeMensajeRecibido) {
	registroParticion * registroNuevo = malloc(sizeof(registroParticion));
	registroNuevo->nroParticion = registroAnterior->nroParticion + 1; //Igualmente se va a reasignar despues
	registroNuevo->idMensaje = -1;
	registroNuevo->posInicialLogica = registroAnterior->posInicialLogica
			+ sizeMensajeRecibido;
	registroNuevo->posInicialFisica = registroAnterior->posInicialFisica
			+ sizeMensajeRecibido;
	registroNuevo->estado = LIBRE;
	registroNuevo->tamanioParticion = registroAnterior->tamanioParticion
			- sizeMensajeRecibido;
	list_add_in_index(listaDeRegistros, registroAnterior->nroParticion + 1,
			registroNuevo);
}
void * usarFirstFit(estructuraMensaje mensaje) {
	/* TODO - DONE
	 * -
	 * - Buscar en registrosDeCache el PRIMERO que:
	 *		- Estado = LIBRE
	 *		- Tamanio >= sizeMensaje
	 *
	 * - Copiar "mensaje" en posInicialFisica del registro elegido
	 * - Crear registro nuevo con datos de mensaje
	 * - Crear registro nuevo con particion libre restante
	 * - Reasignar nroParticion a todos los registros en "registrosDeCache"
	 */
	bool estaVaciaYAlcanza(void * particion) {
		registroParticion * registro = (registroParticion *) particion;
		return registro->estado == LIBRE
				&& registro->tamanioParticion >= mensaje.sizeMensaje;
	}
	registroParticion * registro = (registroParticion *) list_find(
			registrosDeParticiones, (void *) estaVaciaYAlcanza);
	if (registro->tamanioParticion > mensaje.sizeMensaje) {
		aniadirNuevoRegistroALista(registrosDeParticiones, registro,
				mensaje.sizeMensaje);
		reasignarNumerosDeParticion(registrosDeParticiones);
	}

	memcpy(registro->posInicialFisica, mensaje.mensaje, mensaje.sizeMensaje);
	registro->estado = OCUPADO;
	registro->idMensaje = mensaje.id;
	registro->tamanioParticion = mensaje.sizeMensaje;
	registro->tiempoArribo = time(NULL);
	registro->tiempoUltimoUso = time(NULL);

	return registro->posInicialFisica;

}

bool hayEspacioLibrePara(int sizeMensaje) {
	bool estaVaciaYAlcanza(void * particion) {
		registroParticion * registro = (registroParticion *) particion;
		return registro->estado == LIBRE
				&& registro->tamanioParticion >= sizeMensaje;
	}
	return list_any_satisfy(registrosDeParticiones, (void *) estaVaciaYAlcanza);
}

bool compararPorMenorTamanio(void * particion1, void * particion2) {
	registroParticion * registro1 = (registroParticion *) particion1;
	registroParticion * registro2 = (registroParticion *) particion2;

	return registro1->tamanioParticion < registro2->tamanioParticion;
}

bool compararPorMayorTamanio(void * particion1, void * particion2) {
	registroParticion * registro1 = (registroParticion *) particion1;
	registroParticion * registro2 = (registroParticion *) particion2;

	return registro1->tamanioParticion > registro2->tamanioParticion;
}

bool compararPorFIFO(void * particion1, void * particion2) {
	registroParticion * registro1 = (registroParticion *) particion1;
	registroParticion * registro2 = (registroParticion *) particion2;

	return registro1->tiempoArribo < registro2->tiempoArribo;
}
bool compararPorLRU(void * particion1, void * particion2) {
	registroParticion * registro1 = (registroParticion *) particion1;
	registroParticion * registro2 = (registroParticion *) particion2;

	return registro1->tiempoUltimoUso < registro2->tiempoUltimoUso;
}

void eliminarRegistroDeCache(int IDMensaje) {
	bool coincideID(void * registro) {
		registroCache * reg = (registroCache *) registro;
		return reg->idMensaje == IDMensaje;
	}
	list_remove_by_condition(registrosDeCache, (void *) coincideID);

}

registroParticion * liberarSegunFIFO() {
	t_list * particionesOcupadas = list_filter(registrosDeParticiones,
			(void *) estaOcupado);
	t_list * particionesOrdenadasPorFIFO = list_sorted(particionesOcupadas,
			(void *) compararPorFIFO);
	registroParticion * particionALiberar = (registroParticion *) list_get(
			particionesOrdenadasPorFIFO, 0);
	particionALiberar->estado = LIBRE;
	eliminarRegistroDeCache(particionALiberar->idMensaje);
	particionALiberar->idMensaje = -1;
	return particionALiberar;
}
registroParticion * liberarSegunLRU() {
	t_list * particionesOcupadas = list_filter(registrosDeParticiones,
			(void *) estaOcupado);
	t_list * particionesOrdenadasPorLRU = list_sorted(particionesOcupadas,
			(void *) compararPorLRU);
	registroParticion * particionALiberar = (registroParticion *) list_get(
			particionesOrdenadasPorLRU, 0);
	particionALiberar->estado = LIBRE;
	eliminarRegistroDeCache(particionALiberar->idMensaje);
	particionALiberar->idMensaje = -1;
	return particionALiberar;
}
registroParticion * vaciarParticion() {
	//Libera y retorna el registro liberado (nos sirve para buddy system)
	if (algoritmoReemplazo == FIFO)
		return liberarSegunFIFO();
	else
		return liberarSegunLRU();
}

void compactarCacheSegunPD() {
	t_list * listaAuxiliar = list_create();
	crearRegistroInicial(listaAuxiliar);

	void guardarEnListaAuxiliar(void * registroOcupado) {
		registroParticion * registroAMover =
				(registroParticion *) registroOcupado;

		bool estaVaciaYAlcanza(void * particion) {
			registroParticion * registro = (registroParticion *) particion;
			return registro->estado == LIBRE
					&& registro->tamanioParticion
							>= registroAMover->tamanioParticion;
		}

		registroParticion * registro = (registroParticion *) list_find(
				listaAuxiliar, (void *) estaVaciaYAlcanza);
		if (registro->tamanioParticion > registroAMover->tamanioParticion) {
			aniadirNuevoRegistroALista(listaAuxiliar, registro,
					registroAMover->tamanioParticion);
		}

		memcpy(registro->posInicialFisica, registroAMover->posInicialFisica,
				registroAMover->tamanioParticion);
		registro->estado = OCUPADO;
		registro->idMensaje = registroAMover->idMensaje;
		registro->tamanioParticion = registroAMover->tamanioParticion;
		registro->tiempoArribo = registroAMover->tiempoArribo;
		registro->tiempoUltimoUso = registroAMover->tiempoUltimoUso;

	}

	t_list * registrosOcupados = list_filter(registrosDeParticiones,
			(void *) estaOcupado);
	list_iterate(registrosOcupados, (void *) guardarEnListaAuxiliar);

	reasignarNumerosDeParticion(registrosOcupados);

	list_clean(registrosDeParticiones);
	registrosDeParticiones = listaAuxiliar;

}

void limpiarCache() {
	list_clean(registrosDeParticiones);
	crearRegistroInicial(registrosDeParticiones);
}

void compactarCacheSegunBuddySystem() {
	/*	TODO
	 *
	 *
	 */

	t_list * listaAuxiliar = list_create();
	crearRegistroInicial(listaAuxiliar);

	void guardarEnListaAuxiliar(void * registroOcupado) {

		registroParticion * regOcupado = (registroParticion *) registroOcupado;

		bool estaVaciaYAlcanza(void * particion) {
			registroParticion * reg = (registroParticion *) particion;
			return reg->estado == LIBRE
					&& reg->tamanioParticion >= regOcupado->tamanioParticion;
		}

		t_list * particionesValidas = list_filter(listaAuxiliar,
				(void *) estaVaciaYAlcanza);

		t_list * particionesOrdenadasPorTamanio = list_sorted(
				particionesValidas, (void *) compararPorMenorTamanio);

		registroParticion * registro = (registroParticion *) list_get(
				particionesOrdenadasPorTamanio, 0);

		while (registro->tamanioParticion >= 2 * regOcupado->tamanioParticion
				&& registro->tamanioParticion > minimoTamanioParticion) {
			crearNuevoBuddy(listaAuxiliar, registro,
					regOcupado->tamanioParticion);
		}

		memcpy(registro->posInicialFisica, regOcupado->posInicialFisica,
				regOcupado->tamanioParticion);
		registro->estado = OCUPADO;
		registro->idMensaje = regOcupado->idMensaje;
		registro->tiempoArribo = regOcupado->tiempoArribo;
		registro->tiempoUltimoUso = regOcupado->tiempoUltimoUso;

		reasignarNumerosDeParticion(listaAuxiliar);

	}


	t_list * particionesOcupadas = list_filter(registrosDeParticiones,
			(void *) estaOcupado);
	//t_list * particionesOcupadasOrdenadasPorTamanio = list_sorted(particionesOcupadas, (void *) compararPorMayorTamanio);

	list_iterate(particionesOcupadas, guardarEnListaAuxiliar);
}

bool estaOcupado(void* regParticion) {
	registroParticion* regPart = (registroParticion*) regParticion;
	return regPart->estado == OCUPADO;
}
bool hayMensajes() {
	return list_any_satisfy(registrosDeParticiones, (void *) estaOcupado);
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
				compactarCacheSegunPD();
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

void * cachearConParticionesDinamicas(estructuraMensaje mensaje) {

	asegurarEspacioLibrePara(mensaje.sizeMensaje);

	if (algoritmoParticionLibre == FIRST_FIT)
		return usarFirstFit(mensaje);
	else
		return usarBestFit(mensaje);
}

void *asignarParticion(estructuraMensaje mensaje) {

	void* posicionMemoriaParticion;
	if (algoritmoMemoria == BUDDY_SYSTEM)
		posicionMemoriaParticion = cachearConBuddySystem(mensaje);
	else
		posicionMemoriaParticion = cachearConParticionesDinamicas(mensaje); //DONE

	return posicionMemoriaParticion;
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
//--------------------------------------------------------------------------

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

//Envío de mensajes cacheados a nuevos suscriptores
//--------------------------------------------------------------------------
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
//--------------------------------------------------------------------------

//Impresión de dump de cache
//--------------------------------------------------------------------------

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
//--------------------------------------------------------------------------

