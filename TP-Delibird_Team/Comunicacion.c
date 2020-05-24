#include "Team.h"

uint32_t obtenerIdDelProceso(char* ip, char* puerto) {
	int socketBroker = crearConexionCliente(ip, puerto);
	uint32_t idProceso;

	opCode codigoOP = NUEVA_CONEXION;
	send(socketBroker, &codigoOP, sizeof(opCode), 0);
	recv(socketBroker, &idProceso, sizeof(uint32_t), MSG_WAITALL);
	close(socketBroker);

	return idProceso;
}

void enviarGetDePokemon(char *ip, char *puerto, char *pokemon) {
	int *socketBroker = malloc(sizeof(int));
	*socketBroker = crearConexionCliente(ip, puerto);
	uint32_t idRespuesta;

	mensajeGet *msg = malloc(sizeof(mensajeGet));

	msg->longPokemon = strlen(pokemon) + 1;
	msg->pokemon = malloc(msg->longPokemon);
	strcpy(msg->pokemon, pokemon);

	log_debug(logger,"Enviando mensaje...");
	enviarMensajeABroker(*socketBroker, GET, -1, sizeof(uint32_t) + msg->longPokemon, msg);
	recv(*socketBroker,&idRespuesta,sizeof(uint32_t),MSG_WAITALL);
	log_debug(logger,"Mensaje enviado :smilieface:");
	log_info(logger, "Respuesta del Broker: %d", idRespuesta);
	free(msg);
	close(*socketBroker);
	free(socketBroker);
}

