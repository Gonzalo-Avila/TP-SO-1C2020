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

/*MANEJA EL FUNCIONAMIENTO INTERNO DE CADA ENTRENADOR(trabajo en un hilo separado)*/
void gestionarEntrenador(t_entrenador *entrenador) {
	//mover entrenador a posicion del pokemon que necesita
	//pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
	//pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

}

void crearHiloEntrenador(t_entrenador* entrenador) {
	pthread_t nuevoHilo;
	t_listaHilos* nodoListaDeHilos = malloc(sizeof(t_listaHilos));

	pthread_create(&nuevoHilo, NULL, (void*) gestionarEntrenador, entrenador);

	nodoListaDeHilos->hilo = nuevoHilo;
	nodoListaDeHilos->idEntrenador = entrenador->id;

	list_add(listaHilos, nodoListaDeHilos);

	pthread_detach(nuevoHilo);
}

/*Arma el Entrenador*/
t_entrenador* armarEntrenador(int id, char *posicionesEntrenador,
		char *objetivosEntrenador, char *pokemonesEntrenador) {
	t_entrenador* nuevoEntrenador = malloc(sizeof(t_entrenador));
	t_list *posicionEntrenador = list_create();
	t_list *objetivoEntrenador = list_create();
	t_list *pokemonEntrenador = list_create();

	array_iterate_element((char **) string_split(posicionesEntrenador, "|"),
			(void *) enlistar, posicionEntrenador);
	array_iterate_element((char **) string_split(objetivosEntrenador, "|"),
			(void *) enlistar, objetivoEntrenador);
	array_iterate_element((char **) string_split(pokemonesEntrenador, "|"),
			(void *) enlistar, pokemonEntrenador);

	for (int i = 0; i < 2; i++) {
		nuevoEntrenador->pos[i] = atoi(list_get(posicionEntrenador, i));
	}
	nuevoEntrenador->id = id;
	nuevoEntrenador->objetivos = objetivoEntrenador;
	nuevoEntrenador->pokemones = pokemonEntrenador;
	nuevoEntrenador->estado = NUEVO; // Debugeando de mi cuenta que sin esta linea de codigo solo el ultimo elemento lo pasa a new

	list_destroy(posicionEntrenador);

	return nuevoEntrenador;
}

/* Genera los Entrenadores con los datos del Config */
void generarEntrenadores() {
	t_entrenador* unEntrenador = malloc(sizeof(t_entrenador));
	t_list* posiciones = list_create();
	t_list* objetivos = list_create();
	t_list* pokemones = list_create();

	obtenerDeConfig("POSICIONES_ENTRENADORES", posiciones);
	obtenerDeConfig("OBJETIVO_ENTRENADORES", objetivos);
	obtenerDeConfig("POKEMON_ENTRENADORES", pokemones);
	for (int contador = 0; contador < list_size(posiciones); contador++) {
		unEntrenador = armarEntrenador(contador, list_get(posiciones, contador),
				list_get(objetivos, contador), list_get(pokemones, contador));
		list_add(team->entrenadores, unEntrenador);
	}
	list_destroy(posiciones);
	list_destroy(objetivos);
	list_destroy(pokemones);
}

