#include "Team.h"

void inicializarVariablesGlobales() {
	config = config_create("team.config");
	logger = log_create("team_logs", "Team", 1, LOG_LEVEL_TRACE);
	loggerOficial = log_create(config_get_string_value(config, "LOG_FILE"),"Delibird - Team", 0, LOG_LEVEL_TRACE);
	listaHilos = list_create();
	team = malloc(sizeof(t_team));
	team->entrenadores = list_create();
	team->objetivo = list_create();
	listaDeReady = list_create(); //esto se necesita para el FIFO.
	listaDeBloqued = list_create(); //esto es necesario??
//	ipServidor = malloc(strlen(config_get_string_value(config, "IP")) + 1);
	ipServidor = config_get_string_value(config, "IP");
//	puertoServidor = malloc(strlen(config_get_string_value(config, "PUERTO")) + 1);
	puertoServidor = config_get_string_value(config, "PUERTO");
	tiempoDeEspera = atoi(config_get_string_value(config, "CONNECTION_RETRY"));
	team->algoritmoPlanificacion = obtenerAlgoritmoPlanificador();
	listaCondsEntrenadores = list_create();
	listaPosicionesInternas = list_create();
	idsDeCatch = list_create();
	alfa =(float)atof(config_get_string_value(config, "ALFA"));
	socketBrokerApp = malloc(sizeof(int));
	socketBrokerLoc = malloc(sizeof(int));
	socketBrokerCau = malloc(sizeof(int));
	socketGameboy = malloc(sizeof(int));
	brokerConectado = false;

	ciclosDeCPUTotales = 0;
	cambiosDeContexto = 0;
	deadlocksResueltos = 0;

	hayEntrenadorDesalojante = false;

	//inicializo el mutex para los mensajes que llegan del broker
	sem_init(&mutexMensajes, 0, 1);
	sem_init(&mutexEntrenadores,0,1);
	sem_init(&mutexAPPEARED, 0, 1);
	sem_init(&mutexLOCALIZED, 0, 1);
	sem_init(&mutexCAUGHT, 0, 1);
	sem_init(&mutexOBJETIVOS, 0, 1);
	sem_init(&semPlanif, 0, 0);
	sem_init(&procesoEnReady,0,0);
	sem_init(&conexionCreada, 0, 0);
	sem_init(&reconexion, 0, 0);
	sem_init(&resolviendoDeadlock,0,0);

	log_debug(logger, "Se ha iniciado un Team.");
}

void array_iterate_element(char** strings, void (*closure)(char*, t_list*),t_list *lista) {

	while (*strings != NULL) {
		closure(*strings, lista);
		strings++;
	}
}

void enlistar(char *elemento, t_list *lista) {
	if (elemento != NULL)
		list_add(lista, elemento);
}

//implementacion generica para obtener de configs
void obtenerDeConfig(char *clave, t_list *lista) {
	char** listaDeConfig;
	listaDeConfig = config_get_array_value(config, clave);

	array_iterate_element(listaDeConfig, enlistar, lista);
	free(listaDeConfig);
}

void destruirEntrenadores() {
	for (int i = 0; i < list_size(team->entrenadores); i++) {
		t_entrenador* entrenadorABorrar = (t_entrenador*) (list_get(team->entrenadores, i));

		sem_destroy(&semEntrenadores[i]);
		sem_destroy(&semEntrenadoresRR[i]);
		list_destroy_and_destroy_elements(entrenadorABorrar->objetivosOriginales,destructorGeneral);
		list_destroy_and_destroy_elements(entrenadorABorrar->pokemones,	destructorGeneral);
		free(entrenadorABorrar->pokemonAAtrapar.pokemon);

		if(entrenadorABorrar->datosDeadlock.pokemonAIntercambiar != NULL)
			free(entrenadorABorrar->datosDeadlock.pokemonAIntercambiar);
	}

	list_destroy_and_destroy_elements(team->entrenadores, destructorGeneral);
}

