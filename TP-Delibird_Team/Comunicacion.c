#include "Team.h"

void enviarGetDePokemon(char *ip, char *puerto, char *pokemon) {
	int *socketBroker = malloc(sizeof(int));
	*socketBroker = crearConexionClienteConReintento(ip, puerto, tiempoDeEspera);
	uint32_t idRespuesta;

	mensajeGet *msg = malloc(sizeof(mensajeGet));

	msg->longPokemon = strlen(pokemon) + 1;
	msg->pokemon = malloc(msg->longPokemon);
	strcpy(msg->pokemon, pokemon);

	enviarMensajeABroker(*socketBroker, GET, -1, sizeof(uint32_t) + msg->longPokemon, msg);
	recv(*socketBroker,&idRespuesta,sizeof(uint32_t),MSG_WAITALL);

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

void loggearPokemonCapturado(t_entrenador* entrenador, bool resultado){
	if(resultado){
	log_info(logger, "El entrenador %d capturo un %s!", entrenador->id,
					entrenador->pokemonAAtrapar.pokemon);
	log_info(loggerOficial, "El entrenador %d capturo un %s en la posicion [%d,%d].",
			entrenador->id, entrenador->pokemonAAtrapar.pokemon,
			entrenador->pos[0], entrenador->pos[1]);
	}
	else{
		log_info(logger, "El entrenador %d no pudo atrapar a %s!", entrenador->id,
							entrenador->pokemonAAtrapar.pokemon);
		log_info(loggerOficial, "El entrenador %d no logro capturar un %s en la posicion [%d,%d].",
				entrenador->id, entrenador->pokemonAAtrapar.pokemon,
				entrenador->pos[0], entrenador->pos[1]);
	}
}

void procesarObjetivoCumplido(t_catchEnEspera* catchProcesado, uint32_t resultado){

	if(resultado){
		t_list* pokemonesDelEntrenador = catchProcesado->entrenadorConCatch->pokemones;
		char* pokemonAtrapado = malloc(strlen(catchProcesado->entrenadorConCatch->pokemonAAtrapar.pokemon) + 1);
		strcpy(pokemonAtrapado,catchProcesado->entrenadorConCatch->pokemonAAtrapar.pokemon);

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
		//Ver leak posible de objetivo.

		loggearPokemonCapturado(catchProcesado->entrenadorConCatch, true);
	}
	else{
		loggearPokemonCapturado(catchProcesado->entrenadorConCatch, false);
	}

	//Marca objetivos cumplidos de entrenador.
	seCumplieronLosObjetivosDelEntrenador(catchProcesado->entrenadorConCatch);

	sem_wait(&mutexEntrenadores);
	catchProcesado->entrenadorConCatch->suspendido = false;
	sem_post(&mutexEntrenadores);


	log_info(logger,"Pokemones: ");
	imprimirListaDeCadenas(catchProcesado->entrenadorConCatch->pokemones);

	log_info(logger,"Objetivos: ");
	imprimirListaDeCadenas(catchProcesado->entrenadorConCatch->objetivos);

	//Verifica si estan en deadlock, SOLO cuando se acabaron los objetivos generales.
	verificarDeadlock();
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

	mensajeCaught* miCaught = malloc(sizeof(mensajeCaught));
	char* resultado = malloc(5); //OK o FAIL
	memcpy(&miCaught->resultado,miMensajeRecibido->mensaje,sizeof(resultado));

	strcpy(resultado,(miCaught->resultado ? "OK" : "FAIL"));

	if(validarIDCorrelativoCatch(miMensajeRecibido->idCorrelativo)) {
		log_debug(logger, "Se recibio un CAUGHT valido");
		procesarObjetivoCumplido(buscarCatch(miMensajeRecibido->idCorrelativo),miCaught->resultado);
	}

	log_info(logger, "Mensaje recibido: CAUGHT_POKEMON %s", resultado);
	log_info(loggerOficial, "Mensaje recibido: CAUGHT_POKEMON %s", resultado);

	free(resultado);
	free(miCaught);
	free(miMensajeRecibido->mensaje);
	free(miMensajeRecibido);

	sem_post(&mutexCAUGHT);
}

void procesarLOCALIZED(mensajeRecibido* miMensajeRecibido) {
	sem_wait(&mutexLOCALIZED);

	uint32_t cantPokes, longPokemon;
	int offset = 0;

	memcpy(&longPokemon, miMensajeRecibido->mensaje, sizeof(uint32_t));
	offset = sizeof(uint32_t);

	char* pokemon = malloc(longPokemon + 1);
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

		//ponerEnReadyAlMasCercano(posicion->pos[0], posicion->pos[1],pokemon);
		sem_post(&procesoEnReady);
	}
	log_info(logger, "Mensaje recibido: LOCALIZED_POKEMON %s con %d pokemones", pokemon, cantPokes);
	log_info(loggerOficial, "Mensaje recibido: LOCALIZED_POKEMON %s con %d pokemones", pokemon, cantPokes);

	free(miMensajeRecibido->mensaje);
	free(miMensajeRecibido);

	sem_post(&mutexLOCALIZED);
}

