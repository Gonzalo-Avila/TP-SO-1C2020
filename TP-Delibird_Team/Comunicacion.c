#include "Team.h"

/*uint32_t obtenerIdDelProceso(char* ip, char* puerto) {
	int socketBroker = crearConexionCliente(ip, puerto);
	uint32_t idProceso;

	opCode codigoOP = NUEVA_CONEXION;
	send(socketBroker, &codigoOP, sizeof(opCode), 0);
	recv(socketBroker, &idProceso, sizeof(uint32_t), MSG_WAITALL);
	close(socketBroker);

	return idProceso;
}*/

void enviarGetDePokemon(char *ip, char *puerto, char *pokemon) {
	int *socketBroker = malloc(sizeof(int));
	*socketBroker = crearConexionCliente(ip, puerto, tiempoDeEspera);
	uint32_t idRespuesta;

	mensajeGet *msg = malloc(sizeof(mensajeGet));

	msg->longPokemon = strlen(pokemon) + 1;
	msg->pokemon = malloc(msg->longPokemon);
	strcpy(msg->pokemon, pokemon);

	log_debug(logger,"Enviando mensaje GET...");
	enviarMensajeABroker(*socketBroker, GET, -1, sizeof(uint32_t) + msg->longPokemon, msg);
	recv(*socketBroker,&idRespuesta,sizeof(uint32_t),MSG_WAITALL);
	log_debug(logger,"Mensaje enviado GET :smilieface:");
	log_info(logger, "Respuesta del Broker: %d", idRespuesta);

	free(msg);
	close(*socketBroker);
	free(socketBroker);
}

bool validarIDCorrelativoCatch(uint32_t id){

	bool esUnIDCatch(void* elemento) {
		t_catchEnEspera* unCatch = elemento;

		if (unCatch->idCorrelativo == id) {
			return true;
		}
		return false;
	}

	return list_any_satisfy(idsDeCatch, esUnIDCatch);
}


t_catchEnEspera* buscarCatch(uint32_t idCorrelativo){

	bool encontrarCatch(void* elemento){
		t_catchEnEspera* unCatch = elemento;

		if(unCatch->idCorrelativo == idCorrelativo){
			return true;
		}

		return false;
	}

	return list_find(idsDeCatch, encontrarCatch);
}

//Nico | Ver si tb tenemos que borrar el objetivo de los objetivos del Team

/*void borrarPokemonDeObjetivos(char* pokemonAtrapado, t_list* objetivos){

	bool encontrarPokemonEnObjetivos(void* pokemon){
		if(strcmp((char*)pokemon, pokemonAtrapado) == 0){
			return true;
		}
		return false;
	}

	void* pokemonABorrar = list_remove_by_condition(objetivos, encontrarPokemonEnObjetivos);

	free(pokemonABorrar);

}*/ //Nico | Queda sin uso. Lo dejo por si sirve para mas adelante

//TODO Ver si resultado va a ser un bool, en ese caso habria que cambiar la utils tb.
//Por ahora esta hecho tomando resultado como booleano
void procesarObjetivoCumplido(t_catchEnEspera* catchProcesado, uint32_t resultado){
	if(resultado){
		t_list* pokemonesDelEntrenador = catchProcesado->entrenadorConCatch->pokemones;
		char* pokemonAtrapado = catchProcesado->entrenadorConCatch->pokemonAAtrapar.pokemon;

		enlistar(pokemonAtrapado, pokemonesDelEntrenador);
		//borrarPokemonDeObjetivos(pokemonAtrapado, objetivosDelEntrenador);
		//TODO falta remover el porkemon atrapado del mapa interno.

		log_info(logger, "El entrenador %d capturo un %s!", catchProcesado->entrenadorConCatch->id,
				catchProcesado->entrenadorConCatch->pokemonAAtrapar.pokemon);
	}
	else{
		log_info(logger, "El entrenador %d no pudo capturar un %s :(.", catchProcesado->entrenadorConCatch->id,
				catchProcesado->entrenadorConCatch->pokemonAAtrapar.pokemon);
	}

	catchProcesado->entrenadorConCatch->suspendido = false;
}

//TODO Ver si tiene que devolver el ID de rta
void enviarCatchDePokemon(char *ip, char *puerto, t_entrenador* entrenador) {
	int *socketBroker = malloc(sizeof(int));
	*socketBroker = crearConexionCliente(ip, puerto, tiempoDeEspera);
	uint32_t idRespuesta;

	mensajeCatch *msg = malloc(sizeof(mensajeCatch));

	msg->longPokemon = strlen(entrenador->pokemonAAtrapar.pokemon) + 1;
	msg->pokemon = malloc(msg->longPokemon);
	strcpy(msg->pokemon, entrenador->pokemonAAtrapar.pokemon);
	msg->posicionX = entrenador->pokemonAAtrapar.pos[0];
	msg->posicionY = entrenador->pokemonAAtrapar.pos[1];

	log_debug(logger,"Enviando mensaje CATCH...");
	enviarMensajeABroker(*socketBroker, CATCH, -1, sizeof(uint32_t)*3 + msg->longPokemon, msg);
	recv(*socketBroker,&idRespuesta,sizeof(uint32_t),MSG_WAITALL);                              //Recibo el ID que envia automaticamente el Broker
	log_debug(logger,"Mensaje enviado CATCH :smilieface:");

	//Me guardo el ID del CATCH. Es necesario para procesar el CAUGHT
	t_catchEnEspera* elIdCorrelativo = malloc(sizeof(t_catchEnEspera));
	elIdCorrelativo->idCorrelativo = idRespuesta;
	elIdCorrelativo->entrenadorConCatch = entrenador;

	log_info(logger, "ID del CATCH recibido y guardado: %d", idRespuesta);


	list_add(idsDeCatch, elIdCorrelativo);

	free(msg);
	close(*socketBroker);
	free(socketBroker);
}

