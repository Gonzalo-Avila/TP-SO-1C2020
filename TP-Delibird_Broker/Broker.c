#include "Broker.h"
#include <string.h>
//#include <stdbool.h>

//----------------------- [CACHE] -------------------------//

/* Chequea la disponibilidad de tantas posiciones contiguas como sea el tamaño del mensaje. Si en el proceso encuentra alguna
 * ocupada retorna 0 (no alcanza el espacio), si termina sin encontrar ninguna ocupada retorna 1 (espacio suficiente).
 */
int chequearSiAlcanza(int sizeMensaje, void * posicionActual,
		int memoriaRecorrida) {
	for (int offset = 1; offset < sizeMensaje; offset++) {
		if (memoriaRecorrida == CACHESIZE
				|| *(char *) (posicionActual + offset) == 'f') //LAZY EVALUATION POWA
			return 0;
		memoriaRecorrida++;
	}
	return 1;

}
/* Busca secuencialmente algun byte marcado como libre, cuando lo encuentra llama a otra funcion para ver si alcanza el
 * espacio para guardar el mensaje
 *
 */
void * buscarEspacio(int sizeMensaje, void *posicionInicial) {
	int espacioSuficiente = 0;
	int offset = 0;
	while (!espacioSuficiente) {
		if (offset == CACHESIZE)
			return NULL;

		if (*(char *) (posicionInicial + offset) == 'f') {
			espacioSuficiente = chequearSiAlcanza(sizeMensaje,
					posicionInicial + offset, offset);
		}
		offset++;

	}
	return posicionInicial + offset;
}

/* Partiendo desde una posición ocupada, cuenta todas las posiciónes contiguas llenas para determinar el tamaño del mensaje
 *
 */
