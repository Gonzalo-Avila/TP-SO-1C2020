#include "Team.h"

t_team *team = malloc(sizeof(t_team));
t_list *listaHilos;

void inicializarVariablesGlobales(){
	config = config_create("team.config");
	logger = log_create("team_logs","Team",1,LOG_LEVEL_TRACE);
	team->entrenadores = list_create()
	listaHilos = list_create()
}

//PODRIA IMPLEMENTAR UN for(int i = 0, i<= largoListaPosicionesEntrenadores,i++);
//QUE CREE EL HILO Y LO DETACHEE POR CADA TEAM---->crearEntrenador()


//16-04 | Nico | Ver comentario en obtenerObjetivos().
/*void obtenerObjetivosEntrenadores(){
	t_list *listaObjetivos =  list_create();

	listaObjetivos = config_get_array_value(config,"OBJETIVO_ENTRENADORES");

	}*/

//16-04 | Nico | Posible implementación genérica para obtener los parámetros que sean Lista de Listas.
t_list* obtenerObjetivos(char* clave){
	t_list *listaObjetivos = list_create();
	char** posiciones = malloc(sizeof(config_get_array_value(config, clave)));
	posiciones = config_get_array_value(config, clave);

	int contador = 0;
	while(posiciones[contador] != NULL){
		list_add(listaObjetivos, posiciones[contador]);
		contador++;
	}

	return listaObjetivos;
}


//16-04 | Nico | Ver comentario en obtenerObjetivos().
/*void obtenerPokemonesEntrenadores(){
	t_list *listaPokemones =  list_create();

	listaPokemones = config_get_array_value(config,"POKEMON_ENTRENADORES");
	}*/

/*MANEJA EL FUNCIONAMIENTO INTERNO DE CADA ENTRENADOR(trabajo en un hilo separado)*/
void gestionarEntrenador(t_entrenador *entrenador,char *pokemon,t_posicion posPokemon){

	//mueve el entrenador una posicion y ejecuta SLEEP(RETARDO_CICLO_CPU)
	while(moverEntrenadorPorUnidad(entrenador,posPokemon)){

	}
	//Tira un catch de pokemon
}

void crearEntrenador(t_entrenador* entrenador,char *pokemon,t_posicion posPokemon){
	pthread_t nuevoHilo;

	pthread_create(&nuevoHilo, NULL, (void*)gestionarEntrenador,entrenador);
	list_add(listaHilos,nuevoHilo);

	pthread_detach(nuevoHilo);
}

t_entrenador* armarEntrenador(t_list *posicionesEntrenadores,t_list *objetivosEntrenadores,t_list *pokemonesEntrenadores){
	t_entrenador* nuevoEntrenador = malloc(sizeof(t_entrenador));

	memcpy(nuevoEntrenador->pos, list_get(posicionesEntrenadores, 0), sizeof(t_posicion));
	memcpy(nuevoEntrenador->objetivos, list_get(objetivosEntrenadores, 0), sizeof(t_list));
	memcpy(nuevoEntrenador->pokemones, list_get(pokemonesEntrenadores, 0), sizeof(t_list));
	nuevoEntrenador->estado = NUEVO;
	return nuevoEntrenador;

	//mete eso en el struct t_entrenador
	//retorna el entrenador
}

void generarEntrenadores(){
	t_entrenador* unEntrenador = malloc(sizeof(t_entrenador));
	t_list* posiciones = obtenerObjetivos("POSICIONES_ENTRENADORES");
	t_list* objetivos = obtenerObjetivos("OBJETIVO_ENTRENADORES");
	t_list* pokemones = obtenerObjetivos("POKEMON_ENTRENADORES");

	for(int contador = 0; contador < list_size(posiciones); contador++) {
		unEntrenador = armarEntrenador(list_get(posiciones, contador), list_get(objetivos, contador), list_get(pokemones, contador));

		list_add(team->entrenadores,unEntrenador);
	}
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

//16-04 | Nico | No sé si debería poner "TIPO\n" o "TIPO" en los case
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

int main(){
	//Se setean todos los datos
	inicializarVariablesGlobales();

	char * ipServidor = malloc(strlen(config_get_string_value(config,"IP"))+1);
	ipServidor = config_get_string_value(config,"IP");

	char * puertoServidor = malloc(strlen(config_get_string_value(config,"PUERTO"))+1);
	puertoServidor = config_get_string_value(config,"PUERTO");

	e_algoritmo algoritmoPlanificador = obtenerAlgoritmoPlanificador();

	log_info(logger,"Se ha iniciado el cliente team\n");

    //Se crea la conexion con el broker. Esto posteriormente debe ir con un sistema de reintentos por si el broker esta off
	int socketBroker = crearConexionCliente(ipServidor,puertoServidor);
	log_info(logger,"Se ha establecido conexión con el servidor\nIP: %s\nPuerto: %s\nNúmero de socket: %d",
			 config_get_string_value(config,"IP"),config_get_string_value(config,"PUERTO"));

	free(ipServidor);
	free(puertoServidor);

	//Se inicializa el Team con los entrenadores

	//Se suscribe el Team a las colas
	suscribirseACola(APPEARED, socketBroker);
	suscribirseACola(LOCALIZED, socketBroker);
	suscribirseACola(CAUGHT, socketBroker);

	//Procedimiento auxiliar para que no rompa el server en las pruebas
	int codigoOP = FINALIZAR;
	send(socketBroker,(void*)&codigoOP,sizeof(opCode),0);
    close(socketBroker);
    log_info(logger,"Finalizó la conexión con el servidor\n");
    log_info(logger,"El proceso team finalizó su ejecución\n");

    log_destroy(logger);
    config_destroy(config);
    return 0;
}