void procesarAPPEARED(mensajeRecibido* miMensajeRecibido) {
	sem_wait(&mutexAPPEARED);

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

		//ponerEnReadyAlMasCercano(posicion->pos[0], posicion->pos[1],pokemonRecibido);

		log_info(logger, "Mensaje recibido: APPEARED_POKEMON %s %d %d.", pokemonRecibido, posicion->pos[0], posicion->pos[1]);
		log_info(loggerOficial, "Mensaje recibido: APPEARED_POKEMON %s %d %d.", pokemonRecibido, posicion->pos[0], posicion->pos[1]);

		sem_post(&procesoEnReady);
	}
	else{
		log_error(logger,"No me interesa el pokemon %s",pokemonRecibido);
		log_info(loggerOficial, "Se recibio mensaje APPEARED_POKEMON para un %s. Ese pokemon no esta en mis objetivos.",pokemonRecibido);
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
						log_error(logger, "Cola de Mensaje erronea.");
						log_info(loggerOficial, "Se recibio un mensaje invalido.");
						break;
					}
				}
			}
		else {
			brokerConectado = false;
			log_error(logger, "Se perdio la conexión con el Broker.");
			close(*socketServidor);
			close(*socketBrokerApp);
			close(*socketBrokerLoc);
			close(*socketBrokerCau);
			log_debug(logger, "Reintentando conexion...");

			log_info(loggerOficial, "Se perdio la conexion con el Broker");
			while(1){
				*socketServidor = crearConexionCliente(ipServidor, puertoServidor);
				if(*socketServidor == -1){
					log_info(loggerOficial, "No se pudo reestablecer la conexion con el Broker. Reintentando...");
					usleep(tiempoDeEspera * 1000000);
				}
				else{
					log_info(loggerOficial,"Reconexion con el Broker realizada correctamente.");
					break;
				}
			}
			sem_post(&reconexion);
			break;
		}
	}
}

void crearHiloParaAtenderServidor(int *socketServidor) {
	pthread_t hiloAtenderServidor;
	pthread_create(&hiloAtenderServidor, NULL, (void*) atenderServidor,
				socketServidor);
	pthread_detach(hiloAtenderServidor);
}

void reconectar(){
	while(noSeCumplieronLosObjetivos()){
		sem_wait(&reconexion);
		usleep(1 * 1000000);
		crearConexionesYSuscribirseALasColas();

	}
}

void crearHiloDeReconexion(){
	pthread_t hiloDeReconexion;

	pthread_create(&hiloDeReconexion, NULL, (void*) reconectar, NULL);
	pthread_detach(hiloDeReconexion);
}
void crearHilosParaAtenderBroker() {
	crearHiloParaAtenderServidor(socketBrokerApp);
	crearHiloParaAtenderServidor(socketBrokerLoc);
	crearHiloParaAtenderServidor(socketBrokerCau);

	crearHiloDeReconexion();
}

void crearConexion(int* socket){
	*socket = crearConexionClienteConReintento(ipServidor, puertoServidor, tiempoDeEspera);
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

	pthread_join(hiloSocketApp, NULL);
	suscribirseACola(*socketBrokerApp,APPEARED, idDelProceso);
	crearHiloParaAtenderServidor(socketBrokerApp);

	pthread_join(hiloSocketCau, NULL);
	suscribirseACola(*socketBrokerCau,CAUGHT, idDelProceso);
	crearHiloParaAtenderServidor(socketBrokerCau);

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
		int *socketCliente = esperarCliente(*socketEscucha);
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

	log_info(logger, "Enviando GETs...");
	sem_wait(&mutexOBJETIVOS);
	for (int i = 0; i < list_size(team->objetivo); i++) {
		pokemon = list_get(team->objetivo, i);
		enviarGetDePokemon(ip, puerto, pokemon);
	}
	sem_post(&mutexOBJETIVOS);
	log_info(logger, "GETs enviados");
}