/* Liberar memoria al finalizar el programa */
void liberarMemoria() {
	//destruirEntrenadores();
	//asumo que el list_destroy hace los free correspondientes.
	list_destroy(team->objetivo);
	list_destroy_and_destroy_elements(listaHilos, destructorGeneral);
	list_destroy_and_destroy_elements(listaDeReady, destructorGeneral);
	list_destroy_and_destroy_elements(listaDeBloqued, destructorGeneral);
	list_destroy_and_destroy_elements(idsDeCatch, destructorGeneral);
	list_destroy_and_destroy_elements(listaPosicionesInternas, destructorGeneral);
	list_destroy_and_destroy_elements(listaCondsEntrenadores, destructorGeneral);

	config_destroy(config);
	log_destroy(loggerOficial);
	log_info(logger, "Memoria liberada.");
	log_destroy(logger);
}
void destruirSemaforos(){
	sem_destroy(&mutexMensajes);
	sem_destroy(&mutexEntrenadores);
	sem_destroy(&mutexAPPEARED);
	sem_destroy(&mutexLOCALIZED);
	sem_destroy(&mutexCAUGHT);
	sem_destroy(&mutexOBJETIVOS);
	sem_destroy(&semPlanif);
	sem_destroy(&procesoEnReady);
	sem_destroy(&conexionCreada);
	sem_destroy(&reconexion);
	sem_destroy(&resolviendoDeadlock);
}
void liberarConexiones(){
	close(*socketBrokerApp);
	close(*socketBrokerLoc);
	close(*socketBrokerCau);
	close(*socketGameboy);
	free(socketBrokerApp);
	free(socketBrokerLoc);
	free(socketBrokerCau);
	free(socketGameboy);
}
void imprimirResultadosDelTeam(){
	log_info(loggerOficial,"El proceso Team ha finalizado.");
	log_info(loggerOficial,"Ciclos de CPU totales = %d", ciclosDeCPUTotales);
	log_info(loggerOficial,"Cambios de contexto realizados = %d", cambiosDeContexto);
	//TODO - loggear cambios de contexto por entrenador.
	log_info(loggerOficial, "Deadlocks detectados y resueltos = %d", deadlocksResueltos);

	log_info(logger,"El proceso Team ha finalizado.");
	log_info(logger,"Ciclos de CPU totales = %d", ciclosDeCPUTotales);
	log_info(logger,"Cambios de contexto realizados = %d", cambiosDeContexto);
	//TODO - loggear cambios de contexto por entrenador.
	log_info(logger, "Deadlocks detectados y resueltos = %d", deadlocksResueltos);

}

bool elementoEstaEnLista(t_list *lista, char *elemento) {
	bool verifica = false;
	for (int i = 0; i < list_size(lista); i++) {
		if (strcmp(list_get(lista, i), elemento))
			verifica = true;
	}
	return verifica;
}

void inicializarSemEntrenadores() {
	semEntrenadores = malloc(list_size(team->entrenadores) * sizeof(sem_t));
	semEntrenadoresRR = malloc(list_size(team->entrenadores) * sizeof(sem_t));

	for (int j = 0; j < list_size(team->entrenadores); j++) {
		sem_init(&(semEntrenadores[j]), 0, 0);
		sem_init(&(semEntrenadoresRR[j]), 0, 0);
	}
}

void crearHilosDeEntrenadores() {
	for (int i = 0; i < list_size(team->entrenadores); i++) {
		crearHiloEntrenador(list_get(team->entrenadores, i));
     	//Que cada hilo se bloquee a penas empieza.
	}
}

void finalizarProceso(){
	imprimirResultadosDelTeam();
	liberarConexiones();
	destruirSemaforos();
	//liberarMemoria();
	exit(0);
}

int main() {
	signal(SIGINT,finalizarProceso);

	inicializarVariablesGlobales();

	generarEntrenadores();

	setearObjetivosDeTeam();

	inicializarSemEntrenadores();

	crearHilosDeEntrenadores();

	crearHiloPlanificador();

	if(noSeCumplieronLosObjetivos()){

		//Creo conexion con Gameboy
		conectarGameboy();

		//Se suscribe el Team a las colas
		crearConexionesYSuscribirseALasColas();

		enviarGetSegunObjetivo(ipServidor,puertoServidor);
	}

	//El TEAM finaliza cuando termine de ejecutarse el planificador, que sera cuando se cumplan todos los objetivos.
	pthread_join(hiloPlanificador, NULL);

	log_info(logger, "Finalizó la conexión con el servidor\n");

	finalizarProceso();
	return 0;
}

