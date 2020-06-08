#include "Team.h"

void inicializarVariablesGlobales() {
	config = config_create("team.config");
	logger = log_create("team_logs", "Team", 1, LOG_LEVEL_TRACE);
	listaHilos = list_create();
	team = malloc(sizeof(t_team));
	team->entrenadores = list_create();
	team->objetivo = list_create();
	listaDeReady = list_create(); //esto se necesita para el FIFO.
	listaDeBloqued = list_create(); //esto es necesario??
	//pokemonRecibido = string_new();
	ipServidor = malloc(strlen(config_get_string_value(config, "IP")) + 1);
	ipServidor = config_get_string_value(config, "IP");
	puertoServidor = malloc(strlen(config_get_string_value(config, "PUERTO")) + 1);
	puertoServidor = config_get_string_value(config, "PUERTO");
	team->algoritmoPlanificacion = obtenerAlgoritmoPlanificador();
	listaCondsEntrenadores = list_create();
	listaPosicionesInternas = list_create();
	idsDeCatch = list_create();

	//inicializo el mutex para los mensajes que llegan del broker
	sem_init(&mutexMensajes, 0, 1);
	sem_init(&mutexEntrenadores,0,1);
	sem_init(&semPlanif, 0, 0);
	sem_init(&procesoEnReady,0,0);
}

void array_iterate_element(char** strings, void (*closure)(char*, t_list*),
		t_list *lista) {
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
	char** listaDeConfig = malloc(
			sizeof(config_get_array_value(config, clave)));
	listaDeConfig = config_get_array_value(config, clave);

	array_iterate_element(listaDeConfig, enlistar, lista);
}

/* Liberar memoria al finalizar el programa */
void liberarMemoria() {
	for (int i = 0; i < list_size(team->entrenadores); i++) {
		list_destroy(
				((t_entrenador*) (list_get(team->entrenadores, i)))->objetivos);
		list_destroy(
				((t_entrenador*) (list_get(team->entrenadores, i)))->pokemones);
	}
	//asumo que el list_destroy hace los free correspondientes.
	list_destroy(team->objetivo);
	list_destroy(team->entrenadores);
	list_destroy(listaHilos);
	list_destroy(listaDeReady);
	list_destroy(listaDeBloqued);
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

void inicializarSemEntrenadores() {
	semEntrenadores = malloc(list_size(team->entrenadores) * sizeof(sem_t));
	semEntrenadoresRR = malloc(list_size(team->entrenadores) * sizeof(sem_t));
	for (int j = 0; j < list_size(team->entrenadores); j++) {
		sem_init(&(semEntrenadores[j]), 0, 0);
		sem_init(&(semEntrenadoresRR[j]), 0, 0);
		log_info(logger, "Iniciado semáforos para entrenador %d",
				semEntrenadores[j]);
	}
}

void crearHilosDeEntrenadores() {
	for (int i = 0; i < list_size(team->entrenadores); i++) {
		crearHiloEntrenador(list_get(team->entrenadores, i));
     	//Que cada hilo se bloquee a penas empieza.
	}
}

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

	//Creo conexión con Gameboy
	int *socketGameboy = malloc(sizeof(int));
	*socketGameboy = crearConexionEscuchaGameboy();

	//Crea hilo para atender al Gameboy
	//atenderGameboy(socketGameboy);

	//Se suscribe el Team a las colas
	suscribirseALasColas(*socketBrokerApp,*socketBrokerLoc,*socketBrokerCau, idDelProceso);

	crearHilosParaAtenderBroker(socketBrokerApp, socketBrokerLoc, socketBrokerCau);

	generarEntrenadores();

	setearObjetivosDeTeam();

	inicializarSemEntrenadores();

	enviarGetSegunObjetivo(ipServidor,puertoServidor);
	crearHilosDeEntrenadores();

	planificador();

	log_info(logger, "Finalizó la conexión con el servidor\n");
	log_info(logger, "El proceso Team finalizó su ejecución\n");

	free(ipServidor);
	free(puertoServidor);
	close(*socketBrokerApp);
	close(*socketBrokerLoc);
	close(*socketBrokerCau);
	free(socketBrokerApp);
	free(socketBrokerLoc);
	free(socketBrokerCau);
//	liberarMemoria();
	//Todo revisar porque pincha en LiberarMemoria().
	return 0;
}

