#include "Game-Card.h"

void inicializarVariablesGlobales() {
	config = config_create("gamecard.config");
	logger = log_create("gamecard_logs", "GameCard", 1, LOG_LEVEL_TRACE);
}

void cerrarConexion() {
	free(ipServidor);
	free(puertoServidor);
	log_info(logger, "Finalizó la conexión con el servidor\n");
}

void destruirVariablesGlobales() {
	log_destroy(logger);
	config_destroy(config);
}

void esperarMensajesDeBrokerEnCola(int* socketSuscripcion) {

	int* socketSus = malloc(sizeof(int));
	uint32_t ack = 1;
	mensajeRecibido * mensaje;

	while (1) {
		*socketSus = *socketSuscripcion;
		mensaje = recibirMensajeDeBroker(*socketSus);
		send(*socketSus, &ack, sizeof(uint32_t), 0);

		switch (mensaje->colaEmisora) {
		case NEW: {
			//Procesar mensaje NEW
			log_debug(logger, "Llegó un mensaje de la cola NEW");
			break;
		}
		case GET: {
			//Procesar mensaje NEW
			log_debug(logger, "Llegó un mensaje de la cola GET");
			break;
		}
		case CATCH: {
			//Procesar mensaje NEW
			log_debug(logger, "Llegó un mensaje de la cola CATCH");
			break;
		}
		default: {
			log_error(logger,
					"Mensaje recibido de una cola no correspondiente");
			break;
		}
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

void crearConexionBroker() {
	// TODO
	// - Intentar reconexion

	ipServidor = malloc(
			strlen(config_get_string_value(config, "IP_BROKER")) + 1);
	puertoServidor = malloc(
			strlen(config_get_string_value(config, "PUERTO_BROKER")) + 1);

	ipServidor = config_get_string_value(config, "IP_BROKER");
	puertoServidor = config_get_string_value(config, "PUERTO_BROKER");

	uint32_t recStatus = obtenerIdDelProceso(ipServidor, puertoServidor);
	/*
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
	 */

}

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

void realizarFunciones() {
	log_debug(logger, "STOP");
	sleep(50000);
}

void crearSuscripcionesBroker() {

	pthread_t hiloEsperaNEW, hiloEsperaGET, hiloEsperaCATCH;

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


void crearConexionGameBoy(){
	/*TODO
	 *	- Levantar server (crearConexionServidor - utils)
	 *	- Escribir en log el socket
	 *	- crearHilo Atencion de mensajes:
	 */


	// atenderMensajesGameBoy()
}

int main(){
	//Se setean todos los datos
	inicializarVariablesGlobales();

	log_info(logger, "Se ha iniciado el cliente GameCard\n");

	crearConexionGameBoy();

	idProceso = crearConexionBroker();

	if(idProceso!=-1) {
		crearSuscripcionesBroker();
	}

	realizarFunciones();

	cerrarConexion();

	destruirVariablesGlobales();

	//Procedimiento auxiliar para que no rompa el server en las pruebas
	/*int codigoOP = FINALIZAR;
	 send(socketBroker,(void*)&codigoOP,sizeof(opCode),0);
	 close(socketBroker);*/

	log_info(logger, "El proceso gamecard finalizó su ejecución\n");

	return 0;
}
