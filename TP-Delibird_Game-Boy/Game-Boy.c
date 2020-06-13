#include "Game-Boy.h"


void inicializarVariablesGlobales() {
	config = config_create("gameboy.config");
	logger = log_create("gameboy_logs", "GameBoy", 1, LOG_LEVEL_TRACE);
	loggerOficial = log_create("gameboy_logs_oficial", "Delibird - GameBoy",0,LOG_LEVEL_TRACE);
}

int conectarseADestino(proceso destino) {

	char * ipDestino = malloc(16);
	char * puertoDestino = malloc(6);
	char * proceso = malloc(10);
	int socketDestino;

	switch (destino) {
	case SUSCRIPTOR:
	case SUSCRIPTOR_CID:
	case BROKER: {
		strcpy(ipDestino,config_get_string_value(config, "IP_BROKER"));
		strcpy(puertoDestino,config_get_string_value(config, "PUERTO_BROKER"));
		strcpy(proceso, "BROKER");
		break;
	}
	case TEAM: {
		strcpy(ipDestino,config_get_string_value(config, "IP_TEAM"));
		strcpy(puertoDestino,config_get_string_value(config, "PUERTO_TEAM"));
		strcpy(proceso, "TEAM");
		break;
	}
	case GAMECARD: {
		strcpy(ipDestino,config_get_string_value(config, "IP_GAMECARD"));
		strcpy(puertoDestino,config_get_string_value(config, "PUERTO_GAMECARD"));
		strcpy(proceso, "GAMECARD");
		break;
	}
	default: {
		log_error(logger, "Error obteniendo el destino del mensaje");
		return -1;
	}
	}
	socketDestino = crearConexionCliente(ipDestino, puertoDestino);
	if (socketDestino != -1) {
		log_info(logger,
				"Se ha conectado el GameBoy a un proceso.\nTipo de proceso: %s\nIP: %s\nPUERTO: %s\nSocket:%d",
				proceso, ipDestino, puertoDestino, socketDestino);

		log_info(loggerOficial,
					"Se ha conectado el GameBoy a un proceso.Tipo de proceso: %s | IP: %s | PUERTO: %s | Socket:%d",
					proceso, ipDestino, puertoDestino, socketDestino);
	}
	free(ipDestino);
	free(puertoDestino);
	free(proceso);
	return socketDestino;
}
cola definirTipoMensaje(char * argumento) {
	if (strcmp("NEW_POKEMON", argumento) == 0) {
		return NEW;
	}
	if (strcmp("APPEARED_POKEMON", argumento) == 0) {
		return APPEARED;
	}
	if (strcmp("CATCH_POKEMON", argumento) == 0) {
		return CATCH;
	}
	if (strcmp("CAUGHT_POKEMON", argumento) == 0) {
		return CAUGHT;
	}
	if (strcmp("GET_POKEMON", argumento) == 0) {
		return GET;
	}
	if (strcmp("LOCALIZED_POKEMON", argumento) == 0) {
		return LOCALIZED;
	}
	log_error(logger, "[ERROR]");
	log_error(logger, "No se pudo determinar el tipo de cola o suscripción");
	return -1;
}
resultado obtenerResultadoDesdeCadena(char * cadena){
	if(strcmp("OK",cadena)==0)
	   return OK;
	if(strcmp("FAIL",cadena)==0)
	   return FAIL;

	return -1;
}
proceso definirDestino(char * argumento) {
	if (strcmp("TEAM", argumento) == 0) {
		return TEAM;
	}
	if (strcmp("GAMECARD", argumento) == 0) {
		return GAMECARD;
	}
	if (strcmp("BROKER", argumento) == 0) {
		return BROKER;
	}
	if (strcmp("SUSCRIPTOR", argumento) == 0) {
		return SUSCRIPTOR;
	}
	if (strcmp("SUSCRIPTOR_CID", argumento) == 0) {
			return SUSCRIPTOR_CID;
	}
	log_error(logger, "[ERROR]");
	log_error(logger, "No se pudo determinar el proceso destino");
	return -1;

}

