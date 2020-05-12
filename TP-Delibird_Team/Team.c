#include "Team.h"

void inicializarVariablesGlobales() {
	config = config_create("team.config");
	logger = log_create("team_logs", "Team", 1, LOG_LEVEL_TRACE);
	listaHilos = list_create();
	team = malloc(sizeof(t_team));
	team->entrenadores = list_create();
	team->objetivo = list_create();
	colaDeReady = queue_create(); //esto se necesita para el FIFO.
	colaDeBloqued = queue_create(); //esto es necesario??
	pokemonRecibido = string_new();
	ipServidor = malloc(strlen(config_get_string_value(config, "IP")) + 1);
	ipServidor = config_get_string_value(config, "IP");
	puertoServidor = malloc(strlen(config_get_string_value(config, "PUERTO")) + 1);
	puertoServidor = config_get_string_value(config, "PUERTO");
	team->algoritmoPlanificacion = obtenerAlgoritmoPlanificador();
	listaMensajesRecibidosLocalized = list_create();
	listaCondsEntrenadores = list_create();
	//inicializo el mutex para los hilos de entrenador
	pthread_mutex_init(mutexHilosEntrenadores,1);
	//inicializo el mutex para los mensajes que llegan del broker
	sem_init(mutexMensajes, 1, 0);
}

void array_iterate_element(char** strings, void (*closure)(char*, t_list*),
		t_list *lista) {
	while (*strings != NULL) {
		closure(*strings, lista);
		strings++;
	}
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
	recv(*socketBroker,&(idRespuesta),sizeof(uint32_t),MSG_WAITALL);
	log_debug(logger,"Mensaje enviado :smilieface:");
	log_info(logger, "Respuesta del Broker: %d", idRespuesta);
	free(msg);
	close(*socketBroker);
	free(socketBroker);
}

void enlistar(char *elemento, t_list *lista) {
	if (elemento != NULL)
		list_add(lista, elemento);
}

//implementacion generica para obtener de configs
void obtenerDeConfig(char *clave, t_list *lista) {
	char** listaDeConfig = malloc(
			sizeof(config_get_array_value(config, clave)));
	listaDeConfig = config_get_array_value(config, clave);

	array_iterate_element(listaDeConfig, enlistar, lista);
}

