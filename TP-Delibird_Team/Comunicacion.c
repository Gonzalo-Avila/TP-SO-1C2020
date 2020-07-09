#include "Team.h"

void enviarGetDePokemon(char *ip, char *puerto, char *pokemon) {
	int *socketBroker = malloc(sizeof(int));
	*socketBroker = crearConexionClienteConReintento(ip, puerto, tiempoDeEspera);
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

	free(msg->pokemon);
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

		return unCatch->idCorrelativo == idCorrelativo;
	}

	return list_find(idsDeCatch, encontrarCatch);
}

void procesarObjetivoCumplido(t_catchEnEspera* catchProcesado, uint32_t resultado){

	if(resultado){
		t_list* pokemonesDelEntrenador = catchProcesado->entrenadorConCatch->pokemones;
		char* pokemonAtrapado = catchProcesado->entrenadorConCatch->pokemonAAtrapar.pokemon;

		sem_wait(&mutexEntrenadores);
		enlistar(pokemonAtrapado, pokemonesDelEntrenador);
		sem_post(&mutexEntrenadores);

		bool esUnObjetivo(void *objetivo){
			bool verifica = false;

			if(string_equals_ignore_case((char *)objetivo, pokemonAtrapado))
				verifica = true;

			return verifica;
		}

		sem_wait(&mutexOBJETIVOS);
		list_remove_by_condition(team->objetivo,esUnObjetivo);
		sem_post(&mutexOBJETIVOS);

		list_remove_by_condition(catchProcesado->entrenadorConCatch->objetivos,esUnObjetivo);

		log_info(logger, "El entrenador %d capturo un %s!", catchProcesado->entrenadorConCatch->id,
				catchProcesado->entrenadorConCatch->pokemonAAtrapar.pokemon);

		log_info(logger, "El entrenador removio el pokemon %s de sus objetivos si lo tuviese",pokemonAtrapado);
	}
	else{
		log_info(logger, "El entrenador %d no pudo capturar un %s :(.", catchProcesado->entrenadorConCatch->id,
				catchProcesado->entrenadorConCatch->pokemonAAtrapar.pokemon);
	}


	//Marca objetivos cumplidos de entrenador.
	seCumplieronLosObjetivosDelEntrenador(catchProcesado->entrenadorConCatch);

	//Verifica si estan en deadlock, SOLO cuando se acabaron los objetivos generales.
	verificarDeadlock();

	sem_wait(&mutexEntrenadores);
	catchProcesado->entrenadorConCatch->suspendido = false;
	sem_post(&mutexEntrenadores);

	log_info(logger,"El entrenador %d tiene los sig objetivos:",catchProcesado->entrenadorConCatch->id);
	imprimirListaDeCadenas(catchProcesado->entrenadorConCatch->objetivos);
	log_info(logger,"El entrenador %d tiene los sig pokemones:",catchProcesado->entrenadorConCatch->id);
	imprimirListaDeCadenas(catchProcesado->entrenadorConCatch->pokemones);
	log_info(logger,"El entrenador %d tiene el siguien estado: %d",catchProcesado->entrenadorConCatch->id,catchProcesado->entrenadorConCatch->estado);
}

void enviarCatchDePokemon(char *ip, char *puerto, t_entrenador* entrenador) {

	if(brokerConectado){

		sem_wait(&mutexOBJETIVOS);

		if(estaEnLosObjetivos(entrenador->pokemonAAtrapar.pokemon)){
			int *socketBroker = malloc(sizeof(int));
			*socketBroker = crearConexionClienteConReintento(ip, puerto, tiempoDeEspera);
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

			//Me guardo el ID del CATCH. Es necesario para procesar el CAUGHT
			t_catchEnEspera* elIdCorrelativo = malloc(sizeof(t_catchEnEspera));
			elIdCorrelativo->idCorrelativo = idRespuesta;
			elIdCorrelativo->entrenadorConCatch = entrenador;

			list_add(idsDeCatch, elIdCorrelativo);

			free(msg->pokemon);
			free(msg);
			close(*socketBroker);
			free(socketBroker);
		}
		else{
			entrenador->suspendido = false;
		}

		sem_post(&mutexOBJETIVOS);

	}
}

