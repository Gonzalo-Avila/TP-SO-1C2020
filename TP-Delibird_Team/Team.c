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
	colaDeMensajes = queue_create();

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


/*Arma el Entrenador*/
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

/* Genera los Entrenadores con los datos del Config */
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

/* Planificador */
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
	if(strcmp(algoritmo, "FIFO") == 0){
		return FIFO;
	}
	else if(strcmp(algoritmo, "RR") == 0){
		return RR;
	}
	else if(strcmp(algoritmo, "SJFCD") == 0){
		return SJFCD;
	}
	else if(strcmp(algoritmo, "SJFSD") == 0){
		return SJFSD;
	}
	else{
		log_info(logger, "No se ingresó un algoritmo válido en team.config. Se toma FIFO por defecto.\n");
		return FIFO;
	}
}

/* Atender al Broker */
void atenderBroker(int socketBroker) {
	tPaquete* mensajeRecibido = malloc(sizeof(tPaquete));

	while(1) {
		mensajeRecibido = recibirMensaje(socketBroker);

		if(mensajeRecibido->codOperacion == FINALIZAR) {
			break;
		}
		log_info(logger, "Mensaje recibido: %s\n", mensajeRecibido->buffer->stream);
		queue_push(colaDeMensajes, mensajeRecibido);
	}

	//Libero memoria
	free(mensajeRecibido->buffer);
	free(mensajeRecibido);
}

void crearHiloParaAtenderBroker(int socketBroker) {
	pthread_t hiloAtenderBroker;
	pthread_create(&hiloAtenderBroker, NULL, (void*) atenderBroker, socketBroker); //No entiendo el warning
	pthread_detach(hiloAtenderBroker);
}

/* Se suscribe a las colas del Broker */
void suscribirseALasColas(int socketBroker) {
	suscribirseACola(APPEARED, socketBroker);
	suscribirseACola(LOCALIZED, socketBroker);
	suscribirseACola(CAUGHT, socketBroker);
}

/* Parsear mensaje: funcion para TESTING */
t_mensaje* parsearMensaje(char* mensaje) {
	t_mensaje* nuevoMensaje = malloc(sizeof(t_mensaje));
	char** mensajeSeparado = malloc(strlen(mensaje));

	nuevoMensaje->pokemonSize = strlen(mensajeSeparado[1]);
	//trim en caso de que tenga espacios atras o adelante
	string_trim(mensajeSeparado);

	mensajeSeparado = string_split(mensaje, " ");

	//el primer elemento siempre debe ser Tipo de Mensaje, el segundo un pokemon, y el tercero y cuarto son opcionales y tienen las coordenadas
	for(int i = 0; i < 4; i++) {
			if(i == 0) {
				if(strcmp("APPEARED", mensajeSeparado[i]) == 0){
					nuevoMensaje->tipoDeMensaje = APPEARED;
					}
				else if(strcmp("LOCALIZED", mensajeSeparado[i]) == 0){
					nuevoMensaje->tipoDeMensaje = LOCALIZED;
					}
				else if(strcmp("CAUGHT", mensajeSeparado[i]) == 0){
					nuevoMensaje->tipoDeMensaje = CAUGHT;
					}
			}
			if(i == 1) {
				strcpy(mensajeSeparado[i], nuevoMensaje->pokemon);
			}

			//Si el mensaje contiene coordenadas, agregarlas
			if(nuevoMensaje->tipoDeMensaje == LOCALIZED || nuevoMensaje->tipoDeMensaje == CAUGHT){
				if(i == 2) {
					nuevoMensaje->posicionX = atoi(mensajeSeparado[i]);
				}
				if(i == 3) {
					nuevoMensaje->posicionY = atoi(mensajeSeparado[i]);
				}
			}
			/*else if(i > 1){
				break;
			}*/ //22-04 | Nico | Dejo este if comentado pq no se si cortaría todo el ciclo. Sería solo un cambio para evitar que el ciclo
				//se ejecute 2 veces mas en caso de que no haya coordenadas
	}
	return nuevoMensaje;
}

t_mensaje* deserializar(void* paquete) {
	t_mensaje* mensaje = malloc(sizeof(paquete));
	int offset = 0;

	memcpy((void*)mensaje->tipoDeMensaje, paquete, sizeof(int));
	offset += sizeof(int);
	memcpy((void*)mensaje->pokemonSize, paquete+offset, sizeof(int));
	offset += sizeof(int);
	memcpy((void*)mensaje->pokemon, paquete+offset, mensaje->pokemonSize);
	offset += mensaje->pokemonSize;
	if(mensaje->tipoDeMensaje == LOCALIZED || mensaje->tipoDeMensaje == CAUGHT) {
		memcpy((void*)mensaje->posicionX, paquete+offset, sizeof(int));
		offset += sizeof(int);
		memcpy((void*)mensaje->posicionY, paquete+offset, sizeof(int));
	}
	return mensaje;
}

/* Gestiona los mensajes de la cola */
void gestionarMensajes() {
	void* paqueteRecibido = malloc(sizeof(t_queue));
		  paqueteRecibido = queue_pop(colaDeMensajes);
	t_mensaje* nuevoMensaje = malloc(sizeof(t_mensaje));
			   nuevoMensaje->pokemon = string_new();

	nuevoMensaje = deserializar(paqueteRecibido);
	//TODO para enviar al planificador


}

/* Liberar memoria al finalizar el programa */
void liberarMemoria(){
	for(int i = 0;i < list_size(team->entrenadores);i++){
		list_destroy(((t_entrenador*)(list_get(team->entrenadores,i)))->objetivos);
		list_destroy(((t_entrenador*)(list_get(team->entrenadores,i)))->pokemones);
	}
	list_destroy(team->objetivo);
	list_destroy(team->entrenadores);
	list_destroy(listaHilos);
	queue_destroy(colaDeReady);
	queue_destroy(colaDeBloqued);
	queue_destroy(colaDeMensajes);
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

	//Inicializacion
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

//	char * puertoServidor = malloc(strlen(config_get_string_value(config,"PUERTO"))+1);
//	puertoServidor = config_get_string_value(config,"PUERTO");

    //Se crea la conexion con el broker. Esto posteriormente debe ir con un sistema de reintentos por si el broker esta off
//	int socketBroker = crearConexionCliente(ipServidor,puertoServidor);
//	log_info(logger,"Se ha iniciado el cliente team\n");
//	log_info(logger,"Se ha establecido conexión con el servidor\nIP: %s\nPuerto: %s\nNúmero de socket: %d",
//			 ipServidor,puertoServidor, socketBroker);
//
//	free(ipServidor);
//	free(puertoServidor);

	//Se obtiene el algoritmo planificador
//	e_algoritmo algoritmoPlanificador = obtenerAlgoritmoPlanificador();
//		log_info(logger, "Algoritmo de planificacion a utilizar: %s\n", config_get_string_value(config, "ALGORITMO_PLANIFICACION"));

	// Se crea un hilo para atender los mensajes del Broker
//	crearHiloParaAtenderBroker(socketBroker);

	//Se suscribe el Team a las colas
//	suscribirseALasColas(socketBroker);

	//Gestiono los mensajes de la cola
//	gestionarMensajes();

//	//Procedimiento auxiliar para que no rompa el server en las pruebas
//	int codigoOP = FINALIZAR;
//	send(socketBroker,(void*)&codigoOP,sizeof(opCode),0);
//    close(socketBroker);
//    log_info(logger,"Finalizó la conexión con el servidor\n");
//    log_info(logger,"El proceso team finalizó su ejecución\n");
//
//	liberarMemoria();

    return 0;
}

