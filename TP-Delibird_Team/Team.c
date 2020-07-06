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
	tiempoDeEspera = atoi(config_get_string_value(config, "TIEMPO_DE_ESPERA"));
	team->algoritmoPlanificacion = obtenerAlgoritmoPlanificador();
	listaCondsEntrenadores = list_create();
	listaPosicionesInternas = list_create();
	idsDeCatch = list_create();
	alfa =(float)atof(config_get_string_value(config, "ALFA"));
	socketBrokerApp = malloc(sizeof(int));
	socketBrokerLoc = malloc(sizeof(int));
	socketBrokerCau = malloc(sizeof(int));
	socketGameboy = malloc(sizeof(int));

	//inicializo el mutex para los mensajes que llegan del broker
	sem_init(&mutexMensajes, 0, 1);
	sem_init(&mutexEntrenadores,0,1);
	sem_init(&mutexAPPEARED, 0, 1);
	sem_init(&mutexLOCALIZED, 0, 1);
	sem_init(&mutexCAUGHT, 0, 1);
	sem_init(&semPlanif, 0, 0);
	sem_init(&procesoEnReady,0,0);
	sem_init(&conexionCreada, 0, 0);
	sem_init(&resolviendoDeadlock,0,1);

	log_debug(logger, "Se ha iniciado un Team.");
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
	char** listaDeConfig;
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

/*void crearConexionesCliente(int* socketBrokerLoc, int* socketBrokerApp, int* socketBrokerCau) {
	pthread_t hiloSocketLoc;
	pthread_t hiloSocketApp;
	pthread_t hiloSocketCau;

	pthread_create(&hiloSocketLoc, NULL, (void*) suscribirseACola, APPEARED);
	pthread_detach(hiloSocketLoc);

	pthread_create(&hiloSocketApp, NULL, (void*) suscribirseACola, socketBrokerApp);
	pthread_detach(hiloSocketApp);

	pthread_create(&hiloSocketCau, NULL, (void*) crearConexion, socketBrokerCau);
	pthread_detach(hiloSocketCau);


//	*socketBrokerLoc = crearConexionClienteConReintento(ipServidor, puertoServidor, tiempoDeEspera);
//	*socketBrokerApp = crearConexionClienteConReintento(ipServidor, puertoServidor, tiempoDeEspera);
//	*socketBrokerCau = crearConexionClienteConReintento(ipServidor, puertoServidor, tiempoDeEspera);
}*/

int main() {

	inicializarVariablesGlobales();

	generarEntrenadores();

	setearObjetivosDeTeam();

	inicializarSemEntrenadores();

	crearHilosDeEntrenadores();


	//Creo conexion con Gameboy
	conectarGameboy();

	//Se suscribe el Team a las colas
	crearConexionesYSuscribirseALasColas();

	crearHiloPlanificador();

	enviarGetSegunObjetivo(ipServidor,puertoServidor);

	//El TEAM finaliza cuando termine de ejecutarse el planificador, que sera cuando se cumplan todos los objetivos.
	pthread_join(hiloPlanificador, NULL);

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

