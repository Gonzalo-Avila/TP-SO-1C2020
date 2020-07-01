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

int enviarMensajeBroker(cola colaDestino, uint32_t idCorrelativo,uint32_t sizeMensaje, void * mensaje) {
	int socketBroker = crearConexionCliente(ipServidor, puertoServidor);
	if (socketBroker < 0) {
		log_error(logger, "No se pudo enviar el mensaje al broker: conexion fallida.");
		return socketBroker;
	}
	enviarMensajeABroker(socketBroker, colaDestino, idCorrelativo, sizeMensaje, mensaje);
	uint32_t respuestaBroker;
	recv(socketBroker, &respuestaBroker, sizeof(uint32_t), MSG_WAITALL);
	close(socketBroker);
	return respuestaBroker;
}


int crearSuscripcionesBroker(){

	log_info(logger, "Intentando crear suscripciones a broker");
	socketSuscripcionNEW = crearConexionCliente(ipServidor, puertoServidor);
	if(socketSuscripcionNEW==-1)
		return ERROR_CONEXION;
	suscribirseACola(socketSuscripcionNEW, NEW, idProceso);
	log_info(logger, "Suscripcion a NEW_POKEMON realizada");

	socketSuscripcionCATCH = crearConexionCliente(ipServidor, puertoServidor);
	if(socketSuscripcionCATCH==-1)
		return ERROR_CONEXION;
	suscribirseACola(socketSuscripcionCATCH, CATCH, idProceso);
	log_info(logger, "Suscripcion a CATCH_POKEMON realizada");

	socketSuscripcionGET = crearConexionCliente(ipServidor, puertoServidor);
	if(socketSuscripcionGET==-1)
		return ERROR_CONEXION;
	suscribirseACola(socketSuscripcionGET, GET, idProceso);
	log_info(logger, "Suscripcion a GET_POKEMON realizada");

	pthread_create(&hiloEsperaNEW, NULL, (void*) esperarMensajesBroker, &socketSuscripcionNEW);
	pthread_create(&hiloEsperaGET, NULL, (void*) esperarMensajesBroker, &socketSuscripcionGET);
	pthread_create(&hiloEsperaCATCH, NULL, (void*) esperarMensajesBroker, &socketSuscripcionCATCH);

	pthread_detach(hiloEsperaNEW);
	pthread_detach(hiloEsperaGET);
	pthread_detach(hiloEsperaCATCH);

	log_info(logger, "Suscripciones a colas realizadas exitosamente");

	return CONECTADO;

}

estadoConexion conectarYSuscribir(){
	if(idProceso==-1){
		log_info(logger, "Intentando establecer conexion inicial con broker...");
		idProceso=obtenerIdDelProceso(ipServidor,puertoServidor);

		if(idProceso==-1){
			log_info(logger, "No se pudo establecer la conexion inicial con el broker");
			return ERROR_CONEXION;
		}
		log_info(logger, "IDProceso asignado: %d", idProceso);
	}
	estadoConexion statusConexion = crearSuscripcionesBroker();
	if(statusConexion==ERROR_CONEXION)
		log_info(logger,"No se pudieron realizar las suscripciones. Error de conexion");
	return statusConexion;
}

void mantenerConexionBroker(){
	int tiempoReintento = config_get_int_value(config, "TIEMPO_DE_REINTENTO_CONEXION");
	statusConexionBroker = conectarYSuscribir();
	while(1){
		if(statusConexionBroker!=CONECTADO){
			log_info(logger, "Reintentando conectar con el broker...");
			statusConexionBroker = conectarYSuscribir();
		}
		sleep(tiempoReintento);
	}
}

void cerrarConexiones(){
	close(socketEscuchaGameboy);
	close(socketSuscripcionNEW);
	close(socketSuscripcionCATCH);
	close(socketSuscripcionCATCH);
}

void esperarMensajesBroker(int* socketSuscripcion) {
	uint32_t ack = 1;
	while (1) {
		mensajeRecibido * mensaje = recibirMensajeDeBroker(*socketSuscripcion);
		if(mensaje->codeOP==FINALIZAR){
			free(mensaje);
			statusConexionBroker=ERROR_CONEXION;
			close(*socketSuscripcion);
			break;
		}
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

			log_error(logger, "[BROKER] Mensaje recibido de una cola no correspondiente");
			statusConexionBroker = ERROR_CONEXION;
			break;
		}
		}
		free(mensaje->mensaje);
		free(mensaje);
	}
}