void procesarCAUGHT(mensajeRecibido* miMensajeRecibido) {
	sem_wait(&mutexCAUGHT);

	if (validarIDCorrelativoCatch(miMensajeRecibido->idCorrelativo)) {
		log_debug(logger, "Se recibio un CAUGHT");
		mensajeCaught* miCaught = malloc(sizeof(mensajeCaught));
		memcpy(&miCaught->resultado,miMensajeRecibido->mensaje,sizeof(resultado));
		procesarObjetivoCumplido(buscarCatch(miMensajeRecibido->idCorrelativo),miCaught->resultado);
		free(miCaught);
	}
	else{
		log_debug(logger, "Llego un CAUGHT pero no nos sirve");
	}

	free(miMensajeRecibido->mensaje);
	free(miMensajeRecibido);

	sem_post(&mutexCAUGHT);
}

void procesarLOCALIZED(mensajeRecibido* miMensajeRecibido) {
	sem_wait(&mutexLOCALIZED);

	log_debug(logger, "Se recibio un LOCALIZED");
	uint32_t cantPokes, longPokemon;
	int offset = 0;

	memcpy(&longPokemon, miMensajeRecibido->mensaje, sizeof(uint32_t));
	offset = sizeof(uint32_t);

	char* pokemon = malloc(longPokemon);
	memcpy(pokemon, miMensajeRecibido->mensaje + offset, longPokemon);
	offset += longPokemon;

	pokemon[longPokemon]='\0';

	log_debug(logger, "Pokemon: %s", pokemon);

	memcpy(&cantPokes, miMensajeRecibido->mensaje + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);


	if (estaEnLosObjetivos(pokemon) && cantPokes>0) {
		log_debug(logger, "El pokemon %s es un objetivo", pokemon);

		t_posicionEnMapa* posicion = malloc(sizeof(t_posicionEnMapa));
		for (int i = 0; i < cantPokes; i++) {
			posicion->pokemon = pokemon;

			memcpy(&(posicion->pos[0]), miMensajeRecibido->mensaje + offset,sizeof(uint32_t));
			offset += sizeof(uint32_t);

			memcpy(&(posicion->pos[1]), miMensajeRecibido->mensaje + offset,sizeof(uint32_t));
			offset += sizeof(uint32_t);

			list_add(listaPosicionesInternas, posicion);
		}

		ponerEnReadyAlMasCercano(posicion->pos[0], posicion->pos[1],pokemon);
		sem_post(&procesoEnReady);
	}
	else{
		log_debug(logger, "El pokemon no me interesa o llego vacio");
	}
	log_debug(logger, "Se proceso el LOCALIZED");

	free(miMensajeRecibido->mensaje);
	free(miMensajeRecibido);

	sem_post(&mutexLOCALIZED);
}

