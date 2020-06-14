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

void esperarMensajes(int* socketSuscripcion) {

	uint32_t ack = 1;
	mensajeRecibido * mensaje;

	while (1) {
		mensaje = recibirMensajeDeBroker(*socketSuscripcion);
		send(*socketSuscripcion, &ack, sizeof(uint32_t), 0);

		switch (mensaje->colaEmisora) {
		case NEW: {
			//Procesar mensaje NEW
			log_debug(logger, "Llegó un mensaje de la cola NEW");
			break;
		}
		case GET: {
			//Procesar mensaje GET
			log_debug(logger, "Llegó un mensaje de la cola GET");
			break;
		}
		case CATCH: {
			//Procesar mensaje CATCH
			log_debug(logger, "Llegó un mensaje de la cola CATCH");
			break;
		}
		default: {
			log_error(logger,
					"Mensaje recibido de una cola no correspondiente");
			statusConexionBroker = ERROR_CONEXION;
			break;
		}

		free(mensaje);

		}
	}
}

int crearSuscripcionesBroker(){

	log_info(logger, "Creando suscripciones de Broker");
	socketSuscripcionNEW = crearConexionCliente(ipServidor, puertoServidor);
	if(socketSuscripcionNEW<0)
		return ERROR_CONEXION;
	suscribirseACola(socketSuscripcionNEW, NEW, idProceso);

	socketSuscripcionCATCH = crearConexionCliente(ipServidor, puertoServidor);
	if(socketSuscripcionCATCH<0)
		return ERROR_CONEXION;
	suscribirseACola(socketSuscripcionCATCH, CATCH, idProceso);

	socketSuscripcionGET = crearConexionCliente(ipServidor, puertoServidor);
	if(socketSuscripcionGET<0)
		return ERROR_CONEXION;
	suscribirseACola(socketSuscripcionGET, GET, idProceso);

	pthread_create(&hiloEsperaNEW, NULL, (void*) esperarMensajes, &socketSuscripcionNEW);
	pthread_create(&hiloEsperaGET, NULL, (void*) esperarMensajes, &socketSuscripcionGET);
	pthread_create(&hiloEsperaCATCH, NULL, (void*) esperarMensajes, &socketSuscripcionCATCH);

	pthread_detach(hiloEsperaNEW);
	pthread_detach(hiloEsperaGET);
	pthread_detach(hiloEsperaCATCH);

	log_info(logger, "Suscripciones de Broker creadas");

	return CONECTADO;

}

void atenderConexiones(int *socketEscucha) {
	/* Espera nuevas conexiones en el socket de escucha. Al establecerse una nueva, envía esa conexión a un nuevo hilo para que
	 * sea gestionada y vuelve a esperar nuevas conexiones.
	 */
	int backlog_server = 10;
	atenderConexionEn(*socketEscucha, backlog_server);
	while (1) {
		log_debug(logger, "Esperando cliente...");
		int *socketCliente = esperarCliente(*socketEscucha);
		log_info(logger,
				"Se ha conectado un cliente. Número de socket cliente: %d",
				*socketCliente);

        esperarMensajes(socketCliente);
        free(socketCliente);
	}
}


estadoConexion conectarYSuscribir(){
	if(idProceso==-1){
		log_info(logger, "Creando nueva conexion con Broker");
		idProceso=obtenerIdDelProceso(ipServidor,puertoServidor);
		log_info(logger, "IDProceso asignado: %d", idProceso);
		if(idProceso==-1)
			return ERROR_CONEXION;
	}
	estadoConexion statusConexion = crearSuscripcionesBroker();

	return statusConexion;
}

void mantenerConexionBroker(){
	int tiempoReintento = 10;
	//int tiempoReintento = config_get_int_value(config, "TIEMPO_DE_REINTENTO_CONEXION");
	statusConexionBroker = conectarYSuscribir();
	log_info(logger, "Manteniendo conexion de broker");
	while(1){
		if(statusConexionBroker!=CONECTADO){
			log_info(logger, "Reintentando conexion al Broker");
			pthread_cancel(hiloEsperaNEW);
			pthread_cancel(hiloEsperaGET);
			pthread_cancel(hiloEsperaCATCH);
			statusConexionBroker = conectarYSuscribir();
		}
		sleep(tiempoReintento);
	}
}


void procesarNew(){
	/*
	 * TODO -> EnunciadP
	 */
}

void procesarCatch(){
	/*
	 * TODO -> EnunciadO
	 */
}

void procesarGet(){
	/*
	 * TODO -> EnunciadO
	 */
}


void cerrarConexiones(){
	pthread_cancel(hiloEsperaGameboy);
	pthread_cancel(hiloReconexiones);
	close(socketEscuchaGameboy);
	close(socketSuscripcionNEW);
	close(socketSuscripcionCATCH);
	close(socketSuscripcionCATCH);

}

void crearConexionGameBoy(){
	/*TODO - DONE
	 *	- Levantar server (crearConexionServidor - utils)
	 *	- Escribir en log el socket
	 *	- crearHilo Atencion de mensajes:
	 */
	log_info(logger, "Creando hilo de conexion GameBoy");
	//socketEscuchaGameboy = crearConexionServer(config_get_string_value(config,"IP_GAMECARD"), config_get_string_value(config,"PUERTO_GAMECARD"));
	socketEscuchaGameboy = crearConexionServer("127.0.0.3", "6987");
	//pthread_create(&hiloEsperaGameboy, NULL, (void*) atenderConexiones, &socketEscuchaGameboy);
	//pthread_detach(hiloEsperaGameboy);
	atenderConexiones(&socketEscuchaGameboy);
	log_info(logger, "Hilo de conexion GameBoy creado");
}

void crearConexionBroker() {
	log_info(logger, "Creando hilo de conexion Broker");
	pthread_create(&hiloReconexiones, NULL, (void*) mantenerConexionBroker, NULL);
	pthread_detach(hiloReconexiones);
	//mantenerConexionBroker();
	log_info(logger, "Hilo de conexion Broker creado");
}

int main(){
	//Se setean todos los datos
	inicializarVariablesGlobales();
	log_info(logger, "Se ha iniciado el cliente GameCard\n");

    crearConexionBroker();
    crearConexionGameBoy();

	log_info(logger, "El proceso gamecard finalizó su ejecución\n");
	cerrarConexiones();
	destruirVariablesGlobales();

	return 0;
}
