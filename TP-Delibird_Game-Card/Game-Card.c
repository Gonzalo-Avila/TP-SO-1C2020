#include "Game-Card.h"


void inicializarVariablesGlobales() {
	config = config_create("gamecard.config");
	logger = log_create("gamecard_logs", "GameCard", 1, LOG_LEVEL_TRACE);

	ipServidor=malloc(strlen(config_get_string_value(config,"IP_BROKER"))+1);
	strcpy(ipServidor,config_get_string_value(config,"IP_BROKER"));
	puertoServidor=malloc(strlen(config_get_string_value(config,"PUERTO_BROKER"))+1);
	strcpy(puertoServidor,config_get_string_value(config,"PUERTO_BROKER"));

	idProceso=-1;
	statusConexionBroker=0;
}

void destruirVariablesGlobales() {
	free(ipServidor);
	free(puertoServidor);
	log_destroy(logger);
	config_destroy(config);
}


void procesarNEW(){
	/*
	 * TODO -> EnunciadP
	 */
}

void procesarCATCH(){
	/*
	 * TODO -> EnunciadO
	 */
}

void procesarGET(){
	/*
	 * TODO -> EnunciadO
	 */
}

void inicializarFileSystem(){
	/*
	 * Leer metadata, inicializar bloques y marcar como vacios
	 */
}

int main(){
	//Se setean todos los datos
	inicializarVariablesGlobales();
	log_info(logger, "Se ha iniciado el cliente GameCard\n");

	inicializarFileSystem();

    crearConexionBroker();
    crearConexionGameBoy();

	log_info(logger, "El proceso gamecard finalizó su ejecución\n");
	cerrarConexiones();
	destruirVariablesGlobales();

	return 0;
}
