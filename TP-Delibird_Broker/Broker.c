#include "Broker.h"
//#include <stdbool.h>

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

void* deserializarPayload(int socketSuscriptor) {

	log_debug(logger, "deserializarPayload");
	int msjLength;
	recv(socketSuscriptor, &msjLength, sizeof(int), MSG_WAITALL);
	log_debug(logger, "%d", msjLength);
	void* mensaje = malloc(msjLength);
	recv(socketSuscriptor, mensaje, msjLength, MSG_WAITALL);
	return mensaje;
}

char* getCodeStringByNum(int nro) {
	char* codigo[6] = { "NEW_POKEMON", "APPEARED_POKEMON", "CATCH_POKEMON",
			"CAUGHT_POKEMON", "GET_POKEMON", "LOCALIZED_POKEMON" };
	return codigo[nro];
}

t_list * getColaByNum(int nro) {
	t_list* codigo[6] = { NEW_POKEMON, APPEARED_POKEMON, CATCH_POKEMON,
			CAUGHT_POKEMON, GET_POKEMON, LOCALIZED_POKEMON };
	return codigo[nro];
}

t_list* getListaSuscriptoresByNum(int nro) {
	t_list* lista[6] = { suscriptoresNEW, suscriptoresAPP, suscriptoresCAT,
			suscriptoresCAU, suscriptoresGET, suscriptoresLOC };
	return lista[nro];
}

void *getEnviarAColaByNum(int num) {
	void (*codigo[6])(nodoMensaje nodoMsj) =
					  { enviarMensajeASuscriptorNEW,
						enviarMensajeASuscriptorAPP,
						enviarMensajeASuscriptorCAT,
						enviarMensajeASuscriptorCAU,
						enviarMensajeASuscriptorGET,
						enviarMensajeASuscriptorLOC };
	return codigo[num];
}

//Edit Gonzalo - 19/04
//----------

// TODO: revisar si usamos una variable global o size de la lista + 1, para no repetir el ID nunca durante la ejecución
/* Chequea los index de la lista global de IDs, a partir de uno, y cuando encuentra alguno libre se carga ahi
 *
 */
long generarID() {
	return globalID++;
}

t_list * generarListaDeSuscriptoresActuales(cola tipoCola) {
	t_list * listaGenerada = list_create();
	t_list * suscriptoresDeLaCola = getListaSuscriptoresByNum(tipoCola);

	for (int index = 0; index < list_size(suscriptoresDeLaCola); index++) {
		/*
		 * |- NUEVO (0):  el mensaje aun no fue enviado
		 * |- ENVIADO (1): el mensaje se envió, pero no fue confirmado
		 * |- CONFIRMADO (1): el suscriptor confirmó recepción del mensaje
		 */
		suscriptor sus;
		sus.socketSuscriptor = *(int *) list_get(suscriptoresDeLaCola, index);
		sus.status = NUEVO;
		list_add(listaGenerada, &sus);
	}

	return listaGenerada;
}
/*
 void imprimirEstructuraDeDatos(estructuraMensaje mensaje) {
 log_info(logger, "[NUEVO MENSAJE RECIBIDO]");
 log_info(logger, "ID: %d", mensaje.id);
 log_info(logger, "ID correlativo: %d", mensaje.idCorrelativo);
 log_info(logger, "Tamaño de mensaje: %d", mensaje.sizeMensaje);
 //TODO
 //Imprimir lista de suscriptores y datos del mensaje
 }

 int agregarMensajeACola(int socketEmisor,cola tipoCola, int idCorrelativo){
 estructuraMensaje mensajeNuevo;
 mensajeNuevo.id=generarID();
 mensajeNuevo.idCorrelativo=idCorrelativo;

 mensajeNuevo.listaSuscriptores=list_create();
 mensajeNuevo.listaSuscriptores=generarListaDeSuscriptoresActuales(tipoCola);

 recv(socketEmisor,&mensajeNuevo.sizeMensaje,sizeof(int),MSG_WAITALL);

 mensajeNuevo.mensaje = malloc(mensajeNuevo.sizeMensaje);
 recv(socketEmisor,mensajeNuevo.mensaje,mensajeNuevo.sizeMensaje,MSG_WAITALL);

 list_add(getColaByNum(tipoCola),&mensajeNuevo);

 imprimirEstructuraDeDatos(mensajeNuevo);

 if(idCorrelativo!=-1)
 cachearMensaje(mensajeNuevo.mensaje, mensajeNuevo.sizeMensaje);

 return mensajeNuevo.id;
 }
 */