/* Atender al Broker */
void atenderBroker(int *socketBroker) {
	mensajeRecibido *miMensajeRecibido = malloc(sizeof(mensajeRecibido));

	log_info(logger, "Llega hasta antes de recibirMensajeDeBroker");
	while (1) {
		miMensajeRecibido = recibirMensajeDeBroker(*socketBroker);

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
				t_pokemonesLocalized *p = malloc(sizeof(t_pokemonesLocalized));
				int cantPokes,longPokemon;
				int offset = 0;

				memcpy(&longPokemon,miMensajeRecibido->mensaje,sizeof(uint32_t));
				offset = sizeof(uint32_t);

				char *pokemon = malloc(longPokemon);
				memcpy(pokemon,miMensajeRecibido->mensaje + offset,longPokemon);

				offset += longPokemon;
				memcpy(&cantPokes,miMensajeRecibido->mensaje + offset,sizeof(uint32_t));

				offset += sizeof(uint32_t);
				int x[cantPokes],y[cantPokes],cant[cantPokes];

				for(int i = 0;i < cantPokes;i++){
					memcpy(&x[i],miMensajeRecibido->mensaje + offset,sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(&y[i],miMensajeRecibido->mensaje + offset,sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(&cant[i],miMensajeRecibido->mensaje + offset,sizeof(uint32_t));
					offset += sizeof(uint32_t);

					//prototipo para agregar a una estructura que se envie al planificador.
					list_add(p->x,&x[i]);
					list_add(p->y,&y[i]);
					list_add(p->cantidades,&cant[i]);
				}
				//agrego tambien a la estructura el nombre del pokemon y la cant
				p->pokemon = malloc(longPokemon);
				strcpy(p->pokemon,pokemon);
				p->cantPokes = cantPokes;


				sem_wait(mutexMensajes);
				list_add(listaMensajesRecibidosLocalized,p);
				sem_post(mutexMensajes);
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
}

void crearHiloParaAtenderBroker(int *socketBroker) {
	pthread_t hiloAtenderBroker;
	pthread_create(&hiloAtenderBroker, NULL, (void*) atenderBroker,
				socketBroker);
	pthread_detach(hiloAtenderBroker);
}

/* Se suscribe a las colas del Broker */
void suscribirseALasColas(int socketA,int socketL,int socketC) {

	suscribirseACola(socketA,APPEARED);

	suscribirseACola(socketL,LOCALIZED);

	suscribirseACola(socketC,CAUGHT);

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

/* Liberar memoria al finalizar el programa */
void liberarMemoria() {
	for (int i = 0; i < list_size(team->entrenadores); i++) {
		list_destroy(
				((t_entrenador*) (list_get(team->entrenadores, i)))->objetivos);
		list_destroy(
				((t_entrenador*) (list_get(team->entrenadores, i)))->pokemones);
	}
	list_destroy(team->objetivo);
	list_destroy(team->entrenadores);
	list_destroy(listaHilos);
	queue_destroy(colaDeReady);
	queue_destroy(colaDeBloqued);
	log_destroy(logger);
	config_destroy(config);
}

bool elementoEstaEnLista(t_list *lista, char *elemento) {
	bool verifica = false;
	for (int i = 0; i < list_size(lista); i++) {
		if (strcmp(list_get(lista, i), elemento))
			verifica = true;
	}
	return verifica;
}

bool esUnObjetivo(void* objetivo) {
	bool verifica = false;
	if (string_equals_ignore_case(pokemonRecibido, objetivo)) {
		verifica = true;
	}
	return verifica;
}

/* Planificar */
//void planificar() {
//	/*
//	 Nuevo Hilo por cada entrenador
//
//	 APARECE UN POKEMON
//
//	 Me fijo algun entrendaro libre (NUEVO/BLOQUEADO)
//	 idEntrenadorLibreMasCercano= masCercano(team->entrenadores,pokemon);
//	 (team->entrenadores[idEntrenadorLibreMasCercano])->estado = READY
//
//	 meterProcesosListosEnLaColaDeListos();
//	 FIFO con la cola --> cambio estado del Entrenador a EJEC
//
//	 Despierta el Hilo del entrenador con estado EJEC de listaHilos = [threadEntrenador1,threadEntrenador2,...]
//	 Cuando termina el Hilo lo pasa a BLOQUEADO o FIN segun corresponda
//	 */
//
//	//Arma un hilo por entrenador
//
//}

int main() {
	uint32_t idDelProceso;

	inicializarVariablesGlobales();

	//Obtengo el ID del proceso
	idDelProceso = obtenerIdDelProceso(ipServidor, puertoServidor);

	//Creo 3 conexiones con el Broker, una por cada cola
	int *socketBrokerApp = malloc(sizeof(int));
	*socketBrokerApp = crearConexionCliente(ipServidor, puertoServidor);
	int *socketBrokerLoc = malloc(sizeof(int));
	*socketBrokerLoc = crearConexionCliente(ipServidor, puertoServidor);
	int *socketBrokerCau = malloc(sizeof(int));
	*socketBrokerCau = crearConexionCliente(ipServidor, puertoServidor);

	//Se suscribe el Team a las colas
	suscribirseALasColas(*socketBrokerApp,*socketBrokerLoc,*socketBrokerCau, idDelProceso);
	crearHiloParaAtenderBroker(socketBrokerApp);
	crearHiloParaAtenderBroker(socketBrokerLoc);
	crearHiloParaAtenderBroker(socketBrokerCau);

	generarEntrenadores();

	setearObjetivosDeTeam(team);

	for(int j = 0; j < list_size(team->entrenadores);j++){
		pthread_cond_t *cond = malloc(sizeof(pthread_cond_t));

		pthread_cond_init(cond,1);

		list_add(listaCondsEntrenadores,cond);
	}

	enviarGetSegunObjetivo(ipServidor,puertoServidor);

	for (int i = 0; i < list_size(team->entrenadores); i++) {
		crearHiloEntrenador(list_get(team->entrenadores, i));
	}//Que cada hilo se bloquee a penas empieza.

	/*Ahora me quedo esperando a que me llegue un LOCALIZED o en su defecto si no
	 * hay conexion con el broker, un APPEARED.
	 */

	log_info(logger, "Finalizó la conexión con el servidor\n");
	log_info(logger, "El proceso team finalizó su ejecución\n");

	free(ipServidor);
	free(puertoServidor);
	close(*socketBrokerApp);
	close(*socketBrokerLoc);
	close(*socketBrokerCau);
	free(socketBrokerApp);
	free(socketBrokerLoc);
	free(socketBrokerCau);
//	liberarMemoria();
	return 0;
}

