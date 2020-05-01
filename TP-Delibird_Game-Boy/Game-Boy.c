#include "Game-Boy.h"
//#include "Team.h"

void inicializarVariablesGlobales() {
	config = config_create("gameboy.config");
	logger = log_create("gameboy_logs", "GameBoy", 1, LOG_LEVEL_TRACE);
}

int conectarseADestino(proceso destino) {

	char * ipDestino = malloc(16);
	char * puertoDestino = malloc(6);
	char * proceso = malloc(10);
	int socketDestino;

	switch (destino) {
	case SUSCRIPTOR:
	case BROKER: {
		ipDestino = config_get_string_value(config, "IP_BROKER");
		puertoDestino = config_get_string_value(config, "PUERTO_BROKER");
		strcpy(proceso, "BROKER");
		break;
	}
	case TEAM: {
		ipDestino = config_get_string_value(config, "IP_TEAM");
		puertoDestino = config_get_string_value(config, "PUERTO_TEAM");
		strcpy(proceso, "TEAM");
		break;
	}
	case GAMECARD: {
		ipDestino = config_get_string_value(config, "IP_TEAM");
		puertoDestino = config_get_string_value(config, "PUERTO_TEAM");
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
	log_error(logger, "[ERROR]");
	log_error(logger, "No se pudo determinar el proceso destino");
	return -1;

}

int main(int argc, char** argv) {

	//Se setean todos los datos
	inicializarVariablesGlobales();
	log_info(logger, "Se ha iniciado el cliente gameboy\n");

	proceso destino = definirDestino(argv[1]);
	cola tipoMensaje = definirTipoMensaje(argv[2]);
	int socketDestino = conectarseADestino(destino);

	if (destino == SUSCRIPTOR) {
		suscribirseACola(socketDestino, tipoMensaje);
		while (sleep(atoi(argv[4])) != 0) {
			log_info(logger, "Paso un segundo");
			//Recibir mensaje
			//Imprimir mensaje
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
				mensaje.longPokemon = strlen(argv[3]) + 1;
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
				mensaje.longPokemon = strlen(argv[3]) + 1;
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
				enviarMensajeASuscriptor(socketDestino, tipoMensaje,
						datosMensaje);

				log_info(logger, "Se envió un mensaje al proceso %s", argv[1]);

				free(datosMensaje.mensaje);
				free(mensaje.pokemon);
			}
			break;
		}
		case APPEARED: {
			mensajeAppeared mensaje;
			if (destino == BROKER) {
				//./gameboy BROKER APPEARED_POKEMON [POKEMON] [POSX] [POSY] [ID_MENSAJE]
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

				enviarMensajeASuscriptor(socketDestino, tipoMensaje,
						datosMensaje);
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

				enviarMensajeASuscriptor(socketDestino, tipoMensaje,
						datosMensaje);
				log_info(logger, "Se envió un mensaje al proceso %s", argv[1]);

				free(datosMensaje.mensaje);
				free(mensaje.pokemon);
			}
			break;
		}
		case CAUGHT: {
			mensajeCaught mensaje;
			if (destino == BROKER) {
				//./gameboy BROKER CAUGHT_POKEMON [ID_MENSAJE] [OK/FAIL]
				mensaje.resultado = atoi(argv[4]);
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
				strcpy(mensaje.pokemon, argv[4]);
				size = sizeof(uint32_t) + mensaje.longPokemon;

				enviarMensajeABroker(socketDestino, tipoMensaje, -1, size,
						&mensaje);
				log_info(logger,
						"Se envió un mensaje a la cola %s del proceso %s",
						argv[2], argv[1]);

				free(mensaje.pokemon);
			} else {
				//./gameboy GAMECARD GET_POKEMON [POKEMON]
				mensaje.longPokemon = strlen(argv[3]) + 1;
				mensaje.pokemon = malloc(mensaje.longPokemon);
				strcpy(mensaje.pokemon, argv[3]);

				datosMensaje.id = -10;
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

				enviarMensajeASuscriptor(socketDestino, tipoMensaje,
						datosMensaje);
				log_info(logger, "Se envió un mensaje al proceso %s", argv[1]);

				free(datosMensaje.mensaje);
				free(mensaje.pokemon);
			}
			break;
		}
		case LOCALIZED: {
			//Este mensaje no se utiliza desde el gameboy
			break;
		}
		default: {
			log_error(logger, "[ERROR]");
			log_error(logger, "No se pudo determinar el tipo de mensaje");
			break;
		}
		}
	}
	close(socketDestino);
	log_destroy(logger);
	config_destroy(config);
	log_info(logger, "El proceso GameBoy finalizó su ejecución\n");
	return 0;

}
