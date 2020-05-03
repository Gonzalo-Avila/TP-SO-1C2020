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
	pokemonRecibido = string_new();

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


/*MANEJA EL FUNCIONAMIENTO INTERNO DE CADA ENTRENADOR(trabajo en un hilo separado)*/
void gestionarEntrenador(t_entrenador *entrenador){
	//mover entrenador a posicion del pokemon que necesita
	//pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
	//pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


}

void crearHiloEntrenador(t_entrenador* entrenador){
	pthread_t nuevoHilo;
	t_listaHilos* nodoListaDeHilos=malloc(sizeof(t_listaHilos));

	pthread_create(&nuevoHilo, NULL, (void*)gestionarEntrenador,entrenador);

	nodoListaDeHilos->hilo = nuevoHilo;
	nodoListaDeHilos->idEntrenador = entrenador->id;

	list_add(listaHilos,nodoListaDeHilos);

	pthread_detach(nuevoHilo);
}


/*Arma el Entrenador*/
t_entrenador* armarEntrenador(int id,char *posicionesEntrenador,char *objetivosEntrenador,char *pokemonesEntrenador){
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
	nuevoEntrenador->id = id;
	nuevoEntrenador->objetivos = objetivoEntrenador;
	nuevoEntrenador->pokemones = pokemonEntrenador;
	nuevoEntrenador->estado = NUEVO; // Debugeando de mi cuenta que sin esta linea de codigo solo el ultimo elemento lo pasa a new

	list_destroy(posicionEntrenador);

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
		unEntrenador = armarEntrenador(contador,list_get(posiciones, contador), list_get(objetivos, contador), list_get(pokemones, contador));
		list_add(team->entrenadores,unEntrenador);
	}
	list_destroy(posiciones);
	list_destroy(objetivos);
	list_destroy(pokemones);
	//free(unEntrenador); Por algun motivo si liberamos la memoria de unEntrenador, el ultimo entrenador añadido a la lista cambia su id a 0.
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
	mensajeRecibido miMensajeRecibido;
	mensajeRecibido* mensajeAGuardar = malloc(sizeof(mensajeRecibido));

	while(1) {
		miMensajeRecibido = recibirMensajeDeBroker(socketBroker);

		if(miMensajeRecibido.codeOP == FINALIZAR) {
			break;
		}
		log_info(logger, "Mensaje recibido: %s\n", miMensajeRecibido.mensaje);

		//paso el struct a un puntero para guardarlo en la cola
		mensajeAGuardar = &miMensajeRecibido;
		queue_push(colaDeMensajes, mensajeAGuardar);
	}
}

