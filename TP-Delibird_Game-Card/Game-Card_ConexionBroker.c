/*
 * Game-Card_Conexiones.c
 *
 *  Created on: 15 jun. 2020
 *      Author: utnso
 */
#include "Game-Card.h"

void crearConexionBroker() {
	log_info(logger, "Creando hilo de conexion Broker");
	pthread_create(&hiloReconexiones, NULL, (void*) mantenerConexionBroker, NULL);
	pthread_detach(hiloReconexiones);
	log_info(logger, "Hilo de conexion Broker creado");

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

	pthread_create(&hiloEsperaNEW, NULL, (void*) esperarMensajesBroker, &socketSuscripcionNEW);
	pthread_create(&hiloEsperaGET, NULL, (void*) esperarMensajesBroker, &socketSuscripcionGET);
	pthread_create(&hiloEsperaCATCH, NULL, (void*) esperarMensajesBroker, &socketSuscripcionCATCH);

	pthread_detach(hiloEsperaNEW);
	pthread_detach(hiloEsperaGET);
	pthread_detach(hiloEsperaCATCH);

	log_info(logger, "Suscripciones de Broker creadas");

	return CONECTADO;

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
	log_info(logger, "Creando conexion estable de broker");
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

void cerrarConexiones(){
	pthread_cancel(hiloEsperaGameboy);
	pthread_cancel(hiloReconexiones);
	close(socketEscuchaGameboy);
	close(socketSuscripcionNEW);
	close(socketSuscripcionCATCH);
	close(socketSuscripcionCATCH);
}


void esperarMensajesBroker(int* socketSuscripcion) {
	uint32_t ack = 1;
	while (1) {
		mensajeRecibido * mensaje = recibirMensajeDeBroker(*socketSuscripcion);
		send(*socketSuscripcion, &ack, sizeof(uint32_t), 0);
		log_info(logger,"[BROKER] Mensaje recibido");

		switch (mensaje->colaEmisora) {
		case NEW: {
			//Procesar mensaje NEW
			log_debug(logger, "[BROKER] Llegó un mensaje de la cola NEW");
			procesarNEW(mensaje);
			break;
		}
		case GET: {
			//Procesar mensaje GET
			log_debug(logger, "[BROKER] Llegó un mensaje de la cola GET");
			procesarGET(mensaje);
			break;
		}
		case CATCH: {
			//Procesar mensaje CATCH
			log_debug(logger, "[BROKER] Llegó un mensaje de la cola CATCH");
			procesarCATCH(mensaje);
			break;
		}
		default: {

			log_error(logger,
					"[BROKER] Mensaje recibido de una cola no correspondiente");
			statusConexionBroker = ERROR_CONEXION;
			break;
		}
		}
		free(mensaje);
	}
}