int tamanioDelMensaje(int offset, int *cacheExcedida) {
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

/* Partiendo desde una posición vacia, busca secuencialmente hasta encontrar una posición ocupada
 *
 */
void * buscarProximoMensaje(int offset, int * tamanio, int *cacheExcedida) {
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

/* Libera los "huecos" de la memoria cache, haciendo que todos los mensajes esten pegados uno detras del otro.
 *
 */

void compactarMemoria() {
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

//A desarrollar
void eliminarMensaje() {
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
void cachearMensaje(void * mensaje, int sizeMensaje) {

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
}

void enviarMensajesCacheados(int socketSuscriptor, int codSuscripcion) {
	//TODO

}

void* deserializarPayload(int socketSuscriptor) {

	log_debug(logger, "deserializarPayload");
	int msjLength;
	recv(socketSuscriptor, &msjLength, sizeof(int), MSG_WAITALL);
	log_debug(logger, "%d", msjLength);
	void* mensaje = malloc(msjLength);
	recv(socketSuscriptor, mensaje, msjLength, MSG_WAITALL);
	return mensaje;
}

//----------------------- [GETTERS] -------------------------//

char* getCodeStringByNum(int nro) {
	char* codigo[6] = { "NEW_POKEMON", "APPEARED_POKEMON", "CATCH_POKEMON",
			"CAUGHT_POKEMON", "GET_POKEMON", "LOCALIZED_POKEMON" };
	return codigo[nro];
}

t_list * getColaByNum(int nro) {
	t_list* lista[6] = { NEW_POKEMON, APPEARED_POKEMON, CATCH_POKEMON,
			CAUGHT_POKEMON, GET_POKEMON, LOCALIZED_POKEMON };
	return lista[nro];
}

t_list* getListaSuscriptoresByNum(opCode nro) {
	t_list* lista[6] = { suscriptoresNEW, suscriptoresAPP, suscriptoresCAT,
			suscriptoresCAU, suscriptoresGET, suscriptoresLOC };
	return lista[nro];
}

/*void *getEnviarAColaByNum(int num) {
 void (*codigo[6])(estructuraMensaje nodoMsj) =
 {	enviarMensajeASuscriptorNEW,
 enviarMensajeASuscriptorAPP,
 enviarMensajeASuscriptorCAT,
 enviarMensajeASuscriptorCAU,
 enviarMensajeASuscriptorGET,
 enviarMensajeASuscriptorLOC };
 return codigo[num];
 }*/

long getID() {
	return globalID++;
}

//Edit Gonzalo - 19/04
//----------

// revisar si usamos una variable global o size de la lista + 1, para no repetir el ID nunca durante la ejecución
/* Chequea los index de la lista global de IDs, a partir de uno, y cuando encuentra alguno libre se carga ahi
 *
 */

t_list * generarListaDeSuscriptoresActuales(cola tipoCola) {
	t_list * listaGenerada = list_create();
	t_list * suscriptoresDeLaCola = getListaSuscriptoresByNum(tipoCola);
	int* socketSus;

	for (int index = 0; index < list_size(suscriptoresDeLaCola); index++) {
		//FIXME
		socketSus = (int *) list_get(suscriptoresDeLaCola, index);
		list_add(listaGenerada, socketSus);
	}
	return listaGenerada;
}

void imprimirEstructuraDeDatos(estructuraMensaje mensaje) {
	log_info(logger, "[NUEVO MENSAJE RECIBIDO]");
	log_info(logger, "ID: %d", mensaje.id);
	log_info(logger, "ID correlativo: %d", mensaje.idCorrelativo);
	log_info(logger, "Tamaño de mensaje: %d", mensaje.sizeMensaje);
	//DONE
	//Imprimir lista de suscriptores y datos del mensaje
}

estructuraMensaje * generarNodo(estructuraMensaje mensaje) {

	estructuraMensaje * nodo = malloc(sizeof(estructuraMensaje));
	nodo->mensaje = malloc(mensaje.sizeMensaje);

	nodo->id = mensaje.id;
	nodo->idCorrelativo = mensaje.idCorrelativo;
	nodo->estado = mensaje.estado;
	nodo->sizeMensaje = mensaje.sizeMensaje;
	nodo->mensaje = mensaje.mensaje;
	nodo->colaMensajeria = mensaje.colaMensajeria;
	nodo->socketSuscriptor = mensaje.socketSuscriptor;
	return nodo;
}
int agregarMensajeACola(int socketEmisor, cola tipoCola, int idCorrelativo) {

	/* TODO
	 * Al usarse semaforos, ya no tiene sentido hacer una copia de los suscriptores actuales. Quitar.
	 */
	t_list* suscriptoresActuales = generarListaDeSuscriptoresActuales(tipoCola);

	estructuraMensaje mensajeNuevo;

	recv(socketEmisor, &mensajeNuevo.sizeMensaje, sizeof(int), MSG_WAITALL);
	mensajeNuevo.mensaje = malloc(mensajeNuevo.sizeMensaje);
	recv(socketEmisor, mensajeNuevo.mensaje, mensajeNuevo.sizeMensaje, MSG_WAITALL);

	int id = getID();

	mensajeNuevo.id = id;
	mensajeNuevo.idCorrelativo = idCorrelativo;
	mensajeNuevo.estado = ESTADO_NUEVO;
	mensajeNuevo.colaMensajeria = tipoCola;

	imprimirEstructuraDeDatos(mensajeNuevo);

	sem_wait(&mutexColas);
	for (int i = 0; i < list_size(suscriptoresActuales); i++) {

		mensajeNuevo.socketSuscriptor = * (int *) list_get(suscriptoresActuales, i);
		list_add(getColaByNum(tipoCola), generarNodo(mensajeNuevo));
		//if (idCorrelativo != -1) cachearMensaje(mensajeNuevo.mensaje, mensajeNuevo.sizeMensaje);
	}
	sem_post(&mutexColas);
	sem_post(&habilitarEnvio);

	return id;
}

//----------

//
// - Crear estructura de mensaje (Done)
// - Crear ID (Done)
// - Sacar "foto" de lista de suscriptores actuales (int checklistSuscriptor[2];) (Done)
// - Guardar payload (mensaje + size) (Done)
// - Decidir si tiene idCorrelativo [SI (idC = <valor>) || NO idCorrelativo (idC = -1)} (Done)
// - Meter esta estructura en cola (Done)
// - Enviar ID creado al publicador (Done)
// - Guardar en cache (Done)
// |- Averiguar qué guardar en la cache, porque el enunciado dice que solo se puede guardar el payload

// Edit Gonzalo - 19/04
//-------------------------
//-------------------------

void atenderMensaje(int socketEmisor, cola tipoCola) {
	int idMensaje;
	uint32_t idCorrelativo;
	recv(socketEmisor, &idCorrelativo, sizeof(uint32_t), MSG_WAITALL);

	if (tipoCola >= 0 && tipoCola <= 5) {
		idMensaje = agregarMensajeACola(socketEmisor, tipoCola, -1);
		send(socketEmisor, &idMensaje, sizeof(uint32_t), 0);
	} else {
		log_error(logger, "[ERROR]");
		log_error(logger,
				"No pudo obtenerse el tipo de cola en el mensaje recibido");
	}

}

/* Recibe el código de suscripción desde el socket a suscribirse, eligiendo de esta manera la cola y agregando el socket
 * a la lista de suscriptores de la misma.
 */
void atenderSuscripcion(int *socketSuscriptor) {

	int codSuscripcion, sizePaquete;

	recv(*socketSuscriptor, &sizePaquete, sizeof(uint32_t), MSG_WAITALL);
	recv(*socketSuscriptor, &codSuscripcion, sizeof(cola), MSG_WAITALL);
	log_debug(logger, "%d", codSuscripcion);

	enviarMensajesCacheados(*socketSuscriptor, codSuscripcion);

	sem_wait(&mutexColas);
	list_add(getListaSuscriptoresByNum(codSuscripcion), socketSuscriptor);
	log_info(logger,
			"Hay un nuevo suscriptor en la cola %s. Número de socket suscriptor: %d",
			getCodeStringByNum(codSuscripcion), *socketSuscriptor);
	sem_post(&mutexColas);
}

/* Espera mensajes de una conexión ya establecida. Según el código de operación recibido, delega tareas a distintos modulos.
 *
 */
void esperarMensajes(int *socketCliente) {
	int codOperacion;
	int sizeDelMensaje;
	cola tipoCola;

	recv(*socketCliente, &codOperacion, sizeof(int), MSG_WAITALL);
	log_debug(logger, "Esperando mensaje de cliente %d...", *socketCliente);

	switch (codOperacion) {
	case SUSCRIPCION: {
		log_info(logger, "[SUSCRIPCION]");
		atenderSuscripcion(socketCliente);
		log_info(logger, "[SUSCRIPCION-END]");
		break;
	}
	case NUEVO_MENSAJE: {
		/* En este punto habria que:
		 * - Determinar a que cola va a ir ese mensaje, y agregarlo. (Done)
		 * - Reenviar el mensaje a todos los suscriptores de dicha cola (¿o eso se hace en otro proceso asincronico?).
		 *   |- Concluimos en realizarlo mediante la funcion/hilo "atenderColas"
		 * - Guardar el mensaje en la caché. (Done)
		 *   |- Averiguar qué guardar en la cache, porque el enunciado dice que solo se puede guardar el payload
		 * - ...
		 */
		//Edit Gonzalo - 19/04
		//---------------
		log_info(logger, "[NUEVO_MENSAJE]");
		recv(*socketCliente, &sizeDelMensaje, sizeof(uint32_t), MSG_WAITALL);
		recv(*socketCliente, &tipoCola, sizeof(cola), MSG_WAITALL);
		atenderMensaje(*socketCliente, tipoCola);


		//Como es un connect para cada mensaje, aca habria que cerrar la conexion al socket
		close(*socketCliente);
		//---------------
		break;
	}
	case CONFIRMACION_MENSAJE: {
		// DONE
		// - Obtener id del mensaje y socket
		// -

		/*
		 uint32_t idConfirmacion;
		 recv(socketCliente, &tipoCola, sizeof(int), MSG_WAITALL);
		 recv(socketCliente, &idConfirmacion, sizeof(uint32_t), MSG_WAITALL);

		 bool coincideId(estructuraMensaje nodoMsj) {
		 bool encontro = false;
		 if (nodoMsj.id == idConfirmacion)
		 encontro = true;
		 return encontro;
		 }

		 estructuraMensaje nodoMsj = (estructuraMensaje*) list_find(getColaByNum(tipoCola), coincideId);
		 nodoMsj.estado = CONFIRMADO;
		 */
		break;
	}
	case FINALIZAR: {
		/* Finaliza la conexión con el broker de forma ordenada.
		 * No creo que tenga mucho sentido en el TP, seria para hacer pruebas.
		 */
		log_info(logger, "[FINALIZAR]");
		log_info(logger, "El cliente con socket %d se ha desconectado",
				*socketCliente);
		close(*socketCliente);
		break;
	}
	default: {
		log_error(logger, "El mensaje recibido está dañado");
		break;
	}
	}
}

/* Espera nuevas conexiones en el socket de escucha. Al establecerse una nueva, envía esa conexión a un nuevo hilo para que
 * sea gestionada y vuelve a esperar nuevas conexiones.
 */
void atenderConexiones(int *socketEscucha) {
	int backlog_server = config_get_int_value(config, "BACKLOG_SERVER");
	atenderConexionEn(*socketEscucha, backlog_server);
	while (1) {
		log_debug(logger, "Esperando cliente...");
		int *socketCliente = esperarCliente(*socketEscucha);
		log_info(logger,
				"Se ha conectado un cliente. Número de socket cliente: %d",
				*socketCliente);
		/* Esto me habia traido problemas antes, ¿andará asi?
		 * Sino habria que crear una lista de hilos e ir agregando/quitando
		 */

        esperarMensajes(socketCliente);
	}
}

// Chequea si un nodoMensaje tiene estado NUEVO. Devuelve bool porque list_filter requiere ese
void enviarEstructuraMensajeASuscriptor(void* estMensaje) {
	//¿Por que hacemos esto si conocemos el tipo que deberia llegarle a estMensaje? ¿Por que usar void *?
	estructuraMensaje* estMsj = (estructuraMensaje*) estMensaje;
	log_debug(logger, "Se está enviando un mensaje al suscriptor %d",estMsj->socketSuscriptor);
    log_debug(logger,"Se esta enviando el mensaje\nID: %d\nSuscriptor: %d\nID Correlativo: %d\nCola: %d\nSize: %d\nMensaje chorizeado: %s",
    		estMsj->id, estMsj->socketSuscriptor,estMsj->idCorrelativo,estMsj->colaMensajeria,estMsj->sizeMensaje,(char*)(estMsj->mensaje));
	enviarMensajeASuscriptor(*estMsj);
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

// Se filtran los mensajes que tienen estado nuevo y se envian, segun tipo
void atenderColas() {
	while (1) {
		sem_wait(&habilitarEnvio);
		sem_wait(&mutexColas);
		for (int numCola = 0; numCola < 6; numCola++) {
			if (list_size(getColaByNum(numCola)) > 0) {
				t_list* mensajesNuevos = list_filter(getColaByNum(numCola),
						&esMensajeNuevo);
				list_iterate(mensajesNuevos,
						&enviarEstructuraMensajeASuscriptor);
			}
		}
		sem_post(&mutexColas);

	}
}

void inicializarColasYListas() {

	NEW_POKEMON = list_create();
	APPEARED_POKEMON = list_create();
	CATCH_POKEMON = list_create();
	CAUGHT_POKEMON = list_create();
	GET_POKEMON = list_create();
	LOCALIZED_POKEMON = list_create();

	suscriptoresNEW = list_create();
	suscriptoresAPP = list_create();
	suscriptoresGET = list_create();
	suscriptoresLOC = list_create();
	suscriptoresCAT = list_create();
	suscriptoresCAU = list_create();
//IDs = list_create();
}

/*
 * La cache tiene que ser un espacio fijo, reservado al momento de ejecutar el broker. Tiene que ser un espacio tipo void
 * porque no se sabe que se va a guardar ahi, pero tambien tiene que estar inicializado para poder diferenciar que espacios
 * estan vacios. Hay que averiguar como hacer esto bien, puse un caracter para llenar.
 */
void inicializarCache() {
	CACHESIZE = config_get_int_value(config, "TAMANO_MEMORIA");
	cacheBroker = malloc(CACHESIZE);

	memset(cacheBroker, 'f', CACHESIZE);
}

void inicializarVariablesGlobales() {
	config = config_create("broker.config");
	logger = log_create("broker_logs", "Broker", 1, LOG_LEVEL_TRACE);

	inicializarColasYListas();
	inicializarCache();

	sem_init(&mutexColas,0,1);
	sem_init(&habilitarEnvio,0,0);
}

void destruirVariablesGlobales() {
	log_destroy(logger);
	config_destroy(config);
}

void liberarSocket(int* socket) {
	free(socket);
}

int getSocketEscuchaBroker() {

	char * ipEscucha = malloc(
			strlen(config_get_string_value(config, "IP_BROKER")) + 1);
	ipEscucha = config_get_string_value(config, "IP_BROKER");

	char * puertoEscucha = malloc(
			strlen(config_get_string_value(config, "PUERTO_BROKER")) + 1);
	puertoEscucha = config_get_string_value(config, "PUERTO_BROKER");

	int socketEscucha = crearConexionServer(ipEscucha, puertoEscucha);

	log_info(logger, "Se ha iniciado el servidor broker\n");
	log_info(logger,
			"El servidor está configurado y a la espera de un cliente. Número de socket servidor: %d",
			socketEscucha);

	return socketEscucha;

}

void empezarAAtenderCliente(int socketEscucha) {
	pthread_t hiloAtenderCliente;
	pthread_create(&hiloAtenderCliente, NULL, (void*) atenderConexiones,
			&socketEscucha);
	pthread_detach(hiloAtenderCliente);
}

void empezarAtenderColas() {
	pthread_t hiloAtenderColas;
	pthread_create(&hiloAtenderColas, NULL, (void*) atenderColas, NULL);
	pthread_join(hiloAtenderColas, NULL);
}

int main() {

	inicializarVariablesGlobales();

	int socketEscucha = getSocketEscuchaBroker();

	empezarAAtenderCliente(socketEscucha);
	//empezarAtenderColas();  Al pedo armar un nuevo thread por ahora
	atenderColas();

	destruirVariablesGlobales();
	liberarSocket(&socketEscucha);

	return 0;

}