/* Atender al Broker y Gameboy */
void atenderServidor(int *socketServidor) {
	mensajeRecibido *miMensajeRecibido = malloc(sizeof(mensajeRecibido));

	log_debug(logger, "Se atiende al servidor");
	while (1) {
		miMensajeRecibido = recibirMensajeDeBroker(*socketServidor);

		if (miMensajeRecibido->codeOP == FINALIZAR) {
			break;
		}

		switch(miMensajeRecibido->colaEmisora){
			case APPEARED:{
				void* pokemonRecibido = malloc((miMensajeRecibido->sizeMensaje)+1);
				int offset = (miMensajeRecibido->sizePayload) - (miMensajeRecibido->sizeMensaje);
				memcpy(pokemonRecibido, miMensajeRecibido + offset,sizeof(miMensajeRecibido->sizeMensaje));

				if (list_any_satisfy(team->objetivo, esUnObjetivo)) {
					log_debug(logger, "APPEARED recibido");
					log_info(logger, "Pokemon: %s", *(char*)pokemonRecibido);
					enviarGetDePokemon(ipServidor, puertoServidor, pokemonRecibido);
				}
				free(pokemonRecibido);
				break;
			}
			case LOCALIZED:{
				int cantPokes,longPokemon;
				int offset = 0;
				t_posicionEnMapa *pos = malloc(sizeof(t_posicionEnMapa));

				memcpy(&longPokemon,miMensajeRecibido->mensaje,sizeof(uint32_t));
				offset = sizeof(uint32_t);

				char *pokemon = malloc(longPokemon);
				memcpy(pokemon,miMensajeRecibido->mensaje + offset,longPokemon);

				offset += longPokemon;
				memcpy(&cantPokes,miMensajeRecibido->mensaje + offset,sizeof(uint32_t));

				pos->pokemon = malloc(longPokemon);

				offset += sizeof(uint32_t);
				int x[cantPokes],y[cantPokes],cant[cantPokes];

				for(int i = 0;i < cantPokes;i++){
					memcpy(&x[i],miMensajeRecibido->mensaje + offset,sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(&y[i],miMensajeRecibido->mensaje + offset,sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(&cant[i],miMensajeRecibido->mensaje + offset,sizeof(uint32_t));
					offset += sizeof(uint32_t);

					//los agrego al mapa interno
					list_add(pos->x,&x[i]);
					list_add(pos->y,&y[i]);
					list_add(pos->cantidades,&cant[i]);
				}
				strcpy(pos->pokemon,pokemon);

				if(esUnObjetivo(pokemon)){
					list_add(listaPosicionesInternas,pos);
					ponerEnReadyAlMasCercano(x[0],y[0]);
				}
				break;
			}
			case CAUGHT:
				//TODO CAUGHT
				log_info(logger, "Recibi un CAUGHT. ¿Que es eso?¿Se come?");
				break;
			default:
				log_error(logger, "Cola de Mensaje Erronea.");
				break;

		}
		if(miMensajeRecibido->mensaje != NULL){
			log_info(logger, "Mensaje recibido: %s\n", miMensajeRecibido->mensaje);
		}

		free(miMensajeRecibido);
	}
	close(*socketServidor);
}

void crearHiloParaAtenderServidor(int *socketServidor) {
	pthread_t hiloAtenderServidor;
	pthread_create(&hiloAtenderServidor, NULL, (void*) atenderServidor,
				socketServidor);
	pthread_detach(hiloAtenderServidor);
}

void crearHilosParaAtenderBroker(int *socketBrokerApp, int *socketBrokerLoc, int *socketBrokerCau) {
	crearHiloParaAtenderServidor(socketBrokerApp);
	crearHiloParaAtenderServidor(socketBrokerLoc);
	crearHiloParaAtenderServidor(socketBrokerCau);
}

/* Se suscribe a las colas del Broker */
void suscribirseALasColas(int socketA,int socketL,int socketC, uint32_t idProceso) {
	suscribirseACola(socketA,APPEARED, idProceso);
	suscribirseACola(socketL,LOCALIZED, idProceso);
	suscribirseACola(socketC,CAUGHT, idProceso);
}

int crearConexionEscuchaGameboy() {

	char * ipEscucha = malloc(
			strlen(config_get_string_value(config, "IP_TEAM")) + 1);
	ipEscucha = config_get_string_value(config, "IP_TEAM");

	char * puertoEscucha = malloc(
			strlen(config_get_string_value(config, "PUERTO_TEAM")) + 1);
	puertoEscucha = config_get_string_value(config, "PUERTO_TEAM");

	int socketEscucha = crearConexionServer(ipEscucha, puertoEscucha);

	log_info(logger, "Se ha iniciada la conexión con el Gameboy\n");
	log_info(logger,"El Gameboy ya está conectado y puede enviar un mensaje");


	free(ipEscucha);
	free(puertoEscucha);

	return socketEscucha;

}

void atenderGameboy(int *socketEscucha) {
	int backlogGameboy = config_get_int_value(config, "BACKLOG_GAMEBOY");
	atenderConexionEn(*socketEscucha, backlogGameboy);
	log_debug(logger, "Esperando Gameboy...");

	//Capaz tiene que ir un while(1) acá. Hay que probar
	while(1) {
		int *socketGameboy = esperarCliente(*socketEscucha);
		log_info(logger,"Se ha conectado el Gameboy.");

		crearHiloParaAtenderServidor(socketGameboy);
	}
}

t_mensaje* deserializar(void* paquete) {
	t_mensaje* mensaje = malloc(sizeof(paquete));
	int offset = 0;

	memcpy((void*) mensaje->tipoDeMensaje, paquete, sizeof(int));
	offset += sizeof(int);
	memcpy((void*) mensaje->pokemonSize, paquete + offset, sizeof(int));
	offset += sizeof(int);
	memcpy((void*) mensaje->pokemon, paquete + offset, mensaje->pokemonSize);
	offset += mensaje->pokemonSize;
	if (mensaje->tipoDeMensaje == LOCALIZED
			|| mensaje->tipoDeMensaje == CAUGHT) {
		memcpy((void*) mensaje->posicionX, paquete + offset, sizeof(int));
		offset += sizeof(int);
		memcpy((void*) mensaje->posicionY, paquete + offset, sizeof(int));
	}
	return mensaje;
}

void enviarGetSegunObjetivo(char *ip, char *puerto) {
	char *pokemon = malloc(MAXSIZE);

	for (int i = 0; i < list_size(team->objetivo); i++) {
		pokemon = list_get(team->objetivo, 0);
		enviarGetDePokemon(ip, puerto, pokemon);
	}
	free(pokemon);

}
