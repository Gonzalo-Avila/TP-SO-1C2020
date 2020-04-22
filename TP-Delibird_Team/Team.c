#include "Team.h"

void inicializarVariablesGlobales(){

	config = config_create("team.config");
	logger = log_create("team_logs","Team",1,LOG_LEVEL_TRACE);
	listaHilos = list_create();
	team = malloc(sizeof(t_team));
	team->entrenadores = list_create();
	team->objetivo = list_create();
	colaDeReady = queue_create();//esto se necesita para el FIFO.
	colaDeBloqued = queue_create();//esto es necesario??

}

void array_iterate_element(char** strings, void (*closure)(char*,t_list*),t_list *lista){
	while (*strings != NULL) {
		closure(*strings,lista);
		strings++;
	}
}

void enlistar(char *elemento,t_list *lista){
	if(elemento != NULL)
		list_add(lista,elemento);
}

//implementacion generica para obtener de configs
void obtenerDeConfig(char *clave,t_list *lista){
	char** listaDeConfig = malloc(sizeof(config_get_array_value(config,clave)));
	listaDeConfig = config_get_array_value(config,clave);

	array_iterate_element(listaDeConfig,enlistar,lista);
}


//void setearObjetivoDeTeam(){
//	for(int i =0;list_size();i++){
//
//	}
//}
/*MANEJA EL FUNCIONAMIENTO INTERNO DE CADA ENTRENADOR(trabajo en un hilo separado)*/
//void gestionarEntrenador(t_entrenador *entrenador,char *pokemon,t_posicion posPokemon){
//
//	//mueve el entrenador una posicion y ejecuta SLEEP(RETARDO_CICLO_CPU)
//	while(moverEntrenadorPorUnidad(entrenador,posPokemon)){
//
//	}
//	//Tira un catch de pokemon
//}

//void crearEntrenador(t_entrenador* entrenador,char *pokemon,t_posicion posPokemon){
//	pthread_t nuevoHilo;
//
//	pthread_create(&nuevoHilo, NULL, (void*)gestionarEntrenador,entrenador);
//	list_add(listaHilos,nuevoHilo);
//
//	pthread_detach(nuevoHilo);
//}


/*arma el entrenador*/
t_entrenador* armarEntrenador(char *posicionesEntrenador,char *objetivosEntrenador,char *pokemonesEntrenador){
	t_entrenador* nuevoEntrenador = malloc(sizeof(t_entrenador));
	t_list *posicionEntrenador = list_create();
	t_list *objetivoEntrenador = list_create();
	t_list *pokemonEntrenador = list_create();

	array_iterate_element((char **)string_split(posicionesEntrenador, "|"),(void *)enlistar,posicionEntrenador);
	array_iterate_element((char **)string_split(objetivosEntrenador, "|"),(void *)enlistar,objetivoEntrenador);
	array_iterate_element((char **)string_split(pokemonesEntrenador, "|"),(void *)enlistar,pokemonEntrenador);

	for(int i = 0; i < 2;i++){
		nuevoEntrenador->pos[i] = atoi(list_get(posicionEntrenador,i));
	}
	nuevoEntrenador->objetivos = objetivoEntrenador;
	nuevoEntrenador->pokemones = pokemonEntrenador;

	list_destroy(posicionEntrenador);
	list_destroy(objetivoEntrenador);
	list_destroy(pokemonEntrenador);

	return nuevoEntrenador;
}

void generarEntrenadores(){
	t_entrenador* unEntrenador = malloc(sizeof(t_entrenador));
	t_list* posiciones = list_create();
	t_list* objetivos  = list_create();
	t_list* pokemones = list_create();

	obtenerDeConfig("POSICIONES_ENTRENADORES",posiciones);
	obtenerDeConfig("OBJETIVO_ENTRENADORES",objetivos);
	obtenerDeConfig("POKEMON_ENTRENADORES",pokemones);
	for(int contador = 0; contador < list_size(posiciones); contador++) {
		unEntrenador = armarEntrenador(list_get(posiciones, contador), list_get(objetivos, contador), list_get(pokemones, contador));
		list_add(team->entrenadores,unEntrenador);
	}
	list_destroy(posiciones);
	list_destroy(objetivos);
	list_destroy(pokemones);
	free(unEntrenador);
}
void planificador(){
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
}

