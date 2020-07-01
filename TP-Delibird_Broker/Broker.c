#include "Broker.h"


void inicializarVariablesGlobales() {
	config = config_create("broker.config");
	logger = log_create("broker_logs", "Broker", 1, LOG_LEVEL_TRACE);
	loggerOficial = log_create(config_get_string_value(config,"LOG_FILE"),"Delibird - Broker", 0, LOG_LEVEL_TRACE);

	inicializarColasYListas();
	inicializarCache();

	sem_init(&mutexColas, 0, 1);
	sem_init(&habilitarEnvio, 0, 0);
	sem_init(&mutex_regParticiones, 0, 1);

	globalIDProceso = config_get_int_value(config,"PROCESOS_REGISTRADOS")+1;
	globalIDMensaje = config_get_int_value(config,"MENSAJES_REGISTRADOS")+1;
}

void destruirVariablesGlobales() {
	log_destroy(logger);
	log_destroy(loggerOficial);
	config_destroy(config);

	list_clean(suscriptoresNEW);
	list_clean(suscriptoresAPP);
	list_clean(suscriptoresGET);
	list_clean(suscriptoresLOC);
	list_clean(suscriptoresCAT);
	list_clean(suscriptoresCAU);

	//Cerrar el socket (¿Variable global?) y ¿eliminar colas? (se hace cada vez que se manda un mensaje)

    free(cacheBroker);
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

	idCorrelativosRecibidos = list_create();

	registrosDeCache = list_create();
	registrosDeParticiones = list_create();

}

int getSocketEscuchaBroker() {

	/*char * ipEscucha = malloc(
			strlen(config_get_string_value(config, "IP_BROKER")) + 1);*/
	char * ipEscucha = config_get_string_value(config, "IP_BROKER");

	/*char * puertoEscucha = malloc(
			strlen(config_get_string_value(config, "PUERTO_BROKER")) + 1);*/
	char * puertoEscucha = config_get_string_value(config, "PUERTO_BROKER");

	int socketEscucha = crearConexionServer(ipEscucha, puertoEscucha);

	log_info(logger, "Se ha iniciado el servidor broker\n");
	log_info(logger,
			"El servidor está configurado y a la espera de un cliente. Número de socket servidor: %d",
			socketEscucha);


	/*free(ipEscucha);
	free(puertoEscucha);*/

	return socketEscucha;

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
void eliminarSuscriptor(t_list* listaSuscriptores, uint32_t clientID){
	bool existeClientID(void * _suscriptor){
		   suscriptor* sus = (suscriptor *) _suscriptor;
		   return sus->clientID==clientID;
	   }
	list_remove_by_condition(listaSuscriptores, existeClientID);
}

void desuscribir(uint32_t clientID, cola colaSuscripcion) {

	log_info(logger, "[DESUSCRIPCION]");
	log_info(logger, "Se procedera a desuscribir de la cola %s al suscriptor con clientID: %d", getCodeStringByNum(colaSuscripcion),clientID);
	int socketCliente = getSocketActualDelSuscriptor(clientID, colaSuscripcion);
	close(socketCliente);
	eliminarSuscriptor(getListaSuscriptoresByNum(colaSuscripcion), clientID);
	log_info(logger, "[DESUSCRIPCION-END]");
}


int getSocketActualDelSuscriptor(uint32_t clientID, cola colaSuscripcion) {

	suscriptor* sus;
	sus = buscarSuscriptor(clientID, colaSuscripcion);
	return sus->socketCliente;
}


suscriptor * buscarSuscriptor(uint32_t clientID, cola codSuscripcion){
	   bool existeClientID(void * _suscriptor){
		   suscriptor* sus = (suscriptor *) _suscriptor;
		   return sus->clientID==clientID;
	   }
	   t_list * listaSuscriptores = getListaSuscriptoresByNum(codSuscripcion);
	   return list_find(listaSuscriptores,(void *)existeClientID);
}



uint32_t getIDMensaje() {
	config_set_value(config,"MENSAJES_REGISTRADOS",string_itoa(globalIDMensaje));
	config_save(config);
	return globalIDMensaje++;
}

uint32_t getIDProceso() {
	config_set_value(config,"PROCESOS_REGISTRADOS",string_itoa(globalIDProceso));
	config_save(config);
	return globalIDProceso++;
}

void finalizarEjecucion(){
	destruirVariablesGlobales();
	close(copiaSocketGlobal);
	exit(0);
}

int main() {
	signal(SIGUSR1,dumpCache);
	signal(SIGINT,finalizarEjecucion);

	inicializarVariablesGlobales();
	log_info(logger, "PID BROKER: %d", getpid());

	int socketEscucha = getSocketEscuchaBroker();
	copiaSocketGlobal=socketEscucha;

	empezarAAtenderCliente(socketEscucha);
	atenderColas();

	return 0;

}