/* Atender al Broker y Gameboy */
void atenderServidor(int *socketServidor) {
	log_debug(logger, "Se levanta hilo para atender al servidor en socket %d",*socketServidor);
	uint32_t ack=1;
	while (1) {
		mensajeRecibido *miMensajeRecibido = recibirMensajeDeBroker(*socketServidor);

		if (miMensajeRecibido->codeOP == FINALIZAR) {
			break;
		}
		send(*socketServidor, &ack, sizeof(uint32_t), 0);
		if(miMensajeRecibido->codeOP > 0 && miMensajeRecibido->codeOP <= 6) {
				switch(miMensajeRecibido->colaEmisora){
				//TODO - Corregir recepciones, separar el void y acomodarPokemonRecibido.
					case APPEARED:{
						log_debug(logger, "Se recibio un APPEARED");

						char *pokemonRecibido = malloc((miMensajeRecibido->sizeMensaje)+1);
						int offset = (miMensajeRecibido->sizePayload) - (miMensajeRecibido->sizeMensaje);

						memcpy(pokemonRecibido, miMensajeRecibido + offset,sizeof(miMensajeRecibido->sizeMensaje));

						if (estaEnLosObjetivos(pokemonRecibido)){
							log_debug(logger,"El pokemon esta en nuestro objetivo");
							log_info(logger, "Pokemon: %s", (char*)pokemonRecibido);
							enviarGetDePokemon(ipServidor, puertoServidor, pokemonRecibido);
						}

						free(pokemonRecibido);
						break;
					}
					case LOCALIZED:{
						log_debug(logger,"Se recibio un LOCALIZED");

						int cantPokes,longPokemon;
						int offset = 0;

						t_posicionEnMapa *pos = malloc(sizeof(t_posicionEnMapa));
						pos->x = list_create();
						pos->y = list_create();
						pos->cantidades = list_create();

						memcpy(&longPokemon,miMensajeRecibido->mensaje,sizeof(uint32_t));
						offset = sizeof(uint32_t);

						char *pokemon = malloc(longPokemon);
						memcpy(pokemon,miMensajeRecibido->mensaje + offset,longPokemon);

						offset += longPokemon;
						memcpy(&cantPokes,miMensajeRecibido->mensaje + offset,sizeof(uint32_t));

						pos->pokemon = malloc(longPokemon);

						offset += sizeof(uint32_t);
						int x[cantPokes],y[cantPokes],cant[cantPokes];

						memcpy(pos->pokemon,pokemon,longPokemon);

						log_debug(logger,"Pokemon: %s",pos->pokemon);

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

						if(estaEnLosObjetivos(pokemon)){

							log_debug(logger,"El pokemon %s es un objetivo",pokemon);

							list_add(listaPosicionesInternas,pos);
							ponerEnReadyAlMasCercano(x[0],y[0], pokemon);

							sem_post(&procesoEnReady);
							//le aviso al planificador que pase un entrenador a ready.
						}

						log_debug(logger,"Se proceso el LOCALIZED");

						break;
					}
					case CAUGHT:{
						//TODO CAUGHT
						estructuraMensaje* miMensajeCaught = miMensajeRecibido->mensaje;

						//Si no envie yo el CATCH, no me interesa el CAUGHT
						if(validarIDCorrelativoCatch(miMensajeCaught->idCorrelativo)) {
							log_debug(logger, "Se recibio un CAUGHT");

							mensajeCaught* miCaught = miMensajeCaught->mensaje;

							procesarObjetivoCumplido(buscarCatch(miMensajeCaught->idCorrelativo), miCaught->resultado);
						}

						break;
					}
					default:{
						log_error(logger, "Cola de Mensaje Erronea.");
						break;
					}
				}
			}
			else {
				log_error(logger, "Se perdio la conexión con el Broker.");
				close(*socketServidor);
				log_debug(logger, "Reintentando conexion...");
				*socketServidor = crearConexionCliente(ipServidor, puertoServidor, tiempoDeEspera);
			}
		if(miMensajeRecibido->mensaje != NULL){
			log_info(logger, "Mensaje recibido: %s\n", miMensajeRecibido->mensaje);
		}

		free(miMensajeRecibido);
	}
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