void crearHiloParaAtenderBroker(int socketBroker) {
	pthread_t hiloAtenderBroker;
	pthread_create(&hiloAtenderBroker, NULL, (void*) atenderBroker, (void*) socketBroker); //No entiendo el warning
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
void gestionarMensajes(char* ip, char* puerto) {
	void* paqueteRecibido = malloc(sizeof(t_queue));
	t_mensaje* nuevoMensaje = malloc(sizeof(t_mensaje));
			   nuevoMensaje->pokemon = string_new();

	while(!queue_is_empty(colaDeMensajes)){
		  paqueteRecibido = queue_pop(colaDeMensajes);

		  nuevoMensaje = deserializar(paqueteRecibido);

		  if(nuevoMensaje->tipoDeMensaje == APPEARED) {
			  pokemonRecibido = nuevoMensaje->pokemon;

			  if(list_any_satisfy(team->objetivo, esUnObjetivo)){
				  enviarGetSegunObjetivo(ip, puerto, nuevoMensaje->pokemon);
			  }
		  }
		  else if(nuevoMensaje->tipoDeMensaje == LOCALIZED) {
			  planificar(nuevoMensaje->pokemon, nuevoMensaje->posicionX, nuevoMensaje->posicionY);
		  }
		  else if(nuevoMensaje->tipoDeMensaje == CAUGHT) {
			  //TODO CAUGHT
			  log_info(logger, "Recibi un CAUGHT. ¿Que es eso?¿Se come?");
		  }
	//TODO para enviar al planificador

	}
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

	for(int i = 0;i < list_size(team->entrenadores);i++){
		entrenador = list_get(team->entrenadores,i);
		for(int j = 0; j < list_size(entrenador->objetivos); j++){
			list_add(team->objetivo,list_get(entrenador->objetivos,j));
		}
	}
	list_destroy(entrenador->objetivos);
	free(entrenador);
}

void enviarGetSegunObjetivo(char *ip, char *puerto, char* pokemon){
//	for(int i = 0;i < list_size(team->objetivo);i++){
	int *socketBroker = malloc(sizeof(int));
	*socketBroker = crearConexionCliente(ip,puerto);

	enviarMensajeABroker(*socketBroker,GET,-1,sizeof(pokemon),pokemon);
	free(socketBroker);

}

float calcularDistancia(int posX1, int posY1,int posX2,int posY2){
	int cat1,cat2;
	float distancia;

	cat1 = abs(posX2 - posX1);
	cat2 = abs(posY2 - posY1);
	distancia=sqrt(pow(cat1,2)+pow(cat2,2));

	return distancia;
}

//setea la distancia de todos los entrenadores del team al pokemon localizado
t_list *setearDistanciaEntrenadores(t_team *team,int posX,int posY){
	t_entrenador *entrenador = malloc(sizeof(t_entrenador));
	t_list *listaDistancias = list_create();

	for(int i=0;i<list_size(team->entrenadores);i++){
			entrenador = list_get(team->entrenadores,i);

			t_dist *distancia = malloc(sizeof(t_dist));

			distancia->dist = calcularDistancia(entrenador->pos[0],entrenador->pos[1],posX,posY);
			distancia->idEntrenador = i;

			list_add(listaDistancias,distancia);

			free(distancia);
		}
	free(entrenador);

	return listaDistancias;
}


bool menorDist(t_dist *dist1,t_dist *dist2){
	bool verifica = false;

	if(dist1->dist < dist2->dist)
			verifica = true;

	return verifica;
}

//Llega como parametro el team, y las posiciones del pokemon localizado
t_entrenador *entrenadorMasCercano(t_team *team,int posX,int posY){
	t_list* listaDistancias = list_create();

	listaDistancias = setearDistanciaEntrenadores(team,posX,posY);

	list_sort(listaDistancias, (bool*) menorDist);

	int idEntrenadorConDistMenor = ((t_dist*)list_get(listaDistancias,0))->idEntrenador;

	return list_get(team->entrenadores,idEntrenadorConDistMenor);
}

//Esta funcion se podria codear para que sea una funcion generica, pero por el momento solo me sirve saber si está o no en ready.
bool estaEnEspera(void *entrenador){
	bool verifica = false;
	t_entrenador *trainer = malloc(sizeof(t_entrenador));
	trainer = entrenador;
			if(((trainer->estado) == NUEVO)||((trainer->estado) == BLOQUEADO))
				verifica = true;

	return verifica;
}

bool esUnObjetivo(void* objetivo) {
	bool verifica = false;
		if(string_equals_ignore_case(pokemonRecibido, objetivo)) {
			verifica = true;
		}
	return verifica;
}

/* Planificar */
void planificar(char* pokemon, int posicionX, int posicionY){
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

	for(int i = 0; i < list_size(team->entrenadores);i++){
		crearHiloEntrenador(list_get(team->entrenadores,i));
	}//Arma un hilo por entrenador

	t_entrenador* elEntrenadorMasCercano = malloc(sizeof(t_entrenador));
	elEntrenadorMasCercano = entrenadorMasCercano(team, posicionX, posicionY);
	elEntrenadorMasCercano->estado = LISTO;

}

int main(){
//	char * ipServidor = malloc(strlen(config_get_string_value(config,"IP"))+1);
//	ipServidor = config_get_string_value(config,"IP");

//	char * puertoServidor = malloc(strlen(config_get_string_value(config,"PUERTO"))+1);
//	puertoServidor = config_get_string_value(config,"PUERTO");

//	//Se crea la conexion con el broker. Esto posteriormente debe ir con un sistema de reintentos por si el broker esta off
//	int socketBroker = crearConexionCliente(ipServidor,puertoServidor);
//
//	//Inicializacion
//	inicializarVariablesGlobales();
//
//	generarEntrenadores();
//
//	//Se obtiene el algoritmo planificador
//	e_algoritmo algoritmoPlanificador = obtenerAlgoritmoPlanificador();
//
//	setearObjetivosDeTeam(team);
//
//	enviarGetSegunObjetivo(ipServidor,puertoServidor);
//
//	/*Ahora que hago, llamo al planificador o me quedo esperando a que me llegue un pokemon?*/
//
//	log_info(logger, "Algoritmo de planificacion a utilizar: %s\n", config_get_string_value(config, "ALGORITMO_PLANIFICACION"));
//
//	//Se crea un hilo para atender los mensajes del Broker
//	crearHiloParaAtenderBroker(socketBroker);
//
//	//Se suscribe el Team a las colas
//	suscribirseALasColas(socketBroker);
//
//	//Gestiono los mensajes de la cola
//	gestionarMensajes(ipServidor, puertoServidor);
//	log_info(logger, "se llega hasta acá bien");
//	//Procedimiento auxiliar para que no rompa el server en las pruebas
//	int codigoOP = FINALIZAR;
//
//	send(socketBroker,(void*)&codigoOP,sizeof(opCode),0);
//    close(socketBroker);
//
//    log_info(logger,"Finalizó la conexión con el servidor\n");
//    log_info(logger,"El proceso team finalizó su ejecución\n");
//
//	free(ipServidor);
//	free(puertoServidor);
//
//	liberarMemoria();

    return 0;
}