void imprimirEstructuraDeDatos(nodoMensaje mensaje) {
	log_info(logger, "[NUEVO MENSAJE RECIBIDO]");
	log_info(logger, "ID: %d", mensaje.id);
	log_info(logger, "ID correlativo: %d", mensaje.idCorrelativo);
	log_info(logger, "Tamaño de mensaje: %d", mensaje.sizeMensaje);
	//TODO
	//Imprimir lista de suscriptores y datos del mensaje
}

int agregarMensajeACola(int socketEmisor, cola tipoCola, int idCorrelativo) {

	t_list* suscriptoresActuales = generarListaDeSuscriptoresActuales(tipoCola);
	int sizeMensaje;
	void* mensaje;

	recv(socketEmisor, sizeMensaje, sizeof(int), MSG_WAITALL);
	mensaje = malloc(sizeMensaje);
	recv(socketEmisor, mensaje, sizeMensaje, MSG_WAITALL);

	int id = generarID();

	for (int i = 0; i <= list_size(suscriptoresActuales); i++) {

		nodoMensaje mensajeNuevo;
		mensajeNuevo.id = id;
		mensajeNuevo.idCorrelativo = idCorrelativo;
		mensajeNuevo.estado = NUEVO;
		mensajeNuevo.sizeMensaje = sizeMensaje;
		mensajeNuevo.mensaje = mensaje;
		mensajeNuevo.socketSuscriptor = list_get(suscriptoresActuales, i);

		list_add(getColaByNum(tipoCola), &mensajeNuevo);

		imprimirEstructuraDeDatos(mensajeNuevo);

		if (idCorrelativo != -1)
			cachearMensaje(mensajeNuevo.mensaje, mensajeNuevo.sizeMensaje);

	}

	return id;
}

//----------

void atenderMensaje(int socketEmisor, cola tipoCola) {
	int idMensaje;
	uint32_t idCorrelativo;

	// TODO
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
	switch (tipoCola) {
	case NEW:
	case CATCH:
	case GET: {
		recv(socketEmisor, &idCorrelativo, sizeof(uint32_t), MSG_WAITALL);
		idMensaje = agregarMensajeACola(socketEmisor, tipoCola, -1);
		send(socketEmisor, &idMensaje, sizeof(uint32_t), 0);
		break;
	}
	case APPEARED:
	case CAUGHT:
	case LOCALIZED: {

		recv(socketEmisor, &idCorrelativo, sizeof(uint32_t), MSG_WAITALL);
		idMensaje = agregarMensajeACola(socketEmisor, tipoCola, idCorrelativo);
		send(socketEmisor, &idMensaje, sizeof(uint32_t), 0);
		break;

	}
	default: {
		log_error(logger, "[ERROR]");
		log_error(logger,
				"No pudo obtenerse el tipo de cola en el mensaje recibido");
	}
	}

	//-------------------------
}

void enviarMensajesCacheados(int socketSuscriptor, int codSuscripcion) {
	//TODO

}

/* Recibe el código de suscripción desde el socket a suscribirse, eligiendo de esta manera la cola y agregando el socket
 * a la lista de suscriptores de la misma.
 */
