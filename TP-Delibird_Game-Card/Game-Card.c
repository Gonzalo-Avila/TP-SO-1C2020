#include "Game-Card.h"
#include "Utils.h"

void inicializarVariablesGlobales(){
	config = config_create("gamecard.config");
	logger = log_create("gamecard_logs","GameCard",1,LOG_LEVEL_TRACE);
}


uint32_t obtenerIdDelProceso(char* ip, char* puerto) {
	int socketBroker = crearConexionCliente(ip, puerto);
	uint32_t idProceso;

	opCode codigoOP = NUEVA_CONEXION;
	send(socketBroker, &codigoOP, sizeof(opCode), 0);
	recv(socketBroker, &idProceso, sizeof(uint32_t), MSG_WAITALL);
	close(socketBroker);

	return idProceso;
}

int main(){
    //Se setean todos los datos
	inicializarVariablesGlobales();

	char * ipServidor = malloc(strlen(config_get_string_value(config,"IP"))+1);
	ipServidor = config_get_string_value(config,"IP");
	char * puertoServidor = malloc(strlen(config_get_string_value(config,"PUERTO"))+1);
	puertoServidor = config_get_string_value(config,"PUERTO");
    log_info(logger,"Se ha iniciado el cliente GameCard\n");

    //Se crea la conexion con el broker. Esto posteriormente debe ir con un sistema de reintentos por si el broker esta off
    int socketBroker = crearConexionCliente(ipServidor,puertoServidor);
    log_info(logger,"Se ha establecido conexión con el servidor\nIP: %s\nPuerto: %s\nNúmero de socket: %d",
    		config_get_string_value(config,"IP"),config_get_string_value(config,"PUERTO"));

    uint32_t idProceso = obtenerIdDelProceso(ipServidor, puertoServidor);

    suscribirseACola(socketBroker, NEW, idProceso);
    suscribirseACola(socketBroker, GET, idProceso);
    suscribirseACola(socketBroker, CATCH, idProceso);

    free(ipServidor);
    free(puertoServidor);

    //Procedimiento auxiliar para que no rompa el server en las pruebas
    int codigoOP = FINALIZAR;
    send(socketBroker,(void*)&codigoOP,sizeof(opCode),0);
    close(socketBroker);

    log_info(logger,"Finalizó la conexión con el servidor\n");
    log_info(logger,"El proceso team finalizó su ejecución\n");

    log_destroy(logger);
    config_destroy(config);

    return 0;
}