e_algoritmo obtenerAlgoritmoPlanificador() {
	char* algoritmo = malloc(
			strlen(config_get_string_value(config, "ALGORITMO_PLANIFICACION"))
					+ 1);
	algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	if (strcmp(algoritmo, "FIFO") == 0) {
		return FIFO;
	} else if (strcmp(algoritmo, "RR") == 0) {
		return RR;
	} else if (strcmp(algoritmo, "SJFCD") == 0) {
		return SJFCD;
	} else if (strcmp(algoritmo, "SJFSD") == 0) {
		return SJFSD;
	} else {
		log_info(logger,
				"No se ingresó un algoritmo válido en team.config. Se toma FIFO por defecto.\n");
		return FIFO;
	}
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

				//planificador(p);
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

void setearObjetivosDeTeam(t_team *team) {
	t_entrenador *entrenador = malloc(sizeof(t_entrenador));

	for (int i = 0; i < list_size(team->entrenadores); i++) {
		entrenador = list_get(team->entrenadores, i);
		for (int j = 0; j < list_size(entrenador->objetivos); j++) {
			list_add(team->objetivo, list_get(entrenador->objetivos, j));
		}
	}
}

void enviarGetSegunObjetivo(char *ip, char *puerto) {
	char *pokemon = malloc(MAXSIZE);

	for (int i = 0; i < list_size(team->objetivo); i++) {
		pokemon = list_get(team->objetivo, 0);
		enviarGetDePokemon(ip, puerto, pokemon);
	}
	free(pokemon);

}

float calcularDistancia(int posX1, int posY1, int posX2, int posY2) {
	int cat1, cat2;
	float distancia;

	cat1 = abs(posX2 - posX1);
	cat2 = abs(posY2 - posY1);
	distancia = sqrt(pow(cat1, 2) + pow(cat2, 2));

	return distancia;
}

//setea la distancia de todos los entrenadores del team al pokemon localizado
t_dist *setearDistanciaEntrenadores(int id, int posX, int posY) {
	t_entrenador *entrenador = malloc(sizeof(t_entrenador));
	t_dist *distancia = malloc(sizeof(t_dist));

	entrenador = list_get(team->entrenadores, id);

	distancia->dist = calcularDistancia(entrenador->pos[0], entrenador->pos[1],
			posX, posY);
	distancia->idEntrenador = id;

	return distancia;
}

bool menorDist(void *dist1, void *dist2) {
	bool verifica = false;

	if (((t_dist*) dist1)->dist < ((t_dist*) dist2)->dist)
		verifica = true;

	return verifica;
}

t_entrenador *entrenadorMasCercano(int posX, int posY) {
	t_list* listaDistancias = list_create();
	t_dist *distancia = malloc(sizeof(t_dist));

	for (int i = 0; i < list_size(team->entrenadores); i++) {
		distancia = setearDistanciaEntrenadores(i, posX, posY);
		list_add(listaDistancias, distancia);
	}
	list_sort(listaDistancias, menorDist);

	int idEntrenadorConDistMenor =
			((t_dist*) list_get(listaDistancias, 0))->idEntrenador;

	return ((t_entrenador*) list_get(team->entrenadores,
			idEntrenadorConDistMenor));
}

//Esta funcion se podria codear para que sea una funcion generica, pero por el momento solo me sirve saber si está o no en ready.
bool estaEnEspera(void *entrenador) {
	bool verifica = false;
	t_entrenador *trainer = malloc(sizeof(t_entrenador));
	trainer = entrenador;
	if (((trainer->estado) == NUEVO) || ((trainer->estado) == BLOQUEADO))
		verifica = true;

	return verifica;
}

bool esUnObjetivo(void* objetivo) {
	bool verifica = false;
	if (string_equals_ignore_case(pokemonRecibido, objetivo)) {
		verifica = true;
	}
	return verifica;
}

void planificarFifo(){
	t_list *entrenadoresReady = list_create();

	for(int i = 0;i < list_size(team->entrenadores);i++){
		t_entrenador *entrenador = malloc(sizeof(t_entrenador));

		entrenador = list_get(team->entrenadores,i);

		if(entrenador->estado == LISTO)
			list_add(entrenadoresReady,entrenador);
	}
	//como esto es por FIFO tengo que poner en EXEC a solo uno y elegir por FIFO
	//------cuando sale ese de EXEC entra el que sigue

	for(int j = 0; j < list_size(entrenadoresReady);j++){
		t_entrenador *entrenador = malloc(sizeof(t_entrenador));

		entrenador = list_get(entrenadoresReady,j);

		activarHiloDe(entrenador->id);
	}
}

void planificador(t_pokemonesLocalized *pLocalized){
	//En caso que tengamos varias posiciones y varios entrenadores libres, con que criterio elegimos???
	t_entrenador* elEntrenadorMasCercano = malloc(sizeof(t_entrenador));

	elEntrenadorMasCercano = entrenadorMasCercano((int)list_get(pLocalized->x,0),(int)list_get(pLocalized->y,0));
	elEntrenadorMasCercano->estado = LISTO;//me aseguro de que tengo uno en READY antes de llamar para planificar

	switch(team->algoritmoPlanificacion){
			case FIFO:
				planificarFifo();
				break;
			//en caso que tengamos otro algoritmo usamos la funcion de ese algoritmo
			default:
				planificarFifo();
				break;
	}
}

/* Planificar */
void planificar() {
	/*
	 Nuevo Hilo por cada entrenador

	 APARECE UN POKEMON

	 Me fijo algun entrendaro libre (NUEVO/BLOQUEADO)
	 idEntrenadorLibreMasCercano= masCercano(team->entrenadores,pokemon);
	 (team->entrenadores[idEntrenadorLibreMasCercano])->estado = READY

	 meterProcesosListosEnLaColaDeListos();
	 FIFO con la cola --> cambio estado del Entrenador a EJEC

	 Despierta el Hilo del entrenador con estado EJEC de listaHilos = [threadEntrenador1,threadEntrenador2,...]
	 Cuando termina el Hilo lo pasa a BLOQUEADO o FIN segun corresponda
	 */

	for (int i = 0; i < list_size(team->entrenadores); i++) {
		crearHiloEntrenador(list_get(team->entrenadores, i));
	}//Arma un hilo por entrenador

}

int main() {
	inicializarVariablesGlobales();


	int *socketBrokerApp = malloc(sizeof(int));
	*socketBrokerApp = crearConexionCliente(ipServidor, puertoServidor);
	int *socketBrokerLoc = malloc(sizeof(int));
	*socketBrokerLoc = crearConexionCliente(ipServidor, puertoServidor);
	int *socketBrokerCau = malloc(sizeof(int));
	*socketBrokerCau = crearConexionCliente(ipServidor, puertoServidor);

	//Se suscribe el Team a las colas
	suscribirseALasColas(*socketBrokerApp,*socketBrokerLoc,*socketBrokerCau);
	crearHiloParaAtenderBroker(socketBrokerApp);
	crearHiloParaAtenderBroker(socketBrokerLoc);
	crearHiloParaAtenderBroker(socketBrokerCau);

	generarEntrenadores();


	setearObjetivosDeTeam(team);

	enviarGetSegunObjetivo(ipServidor,puertoServidor);

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