void atenderSuscripcion(int socketSuscriptor) {

	int codSuscripcion;
	recv(socketSuscriptor, &codSuscripcion, sizeof(int), MSG_WAITALL);

	enviarMensajesCacheados(socketSuscriptor, codSuscripcion);

	switch (codSuscripcion) {
	case NEW: {
		//Suscribir a NEW_POKEMON
		list_add(suscriptoresNEW, &socketSuscriptor);
		log_info(logger,
				"Hay un nuevo suscriptor en la cola NEW_POKEMON. Número de socket suscriptor: %d",
				socketSuscriptor);
		enviarString(socketSuscriptor,
				"Se suscribio satisfactoriamente a la cola de mensajes NEW_POKEMON");
		break;
	}
	case APPEARED: {
		//Suscribir a APPEARED_POKEMON
		list_add(suscriptoresAPP, &socketSuscriptor);
		log_info(logger,
				"Hay un nuevo suscriptor en la cola APPEARED_POKEMON. Número de socket suscriptor: %d",
				socketSuscriptor);
		enviarString(socketSuscriptor,
				"Se suscribio satisfactoriamente a la cola de mensajes APPEARED_POKEMON");
		break;
	}
	case CATCH: {
		//Suscribir a CATCH_POKEMON
		list_add(suscriptoresCAT, &socketSuscriptor);
		log_info(logger,
				"Hay un nuevo suscriptor en la cola CATCH_POKEMON. Número de socket suscriptor: %d",
				socketSuscriptor);
		enviarString(socketSuscriptor,
				"Se suscribio satisfactoriamente a la cola de mensajes CATCH_POKEMON");
		break;
	}
	case CAUGHT: {
		//Suscribir a CAUGHT_POKEMON
		list_add(suscriptoresCAU, &socketSuscriptor);
		log_info(logger,
				"Hay un nuevo suscriptor en la cola CAUGHT_POKEMON. Número de socket suscriptor: %d",
				socketSuscriptor);
		enviarString(socketSuscriptor,
				"Se suscribio satisfactoriamente a la cola de mensajes CAUGHT_POKEMON");
		break;
	}
	case GET: {
		//Suscribir a GET_POKEMON
		list_add(suscriptoresGET, &socketSuscriptor);
		log_info(logger,
				"Hay un nuevo suscriptor en la cola GET_POKEMON. Número de socket suscriptor: %d",
				socketSuscriptor);
		enviarString(socketSuscriptor,
				"Se suscribio satisfactoriamente a la cola de mensajes GET_POKEMON");
		break;
	}
	case LOCALIZED: {
		//Suscribir a LOCALIZED_POKEMON
		list_add(suscriptoresLOC, &socketSuscriptor);
		log_info(logger,
				"Hay un nuevo suscriptor en la cola LOCALIZED_POKEMON. Número de socket suscriptor: %d",
				socketSuscriptor);
		enviarString(socketSuscriptor,
				"Se suscribio satisfactoriamente a la cola de mensajes LOCALIZED_POKEMON");
		break;
	}
	default: {
		log_error(logger,
				"Intento fallido de suscripción a una cola de mensajes");
		break;
	}
	}
}

/* Espera mensajes de una conexión ya establecida. Según el código de operación recibido, delega tareas a distintos modulos.
 *
 */
