#include "Broker.h"


void inicializarVariablesGlobales() {
	config = config_create("broker.config");
	logger = log_create("broker_logs", "Broker", 1, LOG_LEVEL_TRACE);

	inicializarColasYListas();
	inicializarCache();

	sem_init(&mutexColas, 0, 1);
	sem_init(&habilitarEnvio, 0, 0);

	globalIDProceso = 1;
	globalIDMensaje = 1;
}

void destruirVariablesGlobales() {
	log_destroy(logger);
	config_destroy(config);
}

void liberarSocket(int* socket) {
	free(socket);
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

int getSocketActualDelSuscriptor(uint32_t clientID, cola colaSuscripcion) {
	//TODO
	// - Encontrar el nodo suscriptor en la cola (parametro) con el clientID (parametro) requerido
	// - Retornar socket actual de ese suscriptor

	return 0;
}


void desuscribir(uint32_t clientID, cola colaSuscripcion) {
	//TODO
	// - Buscar el clientID en todas las listas de suscripcion
	// - Hacer close(socket) por cada uno encontrado
	// - Borrar nodo suscriptor

	int socketCliente = getSocketActualDelSuscriptor(clientID, colaSuscripcion);

	close(socketCliente);
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

uint32_t getIDMensaje() {
	return globalIDMensaje++;
}

uint32_t getIDProceso() {
	return globalIDProceso++;
}

int main() {

	inicializarVariablesGlobales();

	int socketEscucha = getSocketEscuchaBroker();

	empezarAAtenderCliente(socketEscucha);
	atenderColas();

	destruirVariablesGlobales();
	liberarSocket(&socketEscucha);

	return 0;

}