e_algoritmo obtenerAlgoritmoPlanificador(){
	char* algoritmo = malloc(strlen(config_get_string_value(config, "ALGORITMO_PLANIFICACION"))+1);
	algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	if(strcmp(algoritmo, "FIFO")){
		return FIFO;
	}
	else if(strcmp(algoritmo, "RR")){
		return RR;
	}
	else if(strcmp(algoritmo, "SJFCD")){
		return SJFCD;
	}
	else if(strcmp(algoritmo, "SJFSD")){
		return SJFSD;
	}
	else{
		log_info(logger, "No se ingresó un algoritmo válido en team.config. Se toma FIFO por defecto.\n");
		return FIFO;
	}
}

void liberarMemoria(){
	for(int i = 0;i < list_size(team->entrenadores);i++){
		list_destroy(((t_entrenador*)(list_get(team->entrenadores,i)))->objetivos);
		list_destroy(((t_entrenador*)(list_get(team->entrenadores,i)))->pokemones);
	}
	list_destroy(team->objetivo);
	list_destroy(team->entrenadores);
	list_destroy(listaHilos);
    log_destroy(logger);
    config_destroy(config);
}

bool elementoEstaEnLista(t_list *lista, char *elemento){
	bool verifica = false;
	for(int i = 0;i < list_size(lista);i++){
		if(strcmp(list_get(lista,i),elemento))
			verifica = true;
	}
	return verifica;
}

void setearObjetivosDeTeam(t_team *team){
	t_entrenador *entrenador = malloc(sizeof(t_entrenador));
	entrenador->objetivos = list_create();
	char *objetivo = string_new();

	for(int i = 0;i < list_size(team->entrenadores);i++){
		entrenador = list_get(team->entrenadores,i);
		for(int j = 0; j < list_size(entrenador->objetivos); j++){
			//strcpy(objetivo,(char *)list_get(entrenador->objetivos,j));
			//memcpy(objetivo,list_get(entrenador->objetivos,j),sizeof(list_get(entrenador->objetivos,j)));
			objetivo = list_get(entrenador->objetivos,i);
			list_add(team->objetivo,objetivo);
		}
	}
	list_destroy(entrenador->objetivos);
	free(objetivo);
	free(entrenador);
}

int main(){

	inicializarVariablesGlobales();
	generarEntrenadores();

//	//Se setean todos los datos
//	inicializarVariablesGlobales();
//
//	char * ipServidor = malloc(strlen(config_get_string_value(config,"IP"))+1);
//	ipServidor = config_get_string_value(config,"IP");
//
//	char * puertoServidor = malloc(strlen(config_get_string_value(config,"PUERTO"))+1);
//	puertoServidor = config_get_string_value(config,"PUERTO");
//
//	e_algoritmo algoritmoPlanificador = obtenerAlgoritmoPlanificador();
//
//	log_info(logger,"Se ha iniciado el cliente team\n");
//
//    //Se crea la conexion con el broker. Esto posteriormente debe ir con un sistema de reintentos por si el broker esta off
//	int socketBroker = crearConexionCliente(ipServidor,puertoServidor);
//	log_info(logger,"Se ha establecido conexión con el servidor\nIP: %s\nPuerto: %s\nNúmero de socket: %d",
//			 config_get_string_value(config,"IP"),config_get_string_value(config,"PUERTO"));
//
//	free(ipServidor);
//	free(puertoServidor);

	t_entrenador *entrenador = malloc(sizeof(t_entrenador));
	entrenador = list_get(team->entrenadores,0);

	printf("Posicion del entrenador 1: [%d,%d]\n",(entrenador->pos)[0],(entrenador->pos)[1]);
	for(int i = 0;i < list_size(entrenador->objetivos);i++){
		printf("Su objetivo es: %s\n",(char *)list_get(entrenador->objetivos,i));
	};

//	setearObjetivosDeTeam(team);
//	printf("%s",(char*)list_get(team->objetivo,0));
//	//Se suscribe el Team a las colas
//	suscribirseACola(APPEARED, socketBroker);
//	suscribirseACola(LOCALIZED, socketBroker);
//	suscribirseACola(CAUGHT, socketBroker);
//
//	//Procedimiento auxiliar para que no rompa el server en las pruebas
//	int codigoOP = FINALIZAR;
//	send(socketBroker,(void*)&codigoOP,sizeof(opCode),0);
//    close(socketBroker);
//    log_info(logger,"Finalizó la conexión con el servidor\n");
//    log_info(logger,"El proceso team finalizó su ejecución\n");

//	list_destroy(entrenador->objetivos);
//	list_destroy(entrenador->pokemones);
//	free(entrenador);
//	liberarMemoria();
    return 0;
}