void imprimirMensaje(mensajeRecibido * mensaje){
	log_info(logger,"Se recibió un nuevo mensaje del broker\n"
	            		"Cola: %s\n"
	            		"ID del mensaje: %d\n"
	            		"ID correlativo: %d\n"
	            		"Tamaño del mensaje: %d\n",
						getCodeStringByNum(mensaje->colaEmisora),mensaje->idMensaje, mensaje->idCorrelativo,mensaje->sizeMensaje);

	log_info(loggerOficial,"Se recibió un nuevo mensaje del broker en la cola: %s\n",getCodeStringByNum(mensaje->colaEmisora));

	int offset=0;
	switch(mensaje->colaEmisora){
	case NEW:{
		mensajeNew msg;

		memcpy(&(msg.longPokemon),mensaje->mensaje+offset,sizeof(uint32_t));
		offset+=sizeof(uint32_t);

		msg.pokemon=malloc(msg.longPokemon);
		memcpy(msg.pokemon,mensaje->mensaje+offset,msg.longPokemon);
		offset+=msg.longPokemon;

		memcpy(&(msg.posicionX),mensaje->mensaje+offset,sizeof(uint32_t));
		offset+=sizeof(uint32_t);

		memcpy(&(msg.posicionY),mensaje->mensaje+offset,sizeof(uint32_t));
		offset+=sizeof(uint32_t);

		memcpy(&(msg.cantPokemon),mensaje->mensaje+offset,sizeof(uint32_t));
		offset+=sizeof(uint32_t);

		log_info(logger, "Longitud pokemon: %d\nPokemon: %s\nPosicion X: %d\nPosicion Y: %d\n Cantidad del pokemon:%d\n",msg.longPokemon,msg.pokemon,msg.posicionX,msg.posicionY,msg.cantPokemon);

		free(msg.pokemon);
		break;
	}
	case APPEARED:{
		mensajeAppeared msg;

		memcpy(&(msg.longPokemon),mensaje->mensaje+offset,sizeof(uint32_t));
		offset+=sizeof(uint32_t);

		msg.pokemon=malloc(msg.longPokemon);
		memcpy(msg.pokemon,mensaje->mensaje+offset,msg.longPokemon);
		offset+=msg.longPokemon;

		memcpy(&(msg.posicionX),mensaje->mensaje+offset,sizeof(uint32_t));
		offset+=sizeof(uint32_t);

		memcpy(&(msg.posicionY),mensaje->mensaje+offset,sizeof(uint32_t));
		offset+=sizeof(uint32_t);

		log_info(logger, "Longitud pokemon: %d\nPokemon: %s\nPosicion X: %d\nPosicion Y: %d\n",msg.longPokemon,msg.pokemon,msg.posicionX,msg.posicionY);

		free(msg.pokemon);
		break;
	}
	case CATCH:{
		mensajeCatch msg;

		memcpy(&(msg.longPokemon),mensaje->mensaje+offset,sizeof(uint32_t));
		offset+=sizeof(uint32_t);

		msg.pokemon=malloc(msg.longPokemon);
		memcpy(msg.pokemon,mensaje->mensaje+offset,msg.longPokemon);
		offset+=msg.longPokemon;

		memcpy(&(msg.posicionX),mensaje->mensaje+offset,sizeof(uint32_t));
		offset+=sizeof(uint32_t);

		memcpy(&(msg.posicionY),mensaje->mensaje+offset,sizeof(uint32_t));
		offset+=sizeof(uint32_t);

		log_info(logger, "Longitud pokemon: %d\nPokemon: %s\nPosicion X: %d\nPosicion Y: %d\n",msg.longPokemon,msg.pokemon,msg.posicionX,msg.posicionY);

		free(msg.pokemon);
		break;
	}
	case CAUGHT:{
		mensajeCaught msg;

		memcpy(&(msg.resultado),mensaje->mensaje+offset,sizeof(uint32_t));
		log_info(logger, "Resultado: %d\n",msg.resultado);

		break;
	}
	case GET:{
		mensajeGet msg;

		memcpy(&(msg.longPokemon),mensaje->mensaje+offset,sizeof(uint32_t));
		offset+=sizeof(uint32_t);

		msg.pokemon=malloc(msg.longPokemon);
		memcpy(msg.pokemon,mensaje->mensaje+offset,msg.longPokemon);


		log_info(logger, "Longitud pokemon: %d\nPokemon: %s\n",msg.longPokemon,msg.pokemon);
		free(msg.pokemon);
		break;
	}
	case LOCALIZED:{
		mensajeLocalized msg;

		memcpy(&(msg.longPokemon),mensaje->mensaje+offset,sizeof(uint32_t));
		offset+=sizeof(uint32_t);

		msg.pokemon=malloc(msg.longPokemon);
		memcpy(msg.pokemon,mensaje->mensaje+offset,msg.longPokemon);
		offset+=msg.longPokemon;

		memcpy(&(msg.listSize),mensaje->mensaje+offset,sizeof(uint32_t));
		offset+=sizeof(uint32_t);

		log_info(logger, "Longitud pokemon: %d\nPokemon: %s\nCantidad de pares de coordenadas: %d\n",msg.longPokemon,msg.pokemon,msg.listSize);

		free(msg.pokemon);
		break;
	}
	default:{
		break;
	}
	}

}
int main(int argc, char** argv) {

	inicializarVariablesGlobales();
	log_info(logger, "Se ha iniciado el cliente gameboy\n");
	pthread_t hiloCountdown;


	proceso destino = definirDestino(argv[1]);
	cola tipoMensaje = definirTipoMensaje(argv[2]);
	int socketDestino = conectarseADestino(destino);

	if(destino == SUSCRIPTOR_CID){
		uint32_t ack = 1;
		uint32_t idProceso = atoi(argv[4]);

		log_debug(logger,"El broker volvio a conectar al proceso con clientID: %d",idProceso);
		suscribirseACola(socketDestino, tipoMensaje,idProceso);
		log_debug(logger,"Se realizó suscripción a la cola %s", getCodeStringByNum(tipoMensaje));

		log_info(loggerOficial,"Se realizó suscripción a la cola %s", getCodeStringByNum(tipoMensaje));

		void tiempoLimiteDeSuscripcion(char * tiempo){
			opCode operacion2 = FINALIZAR;
			sleep(atoi(tiempo));
			int socketFinalizacion = conectarseADestino(destino);
			send(socketFinalizacion,&operacion2,sizeof(opCode),0);
			send(socketFinalizacion,&tipoMensaje,sizeof(cola),0);
			send(socketFinalizacion,&idProceso,sizeof(uint32_t),0);
			log_info(logger,"El gameboy se ha desconectado del broker");
			//close(socketDestino);
			//close(socketFinalizacion);
			//close(socketSuscripcion);
			log_destroy(logger);
			config_destroy(config);
			log_info(logger, "El proceso GameBoy finalizó su ejecución");
			exit(0);
		}

			pthread_create(&hiloCountdown, NULL, (void*) tiempoLimiteDeSuscripcion,
										argv[3]);
			pthread_detach(hiloCountdown);

			while (1) {
				log_info(logger,"Esperando mensajes...");
				mensajeRecibido * mensaje = recibirMensajeDeBroker(socketDestino);
				send(socketDestino, &ack, sizeof(uint32_t), 0);
				imprimirMensaje(mensaje);
				free(mensaje);
			}

	}
	else{
		if (destino == SUSCRIPTOR) {

			uint32_t ack = 1;
			uint32_t idProceso;
			opCode operacion = NUEVA_CONEXION;

			send(socketDestino,&operacion,sizeof(opCode),0);
			recv(socketDestino,&idProceso,sizeof(uint32_t),MSG_WAITALL);
			log_debug(logger,"El broker ha asignado el siguiente ID de proceso: %d",idProceso);

			int socketSuscripcion = conectarseADestino(destino);
			suscribirseACola(socketSuscripcion, tipoMensaje,idProceso);
			log_debug(logger,"Se realizó suscripción a la cola %s", getCodeStringByNum(tipoMensaje));

			log_info(loggerOficial,"Se realizó suscripción a la cola %s", getCodeStringByNum(tipoMensaje));

			void tiempoLimiteDeSuscripcion(char * tiempo){
				opCode operacion2 = FINALIZAR;
				sleep(atoi(tiempo));
				int socketFinalizacion = conectarseADestino(destino);
				send(socketFinalizacion,&operacion2,sizeof(opCode),0);
				send(socketFinalizacion,&tipoMensaje,sizeof(cola),0);
				send(socketFinalizacion,&idProceso,sizeof(uint32_t),0);
				log_info(logger,"El gameboy se ha desconectado del broker");
				//close(socketDestino);
				//close(socketFinalizacion);
				//close(socketSuscripcion);
				log_destroy(logger);
				config_destroy(config);
				log_info(logger, "El proceso GameBoy finalizó su ejecución");
				exit(0);
			}

			pthread_create(&hiloCountdown, NULL, (void*) tiempoLimiteDeSuscripcion,
									argv[3]);
			pthread_detach(hiloCountdown);


			while (1) {
				log_info(logger,"Esperando mensajes...");
				mensajeRecibido * mensaje = recibirMensajeDeBroker(socketSuscripcion);
				send(socketSuscripcion, &ack, sizeof(uint32_t), 0);
				imprimirMensaje(mensaje);
				free(mensaje);

			}

		} else {
			int size;
			estructuraMensaje datosMensaje;
			int offset = 0;
			switch (tipoMensaje) {
			case NEW: {
				mensajeNew mensaje;
				if (destino == BROKER) {
					//./gameboy BROKER NEW_POKEMON [POKEMON] [POSX] [POSY] [CANTIDAD]
					mensaje.longPokemon = strlen(argv[3]);
					mensaje.pokemon = malloc(mensaje.longPokemon);
					strcpy(mensaje.pokemon, argv[3]);
					mensaje.posicionX = atoi(argv[4]);
					mensaje.posicionY = atoi(argv[5]);
					mensaje.cantPokemon = atoi(argv[6]);
					size = sizeof(uint32_t) * 4 + mensaje.longPokemon;
					enviarMensajeABroker(socketDestino, tipoMensaje, -1, size,
							&mensaje);
					log_info(logger,
							"Se envió un mensaje a la cola %s del proceso %s",
							argv[2], argv[1]);
					free(mensaje.pokemon);
				} else {
					//./gameboy GAMECARD NEW_POKEMON [POKEMON] [POSX] [POSY] [CANTIDAD] [ID_MENSAJE]
					mensaje.longPokemon = strlen(argv[3]);
					mensaje.pokemon = malloc(mensaje.longPokemon);
					strcpy(mensaje.pokemon, argv[3]);
					mensaje.posicionX = atoi(argv[4]);
					mensaje.posicionY = atoi(argv[5]);
					mensaje.cantPokemon = atoi(argv[6]);

					datosMensaje.id = atoi(argv[7]);
					datosMensaje.idCorrelativo = -1;
					datosMensaje.sizeMensaje = sizeof(uint32_t) * 4
							+ mensaje.longPokemon;
					datosMensaje.mensaje = malloc(datosMensaje.sizeMensaje);
					memcpy(datosMensaje.mensaje + offset, &(mensaje.longPokemon),
							sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(datosMensaje.mensaje + offset, mensaje.pokemon,
							mensaje.longPokemon);
					offset += mensaje.longPokemon;
					memcpy(datosMensaje.mensaje + offset, &(mensaje.posicionX),
							sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(datosMensaje.mensaje + offset, &(mensaje.posicionY),
							sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(datosMensaje.mensaje + offset, &(mensaje.cantPokemon),
							sizeof(uint32_t));
					offset += sizeof(uint32_t);

					datosMensaje.colaMensajeria = tipoMensaje;
					//datosMensaje.socketSuscriptor = socketDestino;
					enviarMensajeASuscriptor(datosMensaje, socketDestino);

					log_info(logger, "Se envió un mensaje al proceso %s", argv[1]);

					free(datosMensaje.mensaje);
					free(mensaje.pokemon);
				}
				break;
			}
			case APPEARED: {
				mensajeAppeared mensaje;
				if (destino == BROKER) {
					//./gameboy BROKER APPEARED_POKEMON [POKEMON] [POSX] [POSY] [ID_MENSAJE_CORRELATIVO]
					mensaje.longPokemon = strlen(argv[3]) + 1;
					mensaje.pokemon = malloc(mensaje.longPokemon);
					strcpy(mensaje.pokemon, argv[3]);
					mensaje.posicionX = atoi(argv[4]);
					mensaje.posicionY = atoi(argv[5]);
					size = sizeof(uint32_t) * 3 + mensaje.longPokemon;

					enviarMensajeABroker(socketDestino, tipoMensaje, atoi(argv[6]),
							size, &mensaje);
					log_info(logger,
							"Se envió un mensaje a la cola %s del proceso %s",
							argv[2], argv[1]);
					free(mensaje.pokemon);
				} else {
					//./gameboy TEAM APPEARED_POKEMON [POKEMON] [POSX] [POSY]
					mensaje.longPokemon = strlen(argv[3]) + 1;
					mensaje.pokemon = malloc(mensaje.longPokemon);
					strcpy(mensaje.pokemon, argv[3]);
					mensaje.posicionX = atoi(argv[4]);
					mensaje.posicionY = atoi(argv[5]);

					datosMensaje.id = -10;
					datosMensaje.idCorrelativo = -1;
					datosMensaje.sizeMensaje = sizeof(uint32_t) * 3
							+ mensaje.longPokemon;
					datosMensaje.mensaje = malloc(datosMensaje.sizeMensaje);
					memcpy(datosMensaje.mensaje + offset, &(mensaje.longPokemon),
							sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(datosMensaje.mensaje + offset, mensaje.pokemon,
							mensaje.longPokemon);
					offset += mensaje.longPokemon;
					memcpy(datosMensaje.mensaje + offset, &(mensaje.posicionX),
							sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(datosMensaje.mensaje + offset, &(mensaje.posicionY),
							sizeof(uint32_t));
					offset += sizeof(uint32_t);

					datosMensaje.colaMensajeria = tipoMensaje;
					//datosMensaje. = socketDestino;
					enviarMensajeASuscriptor(datosMensaje, socketDestino);
					log_info(logger, "Se envió un mensaje al proceso %s", argv[1]);

					free(datosMensaje.mensaje);
					free(mensaje.pokemon);
				}
				break;
			}
			case CATCH: {
				mensajeCatch mensaje;
				if (destino == BROKER) {
					//./gameboy BROKER CATCH_POKEMON [POKEMON] [POSX] [POSY]
					mensaje.longPokemon = strlen(argv[3]) + 1;
					mensaje.pokemon = malloc(mensaje.longPokemon);
					strcpy(mensaje.pokemon, argv[3]);
					mensaje.posicionX = atoi(argv[4]);
					mensaje.posicionY = atoi(argv[5]);
					size = sizeof(uint32_t) * 3 + mensaje.longPokemon;

					enviarMensajeABroker(socketDestino, tipoMensaje, -1, size,
							&mensaje);
					log_info(logger,
							"Se envió un mensaje a la cola %s del proceso %s",
							argv[2], argv[1]);

					free(mensaje.pokemon);
				} else {
					//./gameboy GAMECARD CATCH_POKEMON [POKEMON] [POSX] [POSY] [ID_MENSAJE]
					mensaje.longPokemon = strlen(argv[3]) + 1;
					mensaje.pokemon = malloc(mensaje.longPokemon);
					strcpy(mensaje.pokemon, argv[3]);
					mensaje.posicionX = atoi(argv[4]);
					mensaje.posicionY = atoi(argv[5]);
					datosMensaje.id = atoi(argv[6]);
					datosMensaje.idCorrelativo = -1;
					datosMensaje.sizeMensaje = sizeof(uint32_t) * 3
							+ mensaje.longPokemon;
					datosMensaje.mensaje = malloc(datosMensaje.sizeMensaje);
					memcpy(datosMensaje.mensaje + offset, &(mensaje.longPokemon),
							sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(datosMensaje.mensaje + offset, mensaje.pokemon,
							mensaje.longPokemon);
					offset += mensaje.longPokemon;
					memcpy(datosMensaje.mensaje + offset, &(mensaje.posicionX),
							sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(datosMensaje.mensaje + offset, &(mensaje.posicionY),
							sizeof(uint32_t));
					offset += sizeof(uint32_t);

					datosMensaje.colaMensajeria = tipoMensaje;
					//datosMensaje.socketSuscriptor = socketDestino;
					enviarMensajeASuscriptor(datosMensaje, socketDestino);
					log_info(logger, "Se envió un mensaje al proceso %s", argv[1]);

					free(datosMensaje.mensaje);
					free(mensaje.pokemon);
				}
				break;
			}
			case CAUGHT: {
				mensajeCaught mensaje;
				if (destino == BROKER) {
					//./gameboy BROKER CAUGHT_POKEMON [ID_MENSAJE_CORRELATIVO] [OK/FAIL]
					mensaje.resultado = obtenerResultadoDesdeCadena(argv[4]);
					size = sizeof(uint32_t);
					enviarMensajeABroker(socketDestino, tipoMensaje, atoi(argv[3]),
							size, &mensaje);
					log_info(logger,
							"Se envió un mensaje a la cola %s del proceso %s",
							argv[2], argv[1]);
				} else {
					//No esta definido este mensaje para suscriptores en el gameboy
				}
				break;
			}
			case GET: {
				mensajeGet mensaje;
				if (destino == BROKER) {
					//./gameboy BROKER GET_POKEMON [POKEMON]
					mensaje.longPokemon = strlen(argv[3]) + 1;
					mensaje.pokemon = malloc(mensaje.longPokemon);
					strcpy(mensaje.pokemon, argv[3]);
					size = sizeof(uint32_t) + mensaje.longPokemon;

					enviarMensajeABroker(socketDestino, tipoMensaje, -1, size,
							&mensaje);
					log_info(logger,
							"Se envió un mensaje a la cola %s del proceso %s",
							argv[2], argv[1]);

					free(mensaje.pokemon);
				} else {
					//./gameboy GAMECARD GET_POKEMON [POKEMON] [ID_MENSAJE]

					mensaje.longPokemon = strlen(argv[3]) + 1;
					mensaje.pokemon = malloc(mensaje.longPokemon);
					strcpy(mensaje.pokemon, argv[3]);

					datosMensaje.id = atoi(argv[4]);
					datosMensaje.idCorrelativo = -1;
					datosMensaje.sizeMensaje = sizeof(uint32_t)
							+ mensaje.longPokemon;
					datosMensaje.mensaje = malloc(datosMensaje.sizeMensaje);
					memcpy(datosMensaje.mensaje + offset, &(mensaje.longPokemon),
							sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(datosMensaje.mensaje + offset, mensaje.pokemon,
							mensaje.longPokemon);
					offset += mensaje.longPokemon;

					datosMensaje.colaMensajeria = tipoMensaje;
					//datosMensaje.socketSuscriptor = socketDestino;
					enviarMensajeASuscriptor(datosMensaje, socketDestino);
					log_info(logger, "Se envió un mensaje al proceso %s", argv[1]);

					free(datosMensaje.mensaje);
					free(mensaje.pokemon);
				}
				break;
			}
			case LOCALIZED: {
				mensajeLocalized mensaje;
				//./broker BROKER LOCALIZED_POKEMON [POKEMON] [CANTIDAD] [POSX] [POSY] (UN PAR DE COORDENADAS X CANTIDAD)
				mensaje.longPokemon = strlen(argv[3]) + 1;
				mensaje.pokemon = malloc(mensaje.longPokemon);
				strcpy(mensaje.pokemon, argv[3]);
				mensaje.listSize = atoi(argv[4]);

				posicYCant *parDeCoordenadas = malloc(sizeof(posicYCant));

				//Funcion interna para poder usar commons con mas parametros
				bool coordenadaEstaEnLista(void* coordenada){
					posicYCant* coordenadaEnLista = coordenada;
						return (coordenadaEnLista->posicionX == parDeCoordenadas->posicionX &&
								coordenadaEnLista->posicionY == parDeCoordenadas->posicionY);
				}

				for(int i=0;i<mensaje.listSize;i++){
					int cont=0;
					parDeCoordenadas->posicionX = atoi(argv[5+cont]);
					parDeCoordenadas->posicionY = atoi(argv[6+cont]);

					//Chequeo si hay mas de un pokemon en la misma posicion
					if(list_any_satisfy(mensaje.posicionYCant,coordenadaEstaEnLista)){
						posicYCant* posicionAModificarCantidad =list_find(mensaje.posicionYCant,coordenadaEstaEnLista);
						posicionAModificarCantidad->cantidad++;
					}
					else{
						parDeCoordenadas->cantidad = 1;
						list_add(mensaje.posicionYCant,parDeCoordenadas);
					}
					cont=cont+2;
				}
				size = sizeof(uint32_t) * 8 + mensaje.longPokemon;
				enviarMensajeABroker(socketDestino, tipoMensaje, -1, size,
						&mensaje);
				log_info(logger,
						"Se envió un mensaje a la cola %s del proceso %s",
						argv[2], argv[1]);
				free(mensaje.pokemon);
				free(parDeCoordenadas);
				break;
			}
			default: {
				log_error(logger, "[ERROR]");
				log_error(logger, "No se pudo determinar el tipo de mensaje");
				break;
			}
			}
		}
	}

	log_info(logger, "El proceso GameBoy finalizó su ejecución");

	close(socketDestino);
	log_destroy(logger);
	log_destroy(loggerOficial);
	config_destroy(config);

	return 0;

}