void procesarAPPEARED(mensajeRecibido* miMensajeRecibido) {
	sem_wait(&mutexAPPEARED);

	log_debug(logger, "Se recibio un APPEARED");
	char * pokemonRecibido;
	uint32_t longPokemon;
	int offset=0;

	memcpy(&longPokemon, miMensajeRecibido->mensaje+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	pokemonRecibido=malloc(longPokemon+1);
	memcpy(pokemonRecibido,miMensajeRecibido->mensaje+offset,longPokemon);
	pokemonRecibido[longPokemon]='\0';
	offset += longPokemon;

	if (estaEnLosObjetivos(pokemonRecibido)) {
		log_debug(logger, "El pokemon esta en nuestro objetivo");
		log_info(logger, "Pokemon: %s", pokemonRecibido);

		t_posicionEnMapa* posicion = malloc(sizeof(t_posicionEnMapa));
		posicion->pokemon = pokemonRecibido;

		memcpy(&(posicion->pos[0]), miMensajeRecibido->mensaje+offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		memcpy(&(posicion->pos[1]), miMensajeRecibido->mensaje+offset, sizeof(uint32_t));

		list_add(listaPosicionesInternas, posicion);

		ponerEnReadyAlMasCercano(posicion->pos[0], posicion->pos[1],pokemonRecibido);

		sem_post(&procesoEnReady);
	}
	else{
		log_error(logger,"No me interesa el pokemon %s",pokemonRecibido);
	}

	free(miMensajeRecibido->mensaje);
	free(miMensajeRecibido);

	sem_post(&mutexAPPEARED);
}

void enviarACK(int* socketServidor) {
	uint32_t ack=1;
	send(*socketServidor, &ack, sizeof(uint32_t), 0);
}

void levantarHiloDeRecepcionAPPEARED(mensajeRecibido* miMensajeRecibido){
	pthread_t hiloAPPEARED;

	pthread_create(&hiloAPPEARED, NULL, (void*) procesarAPPEARED, miMensajeRecibido);
	pthread_detach(hiloAPPEARED);
}

void levantarHiloDeRecepcionLOCALIZED(mensajeRecibido* miMensajeRecibido){
	pthread_t hiloLOCALIZED;

	pthread_create(&hiloLOCALIZED, NULL, (void*) procesarLOCALIZED, miMensajeRecibido);
	pthread_detach(hiloLOCALIZED);
}

void levantarHiloDeRecepcionCAUGHT(mensajeRecibido* miMensajeRecibido){
	pthread_t hiloCAUGHT;

	pthread_create(&hiloCAUGHT, NULL, (void*) procesarCAUGHT, miMensajeRecibido);
	pthread_detach(hiloCAUGHT);
}

/* Atender al Broker y Gameboy */
void atenderServidor(int *socketServidor) {
	log_debug(logger, "Se levanta hilo para atender al servidor en socket %d",*socketServidor);

	while (1) {
		mensajeRecibido *miMensajeRecibido = recibirMensajeDeBroker(*socketServidor);

		if (miMensajeRecibido->codeOP == FINALIZAR) {
			break;
		}

		enviarACK(socketServidor);
		if(miMensajeRecibido->codeOP > 0 && miMensajeRecibido->codeOP <= 6) {
				switch(miMensajeRecibido->colaEmisora){
					case APPEARED:{
						levantarHiloDeRecepcionAPPEARED(miMensajeRecibido);
						break;
					}
					case LOCALIZED:{
						levantarHiloDeRecepcionLOCALIZED(miMensajeRecibido);
						break;
					}
					case CAUGHT:{
						levantarHiloDeRecepcionCAUGHT(miMensajeRecibido);
						break;
					}
					default:{
						log_error(logger, "Cola de Mensaje Erronea.");
						break;
					}
				}
			}
			else {
				brokerConectado = false;
				log_error(logger, "Se perdio la conexión con el Broker.");
				close(*socketServidor);
				log_debug(logger, "Reintentando conexion...");
				*socketServidor = crearConexionClienteConReintento(ipServidor, puertoServidor, tiempoDeEspera);
			}

		if(miMensajeRecibido->mensaje != NULL){
			log_info(logger, "Mensaje recibido: %s\n", miMensajeRecibido->mensaje);
		}

	}
}

void crearHiloParaAtenderServidor(int *socketServidor) {
	pthread_t hiloAtenderServidor;
	pthread_create(&hiloAtenderServidor, NULL, (void*) atenderServidor,
				socketServidor);
	pthread_detach(hiloAtenderServidor);
}

void crearHilosParaAtenderBroker() {
	crearHiloParaAtenderServidor(socketBrokerApp);
	crearHiloParaAtenderServidor(socketBrokerLoc);
	crearHiloParaAtenderServidor(socketBrokerCau);
}

void crearConexion(int* socket){
	*socket = crearConexionClienteConReintento(ipServidor, puertoServidor, tiempoDeEspera);
	log_debug(logger, "Conexion creada. Socket = %d", *socket);
}

//Obtengo el ID del proceso

void obtenerID(){
	idDelProceso = obtenerIdDelProcesoConReintento(ipServidor, puertoServidor, tiempoDeEspera);
}

/* Se suscribe a las colas del Broker */
void crearConexionesYSuscribirseALasColas() {
	//pthread_t hiloObtenerID;
	pthread_t hiloSocketLoc;
	pthread_t hiloSocketApp;
	pthread_t hiloSocketCau;

	//pthread_create(&hiloObtenerID, NULL, (void*) obtenerID, &idDelProceso); //No funciona pq le paso la direccion y la funcion recibe un entero. TODO - Revisar
	obtenerID();
	pthread_create(&hiloSocketLoc, NULL, (void*) crearConexion, socketBrokerLoc);
	pthread_create(&hiloSocketApp, NULL, (void*) crearConexion, socketBrokerApp);
	pthread_create(&hiloSocketCau, NULL, (void*) crearConexion, socketBrokerCau);

	//Espero a que el socket este conectado antes de utilizarlo
	pthread_join(hiloSocketLoc, NULL);
	suscribirseACola(*socketBrokerLoc,LOCALIZED, idDelProceso);
	crearHiloParaAtenderServidor(socketBrokerLoc);

	log_debug(logger, "Suscripto a la cola LOCALIZED.");

	pthread_join(hiloSocketApp, NULL);
	suscribirseACola(*socketBrokerApp,APPEARED, idDelProceso);
	crearHiloParaAtenderServidor(socketBrokerApp);

	log_debug(logger, "Suscripto a la cola APPEARED.");

	pthread_join(hiloSocketCau, NULL);
	suscribirseACola(*socketBrokerCau,CAUGHT, idDelProceso);
	crearHiloParaAtenderServidor(socketBrokerCau);

	log_debug(logger, "Suscripto a la cola CAUGHT.");

	brokerConectado = true;
	sem_post(&conexionCreada);

}

void crearConexionEscuchaGameboy(int* socketGameboy) {

	char * ipEscucha = malloc(strlen(config_get_string_value(config, "IP_TEAM")) + 1);
	ipEscucha = config_get_string_value(config, "IP_TEAM");

	char * puertoEscucha = malloc(strlen(config_get_string_value(config, "PUERTO_TEAM")) + 1);
	puertoEscucha = config_get_string_value(config, "PUERTO_TEAM");

	*socketGameboy = crearConexionServer(ipEscucha, puertoEscucha);

	free(ipEscucha);
	free(puertoEscucha);
	
	log_info(logger, "Socket de escucha para Gameboy creado");

	atenderGameboy(socketGameboy);
}

void conectarGameboy(){
	pthread_t hiloConectarGameboy;

	pthread_create(&hiloConectarGameboy, NULL,(void*) crearConexionEscuchaGameboy, socketGameboy);
}

void atenderGameboy(int *socketEscucha) {
	int backlogGameboy = config_get_int_value(config, "BACKLOG_GAMEBOY");
	atenderConexionEn(*socketEscucha, backlogGameboy);

	while (1) {
		log_debug(logger, "Esperando cliente...");
		int *socketCliente = esperarCliente(*socketEscucha);
		log_info(logger, "Se ha conectado un cliente. Número de socket cliente: %d", *socketCliente);
		esperarMensajesGameboy(socketCliente);
		free(socketCliente);
		}
}

void esperarMensajesGameboy(int* socketSuscripcion) {

	log_info(logger,"[GAMEBOY] Recibiendo mensaje del socket cliente: %d",*socketSuscripcion);
	mensajeRecibido * mensaje = recibirMensajeDeBroker(*socketSuscripcion);
	log_info(logger,"[GAMEBOY] Mensaje recibido");

	//imprimirContenido(mensaje, socketSuscripcion);

	switch (mensaje->colaEmisora) {
	case APPEARED: {
		//Procesar mensaje NEW
		log_debug(logger, "[GAMEBOY] Llegó un mensaje de la cola APPEARED");
		procesarAPPEARED(mensaje);
		break;
	}
	case LOCALIZED: {
		//Procesar mensaje GET
		log_debug(logger, "[GAMEBOY] Llegó un mensaje de la cola LOCALIZED");
		procesarLOCALIZED(mensaje);

		break;
	}
	case CAUGHT: {
		//Procesar mensaje CATCH
		log_debug(logger, "[GAMEBOY] Llegó un mensaje de la cola CAUGHT");
		procesarCAUGHT(mensaje);
		break;
	}
	default: {

		log_error(logger, "[GAMEBOY] Mensaje recibido de una cola no correspondiente");
		break;
	}

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
	sem_wait(&conexionCreada);
	char *pokemon;

	sem_wait(&mutexOBJETIVOS);
	for (int i = 0; i < list_size(team->objetivo); i++) {
		pokemon = list_get(team->objetivo, i);
		enviarGetDePokemon(ip, puerto, pokemon);
	}
	sem_post(&mutexOBJETIVOS);
}
