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

void esperarMensajesDeBrokerEnCola(int* socketSuscripcion) {

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
			break;
		}

		free(mensaje);

		}

	}
}


void atenderNEWBroker(){
	/*	TODO
	 * - Recibir mensaje
	 * - llamar a procesarNEW()
	 * - Si recibir = -1  		==> Resuscribir todas las colas
	 */
}

void atenderCATCHBroker(){
	/*	TODO
	 * - Recibir mensaje
	 * - llamar a procesarCATCH()
	 * - Si recibir = -1  		==> Resuscribir todas las colas
	 */
}

void atenderGETBroker(){
	/*	TODO
	 * - Recibir mensaje
	 * - llamar a procesarGET()
	 * - Si recibir = -1  		==> Resuscribir todas las colas
	 */

}

/*void crearConexionBroker() {
	// TODO - Ver cambios
	// - Intentar reconexion

	ipServidor = malloc(
			strlen(config_get_string_value(config, "IP_BROKER")) + 1);
	puertoServidor = malloc(
			strlen(config_get_string_value(config, "PUERTO_BROKER")) + 1);

	ipServidor = config_get_string_value(config, "IP_BROKER");
	puertoServidor = config_get_string_value(config, "PUERTO_BROKER");

	uint32_t recStatus = obtenerIdDelProceso(ipServidor, puertoServidor);
	 * TODO
	 * - Si recStatus == -1 ===> Reconexion X tiempo (config)
	 * - Crear suscripcion NEW
	 * - Not OK				===> Reconexion X tiempo (config)
	 * - Crear suscripcion CATCH
	 * - Not OK				===> Reconexion X tiempo (config)
	 * - Crear suscripcion GET
	 * - Not OK				===> Reconexion X tiempo (config)	 *
	 *
	 * MAYBE
	 * - Ver de hacer while final para matar a los 3 hilos si algun recieve fallo
 }
	 */


/*
int suscribirA(cola colaMensaje) {
	int socketConexion = -1;
	int tiempoReintento = config_get_int_value(config, "TIEMPO_DE_REINTENTO_CONEXION");
	bool seLogroConexion = false;

	while (!seLogroConexion) {
		socketConexion = crearConexionCliente(ipServidor, puertoServidor);
		if(socketConexion == -1){
			sleep(tiempoReintento);
		}else{
			seLogroConexion = true;
			break;
		}
	}

	if(seLogroConexion){
		log_info(logger, "GameCard se ha conectado al Broker");
		suscribirseACola(socketConexion, colaMensaje, idProceso);
		log_info(logger, "GameCard se ha suscripto a %s", getCodeStringByNum(colaMensaje));
	}
	return socketConexion;
}


void crearSuscripcionesBroker() {

	int socketSuscripcionNEW = suscribirA(NEW);
	int socketSuscripcionGET = suscribirA(GET);
	int socketSuscipcionCATCH = suscribirA(CATCH);

	if(socketSuscripcionNEW != -1)
		pthread_create(&hiloEsperaNEW, NULL, (void*) esperarMensajesDeBrokerEnCola, &socketSuscripcionNEW);
	if(socketSuscripcionGET != -1)
		pthread_create(&hiloEsperaGET, NULL, (void*) esperarMensajesDeBrokerEnCola, &socketSuscripcionGET);
	if(socketSuscipcionCATCH != -1)
		pthread_create(&hiloEsperaCATCH, NULL, (void*) esperarMensajesDeBrokerEnCola, &socketSuscipcionCATCH);

	pthread_detach(hiloEsperaNEW);
	pthread_detach(hiloEsperaGET);
	pthread_detach(hiloEsperaCATCH);

	log_info(logger, "Esperando mensajes...");
}*/

void realizarFunciones() {
	log_debug(logger, "STOP");
	sleep(50000);
}


int crearSuscripcionesBroker(){
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

	pthread_create(&hiloEsperaNEW, NULL, (void*) esperarMensajesDeBrokerEnCola, &socketSuscripcionNEW);
	pthread_create(&hiloEsperaGET, NULL, (void*) esperarMensajesDeBrokerEnCola, &socketSuscripcionGET);
	pthread_create(&hiloEsperaCATCH, NULL, (void*) esperarMensajesDeBrokerEnCola, &socketSuscripcionCATCH);

	pthread_detach(hiloEsperaNEW);
	pthread_detach(hiloEsperaGET);
	pthread_detach(hiloEsperaCATCH);

	return CONECTADO;
}

estadoConexion conectarYSuscribir(){
	if(idProceso==-1){
		idProceso=obtenerIdDelProceso(ipServidor,puertoServidor);
		if(idProceso==-1)
			return ERROR_CONEXION;
	}
	estadoConexion statusConexion = crearSuscripcionesBroker();

	return statusConexion;
}

void mantenerConexion(){
	int tiempoReintento = config_get_int_value(config, "TIEMPO_DE_REINTENTO_CONEXION");
	while(1){
		if(statusConexionBroker!=CONECTADO){
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

void atenderMensajesGameBoy(){
	/*
	 * TODO
	 * - Identificar mensaje
	 * - Procesar en nuevo hilo con funcion acorde al mensaje (procesarNew, procesarCatch, procesarGet)
	 */

	//switch(){
		//procesarNew();
		//procesarCatch();
		//procesarGet();
	//}


}

int crearConexionGameBoy(){
	/*TODO - DONE
	 *	- Levantar server (crearConexionServidor - utils)
	 *	- Escribir en log el socket
	 *	- crearHilo Atencion de mensajes:
	 */
	socketEscuchaGameboy = crearConexionServer(config_get_string_value(config,"IP_GAMECARD"), config_get_string_value(config,"PUERTO_GAMECARD"));
	pthread_create(&hiloEsperaGameboy, NULL, (void*) esperarMensajesDeBrokerEnCola, &socketEscuchaGameboy);
	pthread_detach(hiloEsperaGameboy);
	return socketEscuchaGameboy;

}


int main(){
	//Se setean todos los datos
	inicializarVariablesGlobales();

	log_info(logger, "Se ha iniciado el cliente GameCard\n");

    crearConexionGameBoy();


	statusConexionBroker = conectarYSuscribir();
	/*pthread_create(&hiloReconexiones, NULL, (void*) mantenerConexion, NULL);
	pthread_detach(hiloReconexiones);*/

	realizarFunciones();




	log_info(logger, "El proceso gamecard finalizó su ejecución\n");

	destruirVariablesGlobales();
	close(socketEscuchaGameboy);
	close(socketSuscripcionNEW);
	close(socketSuscripcionCATCH);
	close(socketSuscripcionCATCH);


	return 0;
}