void esperarMensajes(int socketCliente) {
	int codOperacion;
	int sizeDelMensaje;
	cola tipoCola;

	recv(socketCliente, &codOperacion, sizeof(int), MSG_WAITALL);

	switch (codOperacion) {
	case SUSCRIPCION: {
		log_info(logger, "[SUSCRIPCION]");
		atenderSuscripcion(socketCliente);
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
		log_info(logger, "[NUEVO_MENSAJE]");
		//Edit Gonzalo - 19/04
		//---------------
		recv(socketCliente, &sizeDelMensaje, sizeof(uint32_t), MSG_WAITALL);
		recv(socketCliente, &tipoCola, sizeof(cola), MSG_WAITALL);
		atenderMensaje(socketCliente, tipoCola);

		//Como es un connect para cada mensaje, aca habria que cerrar la conexion al socket

		//---------------
		break;
	}
	case CONFIRMACION_MENSAJE: {
		// TODO
		// - Obtener id del mensaje y socket
		// -

		uint32_t idConfirmacion;
		recv(socketCliente, &tipoCola, sizeof(cola), MSG_WAITALL);
		recv(socketCliente, &idConfirmacion, sizeof(uint32_t), MSG_WAITALL);

		bool coincideId(nodoMensaje nodoMsj){
			bool encontro = true;
			if(nodoMsj.id == idConfirmacion)
			return encontro;
		}

		nodoMensaje nodoMsj = list_find(tipoCola, coincideId);
		nodoMsj.estado = CONFIRMADO;
		break;
	}
	case FINALIZAR: {
		/* Finaliza la conexión con el broker de forma ordenada.
		 * No creo que tenga mucho sentido en el TP, seria para hacer pruebas.
		 */
		log_info(logger, "[FINALIZAR]");
		log_info(logger, "El cliente con socket %d se ha desconectado",
				socketCliente);
		close(socketCliente);
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
void atenderConexiones(int socketEscucha) {
	int backlog_server = config_get_int_value(config, "BACKLOG_SERVER");
	atenderConexionEn(socketEscucha, backlog_server);
	while (1) {
		int socketCliente = esperarCliente(socketEscucha);
		log_info(logger,
				"Se ha conectado un cliente. Número de socket cliente: %d",
				socketCliente);
		/* Esto me habia traido problemas antes, ¿andará asi?
		 * Sino habria que crear una lista de hilos e ir agregando/quitando
		 */
		pthread_t nuevoHilo;
		pthread_create(&nuevoHilo, NULL, (void*) esperarMensajes,
				socketCliente); //No entiendo el warning, si le paso un puntero no anda
		pthread_detach(nuevoHilo);

	}
}

void enviarASuscriptores(estructuraMensaje *estMsj) {
	log_debug(logger, ">>>>>>>>>>>>>>>>>>>>>>>>>>>> Enviando a todos los subs");
	for (int i = 0; i < list_size(estMsj->listaSuscriptores); i++) {
		//hacer logica de chequeo de conexion
		//	// Si no lo esta hay que hacer logica de reconexion. Quizas se podria hacer antes y delegar, para no
		//	// evitar el envio a los demas suscriptores en la lista

		int socketSuscriptor;
		//memcpy(&socketSuscriptor, list_get(estMsj->listaSuscriptores, i), sizeof(int));
		socketSuscriptor = *(int *) list_get(estMsj->listaSuscriptores, i);
		//enviarMensaje(socketSuscriptor, (char*) estMsj->msj);
		enviarString(socketSuscriptor, "hola");

	}
}

//	TODO
// - Tomar elemento de queue
// - Decidir si tiene idCorrelativo
// - Serializar con o sin idCorrelativo
// - Enviar a todos los suscriptores
/*
 void atenderColas() {
 //log_debug(logger, "atenderColas");
 estructuraMensaje * mensaje;

 while (1) {
 for (int i = 0; i < 6; i++) { //Revisar cada una de las 6 colas
 t_list * colaActual = getColaByNum(i);
 if (list_size(colaActual) > 0) { // Fijarse si en cada cola hay mensajes pendientes

 for (int j = 0; j < list_size(colaActual); j++) {
 mensaje = list_get(colaActual, j);
 for (int k = 0; k < list_size(mensaje->listaSuscriptores);
 k++) {
 suscriptor sus;
 sus = *(suscriptor *) list_get(
 mensaje->listaSuscriptores, k);
 if (sus.status != CONFIRMADO) {
 enviarMensajeASuscriptor(sus.socketSuscriptor, i,
 *mensaje);
 }
 }
 }
 }
 }
 }
 }
 */
//list_iterate(getColaByNum(i), enviarMensajePendiente);


// Funcion que adapta la estructua nodoMensaje a estructuraMensaje para poder utilizar fuinciones de estructuraMensaje
estructuraMensaje nodoAEstructura(nodoMensaje nodoMsj){
	estructuraMensaje estMsj;
	estMsj.id = nodoMsj.id;
	estMsj.idCorrelativo = nodoMsj.idCorrelativo;
	estMsj.sizeMensaje = nodoMsj.sizeMensaje;
	estMsj.mensaje = malloc(estMsj.sizeMensaje);
	estMsj.mensaje = nodoMsj.mensaje;

	return estMsj;
}

void enviarMensajeASuscriptorNEW(nodoMensaje nodoMsj) {
	enviarMensajeASuscriptor(nodoMsj.socketSuscriptor, NEW_POKEMON, nodoAEstructura(nodoMsj));
	nodoMsj.estado = ENVIADO;
}

void enviarMensajeASuscriptorAPP(nodoMensaje nodoMsj) {
	enviarMensajeASuscriptor(nodoMsj.socketSuscriptor, APPEARED_POKEMON, nodoAEstructura(nodoMsj));
	nodoMsj.estado = ENVIADO;
}

void enviarMensajeASuscriptorCAT(nodoMensaje nodoMsj) {
	enviarMensajeASuscriptor(nodoMsj.socketSuscriptor, CATCH_POKEMON, nodoAEstructura(nodoMsj));
	nodoMsj.estado = ENVIADO;
}

void enviarMensajeASuscriptorCAU(nodoMensaje nodoMsj) {
	enviarMensajeASuscriptor(nodoMsj.socketSuscriptor, CAUGHT_POKEMON, nodoAEstructura(nodoMsj));
	nodoMsj.estado = ENVIADO;
}

void enviarMensajeASuscriptorGET(nodoMensaje nodoMsj) {
	enviarMensajeASuscriptor(nodoMsj.socketSuscriptor, GET_POKEMON, nodoAEstructura(nodoMsj));
	nodoMsj.estado = ENVIADO;
}

void enviarMensajeASuscriptorLOC(nodoMensaje nodoMsj) {
	enviarMensajeASuscriptor(nodoMsj.socketSuscriptor, LOCALIZED_POKEMON, nodoAEstructura(nodoMsj));
	nodoMsj.estado = ENVIADO;
}

// Chequea si un nodoMensaje tiene estado NUEVO. Devuelve bool porque list_filter requiere ese
bool esMensajeNuevo(nodoMensaje mensaje) {
	bool esNuevo = false;
	if (mensaje.estado == NUEVO) {
		esNuevo = true;
	}
	return esNuevo;
}


void atenderColas() {
	while (1) {
		for (int i = 0; i < 6; i++) {
			// Se filtran los mensajes que tienen estado nuevo y se envian, segun tipo
			nodoMensaje* mensajesNuevos = list_filter(getColaByNum(i), esMensajeNuevo);
			list_iterate(mensajesNuevos, getEnviarAColaByNum(i));
		}
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

	IDs = list_create();
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
}

int main() {

	inicializarVariablesGlobales();

	char * ipEscucha = malloc(
			strlen(config_get_string_value(config, "IP_BROKER")) + 1);
	ipEscucha = config_get_string_value(config, "IP_BROKER");

	char * puertoEscucha = malloc(
			strlen(config_get_string_value(config, "PUERTO_BROKER")) + 1);
	puertoEscucha = config_get_string_value(config, "PUERTO_BROKER");

	log_info(logger, "Se ha iniciado el servidor broker\n");

	int socketEscucha = crearConexionServer(ipEscucha, puertoEscucha);
	log_info(logger,
			"El servidor está configurado y a la espera de un cliente. Número de socket servidor: %d",
			socketEscucha);

	//atenderConexiones(socketEscucha); // Agregar suscriptores a listas, Agregar mensajes a colas y Recibir confirmacion de mensajes
	//	atenderColas(); 				// Verifica mensajes encolados y los envia

	pthread_t hiloAtenderCliente;
	pthread_create(&hiloAtenderCliente, NULL, (void*) atenderConexiones,
			socketEscucha);

	pthread_t hiloAtenderColas;
	//pthread_create(&hiloAtenderColas, NULL, (void*) atenderColas, NULL);

	pthread_join(hiloAtenderCliente, NULL);
	//pthread_join(hiloAtenderColas, NULL);

	//Otros hilos seguirian aca
	free(puertoEscucha);
	log_destroy(logger);
	config_destroy(config);

	return 0;

}
