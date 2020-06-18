/*
 * Game-Card_ConexionGameBoy.c
 *
 *  Created on: 15 jun. 2020
 *      Author: utnso
 */
#include "Game-Card.h"

void crearConexionGameBoy() {
	log_info(logger, "Creando hilo de conexion GameBoy");
	socketEscuchaGameboy = crearConexionServer(config_get_string_value(config,"IP_GAMECARD"), config_get_string_value(config,"PUERTO_GAMECARD"));
	//socketEscuchaGameboy = crearConexionServer("127.0.0.3", "6987");
	atenderConexiones(&socketEscuchaGameboy);
	log_info(logger, "Hilo de conexion GameBoy creado");
}

void atenderConexiones(int *socketEscucha) {
	int backlog_server = 10;
	atenderConexionEn(*socketEscucha, backlog_server);
	while (1) {
		log_debug(logger, "Esperando cliente...");
		int *socketCliente = esperarCliente(*socketEscucha);
		log_info(logger,
				"Se ha conectado un cliente. Número de socket cliente: %d",
				*socketCliente);

		esperarMensajesGameboy(socketCliente);
		free(socketCliente);
	}
}

void imprimirContenido(mensajeRecibido * mensaje, int * socketSuscripcion){
	log_debug(logger, "Recibiendo mensaje del socket cliente: %d",
			*socketSuscripcion);
	log_debug(logger, "codeOP %d",(int) mensaje->codeOP);
	log_debug(logger, "sizePayload %d", mensaje->sizePayload);
	log_debug(logger, "colaEmisora %s", getCodeStringByNum(mensaje->colaEmisora));
	log_debug(logger, "idMensaje %d", mensaje->idMensaje);
	log_debug(logger, "idCorrelativo %d", mensaje->idCorrelativo);
	log_debug(logger, "sizeMensaje %d", mensaje->sizeMensaje);
	log_debug(logger, "Mensaje recibido: %c", (char*) mensaje->mensaje);
}

void esperarMensajesGameboy(int* socketSuscripcion) {

	log_info(logger,"[GAMEBOY] Recibiendo mensaje del socket cliente: %d",*socketSuscripcion);
	mensajeRecibido * mensaje = recibirMensajeDeBroker(*socketSuscripcion);
	log_info(logger,"[GAMEBOY] Mensaje recibido");

	//imprimirContenido(mensaje, socketSuscripcion);

	switch (mensaje->colaEmisora) {
	case NEW: {
		//Procesar mensaje NEW
		log_debug(logger, "[GAMEBOY] Llegó un mensaje de la cola NEW");
		procesarNEW(mensaje);
		break;
	}
	case GET: {
		//Procesar mensaje GET
		log_debug(logger, "[GAMEBOY] Llegó un mensaje de la cola GET");
		procesarGET(mensaje);

		break;
	}
	case CATCH: {
		//Procesar mensaje CATCH
		log_debug(logger, "[GAMEBOY] Llegó un mensaje de la cola CATCH");
		procesarCATCH(mensaje);
		break;
	}
	default: {

		log_error(logger, "[GAMEBOY] Mensaje recibido de una cola no correspondiente");
		statusConexionBroker = ERROR_CONEXION;
		break;
	}

	}
	free(mensaje);

}
