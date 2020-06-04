#include "Broker.h"

//Almacenamiento en cache
//--------------------------------------------------------------------------
void crearRegistroInicial(t_list * listaDeRegistrosDestino) {

	registroParticion * regParticion = malloc(sizeof(registroParticion));
	regParticion->nroParticion = 0;
	regParticion->posInicialFisica = cacheBroker;
	regParticion->posInicialLogica = 0;
	regParticion->tamanioParticion = CACHESIZE;
	regParticion->tamanioMensaje = 0;
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
	reasignarNumerosDeParticion(listaDeParticiones);
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
void consolidar(registroParticion * particionLiberada, t_list * registros) {

	registroParticion * buddy = obtenerBuddy(particionLiberada);

	while (buddy->estado == LIBRE) {
		if (particionLiberada->posInicialLogica < buddy->posInicialLogica) {
			bool coincideID(void * registro) {
				registroParticion * reg = (registroParticion *) registro;
				return reg->nroParticion == particionLiberada->nroParticion + 1;
			}
			particionLiberada->tamanioParticion = 2
					* particionLiberada->tamanioParticion;
			particionLiberada->idMensaje = -1;
			particionLiberada->tamanioMensaje = 0;
			list_remove_by_condition(registros, (void *) coincideID);
			reasignarNumerosDeParticion(registros);
			buddy = obtenerBuddy(particionLiberada);
		} else {
			bool coincideID(void * registro) {
				registroParticion * reg = (registroParticion *) registro;
				return reg->nroParticion == buddy->nroParticion + 1;
			}
			buddy->tamanioParticion = 2 * buddy->tamanioParticion;
			buddy->idMensaje = -1;
			buddy->tamanioMensaje = 0;
			list_remove_by_condition(registros, (void *) coincideID);
			reasignarNumerosDeParticion(registros);
			particionLiberada = buddy;
			buddy = obtenerBuddy(particionLiberada);
		}

	}

}

void asegurarQueHayaEspacio(int sizeMensaje) {

	int cantBusquedas = config_get_int_value(config, "FRECUENCIA_COMPACTACION");
	if (cantBusquedas != -1) {
		int i = 0;
		while (!hayEspacioLibrePara(sizeMensaje)) {
			registroParticion * particionLiberada = vaciarParticion();
			if (list_size(registrosDeParticiones) > 1)
				consolidar(particionLiberada, registrosDeParticiones);
			if (hayEspacioLibrePara(sizeMensaje)) {
				break;
			}
			i++;
			if (i == cantBusquedas) {
				compactarCacheSegunBuddySystem();
				i = 0;
			}
		}
	} else {
		while (!hayEspacioLibrePara(sizeMensaje)) {
			registroParticion * particionLiberada = vaciarParticion();
			if (list_size(registrosDeParticiones) > 1)
				consolidar(particionLiberada, registrosDeParticiones);
		}
	}
}
void * cachearConBuddySystem(estructuraMensaje mensaje) {

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

	//reasignarNumerosDeParticion(registrosDeParticiones);

	return registro->posInicialFisica;

}

void * usarBestFit(estructuraMensaje mensaje) {

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
	t_list * particionesOcupadas = list_filter(registrosDeParticiones,(void *) estaOcupado);
	t_list * particionesOrdenadasPorFIFO = list_sorted(particionesOcupadas,(void *) compararPorFIFO);
	registroParticion * particionALiberar = (registroParticion *) list_get(particionesOrdenadasPorFIFO, 0);
	particionALiberar->estado = LIBRE;
	eliminarRegistroDeCache(particionALiberar->idMensaje);
	particionALiberar->idMensaje = -1;
	particionALiberar->tamanioMensaje=0;
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
	particionALiberar->tamanioMensaje=0;
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
	bool estaCompactada = false;
	bool seMovioParticion = false;

	while (!estaCompactada) {
		for (int i = 0; i < list_size(registrosDeParticiones); i++) {
			registroParticion* regParcial = list_get(registrosDeParticiones, i);
			if (regParcial->estado == OCUPADO) {
				for (int j = 0; j < i; j++) {
					registroParticion* regAEvaluar = list_get(
							registrosDeParticiones, j);
					if (regAEvaluar->estado == LIBRE
							&& regAEvaluar->tamanioParticion
									>= regParcial->tamanioParticion) {

						while (regAEvaluar->tamanioParticion
								>= 2 * regParcial->tamanioMensaje
								&& regAEvaluar->tamanioParticion
										> minimoTamanioParticion) {
							crearNuevoBuddy(registrosDeParticiones, regAEvaluar,
									regParcial->tamanioMensaje);
						}

						memcpy(regAEvaluar->posInicialFisica,
								regParcial->posInicialFisica,
								regParcial->tamanioParticion);

						regAEvaluar->estado = OCUPADO;
						regAEvaluar->idMensaje = regParcial->idMensaje;
						regAEvaluar->tamanioParticion =regParcial->tamanioParticion;
						regAEvaluar->tiempoArribo = regParcial->tiempoArribo;
						regAEvaluar->tiempoUltimoUso =regParcial->tiempoUltimoUso;
						regAEvaluar->tamanioMensaje=regParcial->tamanioMensaje;

						regParcial->estado = LIBRE;
						regParcial->idMensaje=-1;
						regParcial->tamanioMensaje=0;
						consolidar(regParcial, registrosDeParticiones);
						seMovioParticion = true;
						break;
					}
				} //for
				if (seMovioParticion) {
					seMovioParticion = false;
					estaCompactada = false;
					break;
				}
				estaCompactada = true;
			}
		}

	}
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
		asignarParticion(mensaje);
		crearRegistroCache(mensaje);
	} else {
		log_info(logger,
				"No se pudo cachear el mensaje con ID %d por ser mas grande que la cache",
				mensaje.id);
	}

}
//--------------------------------------------------------------------------

void crearRegistroCache(estructuraMensaje mensaje) {

	registroCache * nuevoRegistro = malloc(sizeof(registroCache));

	nuevoRegistro->idMensaje = mensaje.id;
	nuevoRegistro->idCorrelativo = mensaje.idCorrelativo;
	nuevoRegistro->colaMensaje = mensaje.colaMensajeria;
	nuevoRegistro->procesosALosQueSeEnvio = list_create();
	nuevoRegistro->procesosQueConfirmaronRecepcion = list_create();
	nuevoRegistro->sizeMensaje = mensaje.sizeMensaje;

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
	return list_filter(registrosDeCache,(void *)estaEnLaColaYNoSeConfirmoAun);
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

		bool coincideIDMsg(void * regParticion){
				registroParticion * regP = (registroParticion *) regParticion;
				return regP->idMensaje==reg->idMensaje;
		}
		registroParticion * regPart = (registroParticion *)list_find(registrosDeParticiones,(void*)coincideIDMsg);
		void * posicionEnMemoria = regPart->posInicialFisica;


		memcpy(mensajeAEnviar.mensaje, posicionEnMemoria,
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
	/*TODO
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

	enviarMensajes(mensajesAEnviar, nuevoSuscriptor);
}
//--------------------------------------------------------------------------

//Impresión de dump de cache
//--------------------------------------------------------------------------

/*void * posicionInicial(registroCache* regCache) {

	return regCache->posicionEnMemoria;
}*/

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

cola obtenerCola(registroParticion * registroParticion){
	bool coincideIDMsg(void * registro ){
		registroCache * regCache = (registroCache *) registro;
		return regCache->idMensaje==registroParticion->idMensaje;
	}
	registroCache * registroQueCoincide = (registroCache *) list_find(registrosDeCache,(void *) coincideIDMsg);
	return registroQueCoincide->colaMensaje;
}

char getEstadoParticion(estadoParticion estado){
	char a='X';
	if(estado==LIBRE)
		a='L';

	return a;
}

void dumpCache() {

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
		registroParticion * regParticion = (registroParticion*) registro;
		//fprintf(cacheDumpFile, "%-15s %-15s %-15s %-15s \n", "A", "B", "C", "D");
		i++;
		fprintf(cacheDumpFile, "Particion #%d : %-12p %s %-12p ", i,
				regParticion->posInicialFisica, "-",
				regParticion->posInicialFisica+regParticion->tamanioParticion-1);
		fprintf(cacheDumpFile, "[%c]     ", getEstadoParticion(regParticion->estado));
		fprintf(cacheDumpFile, "Size: %d b    ",regParticion->tamanioParticion);

		if(regParticion->estado==OCUPADO){
			fprintf(cacheDumpFile, "LRU: %s  ", timeToString(regParticion->tiempoUltimoUso));
			fprintf(cacheDumpFile, "Cola: %-16s",getCodeStringByNum(obtenerCola(regParticion)));
			fprintf(cacheDumpFile, "ID: %-15d", regParticion->idMensaje);
		}

		fprintf(cacheDumpFile, "\n");
	}

	list_iterate(registrosDeParticiones, (void*) escribirRegistro);

	fprintf(cacheDumpFile,
				"------------------------------------------------------------------------------------------------------------------\n\n");

	fclose(cacheDumpFile);
}
//--------------------------------------------------------------------------

