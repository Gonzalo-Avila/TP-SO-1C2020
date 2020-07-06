#include "Team.h"

void crearHiloPlanificador(){
	pthread_create(&hiloPlanificador, NULL, (void*) planificador, NULL);
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

//Esta funcion se podria codear para que sea una funcion generica, pero por el momento solo me sirve saber si está o no en ready.
bool estaEnEspera(t_entrenador *trainer) { // @suppress("Type cannot be resolved")
	bool verifica = false;
	if (trainer->estado == NUEVO || (trainer->estado == BLOQUEADO && !trainer->suspendido))
		verifica = true;

	return verifica;
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
	distancia->id = id;

	return distancia;
}

bool menorDist(void *dist1, void *dist2) {
	bool verifica = false;

	if (((t_dist*) dist1)->dist < ((t_dist*) dist2)->dist)
		verifica = true;

	return verifica;
}

bool puedaAtraparPokemones(t_entrenador *entrenador){
	return list_size(entrenador->pokemones) < list_size(entrenador->objetivos);
}
int entrenadorMasCercanoEnEspera(int posX, int posY) {
	t_list* listaDistancias = list_create();
	t_dist *distancia = malloc(sizeof(t_dist));
	int idEntrenadorConDistMenor = -1;
	int idEntrenadorAux;
	int j = 0;

	for (int i = 0; i < list_size(team->entrenadores); i++) {
		distancia = setearDistanciaEntrenadores(i, posX, posY);
		list_add(listaDistancias, distancia);
	}

	list_sort(listaDistancias, menorDist);

	while(j < list_size(listaDistancias)){
		log_error(logger,"Comienza la vuelta %d del while",j);

		idEntrenadorAux = ((t_dist*) list_get(listaDistancias, j))->id;

		if(estaEnEspera(((t_entrenador*) list_get(team->entrenadores,idEntrenadorAux))) &&
				puedaAtraparPokemones((t_entrenador*)list_get(team->entrenadores,idEntrenadorAux))){
			idEntrenadorConDistMenor = idEntrenadorAux;
			break;
		}

		log_error(logger,"Se completo la vuelta %d del while",j);

		j++;
	}//esta estructura se fija si el entrenador esta en espera.

	return idEntrenadorConDistMenor;

}

t_list *obtenerEntrenadoresReady(){
	t_list *entrenadoresReady = list_create();
	t_entrenador *entrenador = malloc(sizeof(t_entrenador));

	for(int i = 0;i < list_size(team->entrenadores);i++){
		entrenador = list_get(team->entrenadores,i);

		sem_wait(&mutexEntrenadores);
		if(entrenador->estado == LISTO)
			list_add(entrenadoresReady,entrenador);
		sem_post(&mutexEntrenadores);
		}

	return entrenadoresReady;
}

bool noSeCumplieronLosObjetivos(){
	bool verifica = false;
	t_entrenador *entrenador;

	int i = 0;

	while(i < list_size(team->entrenadores)){
		entrenador = list_get(team->entrenadores,i);

		if(entrenador->estado != FIN){
			verifica = true;
			break;
		}
		i++;
	}
	return verifica;
}


void ponerEnReadyAlMasCercano(int x, int y, char* pokemon){
	t_entrenador* entrenadorMasCercano;
	int idEntrenadorMasCercano;

	idEntrenadorMasCercano = entrenadorMasCercanoEnEspera(x,y);

	if(idEntrenadorMasCercano != -1){

		entrenadorMasCercano = (t_entrenador*) list_get(team->entrenadores,idEntrenadorMasCercano);


		sem_wait(&mutexEntrenadores);
		entrenadorMasCercano->estado = LISTO;//me aseguro de que tengo uno en READY antes de llamar para planificar
		entrenadorMasCercano->pokemonAAtrapar.pokemon = pokemon;
		entrenadorMasCercano->pokemonAAtrapar.pos[0] = x;
		entrenadorMasCercano->pokemonAAtrapar.pos[1] = y;
		list_add(listaDeReady,entrenadorMasCercano);
		sem_post(&mutexEntrenadores);
	}
	else{
		escaneoDeDeadlock();
	}

}

float calcularEstimacion(t_entrenador* entrenador){
	float rafagaAnterior = entrenador->datosSjf.duracionRafagaAnt;
	float estimadoAnterior = entrenador->datosSjf.estimadoRafagaAnt;
	//Chequeo si el entrenador fue desalojado
	if(!entrenador->datosSjf.fueDesalojado){
		float estimadoProximaRafaga = alfa*rafagaAnterior+(1-alfa)*estimadoAnterior;
		// Modifico valor del estimado actual
		entrenador->datosSjf.estimadoRafagaAct = estimadoProximaRafaga;
		//modifico valor del proximo estimado anterior
		entrenador->datosSjf.estimadoRafagaAnt = entrenador->datosSjf.estimadoRafagaAct;
	}
 	return entrenador->datosSjf.estimadoRafagaAct;
}

bool menorEstimacion(void* entrenador1, void* entrenador2) {
	float estimadoEntrenador1 = calcularEstimacion((t_entrenador*) entrenador1);
	float estimadoEntrenador2 = calcularEstimacion((t_entrenador*) entrenador2);
	return estimadoEntrenador1 < estimadoEntrenador2;
}

t_entrenador* entrenadorConMenorRafaga(){
		list_sort(listaDeReady,menorEstimacion);
		t_entrenador* entrenador = list_get(listaDeReady,0);
		return entrenador;
}

bool hayNuevoEntrenadorConMenorRafaga(t_entrenador* entrenador){
	if(!list_is_empty(listaDeReady)){
		list_sort(listaDeReady,menorEstimacion);
		t_entrenador* entrenador2 = list_get(listaDeReady,0);
		return entrenador->datosSjf.estimadoRafagaAct < entrenador2->datosSjf.estimadoRafagaAct;
	}
	else return false;
}

void activarHiloDe(int id){
	sem_post(&semEntrenadores[id]);
}

void activarHiloDeRR(int id, int quantum){
	for(int i = 0; i<quantum; i++)
		sem_post(&semEntrenadoresRR[id]);
}

void planificarFifo(){
		log_debug(logger,"Se activa el planificador");
		while(noSeCumplieronLosObjetivos()){
			t_entrenador *entrenador;

			sem_wait(&procesoEnReady);
			log_debug(logger,"Hurra, tengo algo en ready");

			if(!list_is_empty(listaDeReady)){
				//es necesario este if? si tengo el semaforo...

				sem_wait(&mutexEntrenadores);
				entrenador = list_get(listaDeReady,0);
				entrenador->estado = EJEC;
				sem_post(&mutexEntrenadores);

				list_remove(listaDeReady,0);
				//No esta mas en ready el entrenador, esta en EXEC.
				//El entrenador debe poder cambiar su estado para que no sea mas EXEC luego de ejecutar.
				//Porque esto es FIFO baby.

				activarHiloDe(entrenador->id);
			}
			sem_wait(&semPlanif);
		}
}

void planificarRR(){
	int quantum = atoi(config_get_string_value(config, "QUANTUM"));
	log_debug(logger,"Se activa el planificador");

			while(noSeCumplieronLosObjetivos()){
				t_entrenador *entrenador;

				sem_wait(&procesoEnReady);
				log_debug(logger,"Hurra, tengo algo en ready");

				if(!list_is_empty(listaDeReady)){
					//es necesario este if? si tengo el semaforo...

					sem_wait(&mutexEntrenadores);
					entrenador = list_get(listaDeReady,0);
					entrenador->estado = EJEC;
					sem_post(&mutexEntrenadores);

					list_remove(listaDeReady,0);
					//No esta mas en ready el entrenador, esta en EXEC.
					//El entrenador debe poder cambiar su estado para que no sea mas EXEC luego de ejecutar.

					activarHiloDe(entrenador->id);
					activarHiloDeRR(entrenador->id, quantum);
				}
				sem_wait(&semPlanif);
			}
}

void planificarSJFsinDesalojo(){
	log_debug(logger,"Se activa el planificador");

			while(noSeCumplieronLosObjetivos()){
				t_entrenador *entrenador;

				sem_wait(&procesoEnReady);
				log_debug(logger,"Hurra, tengo algo en ready");

				if(!list_is_empty(listaDeReady)){
					//es necesario este if? si tengo el semaforo...

					sem_wait(&mutexEntrenadores);
					entrenador = entrenadorConMenorRafaga();
					entrenador->estado = EJEC;
					sem_post(&mutexEntrenadores);
					//Remuevo de la listaDeReady el primer entrenador ya que la ordene por antes menor rafaga
					list_remove(listaDeReady,0);
					//No esta mas en ready el entrenador, esta en EXEC.
					//El entrenador debe poder cambiar su estado para que no sea mas EXEC luego de ejecutar.

					activarHiloDe(entrenador->id);
				}
				sem_wait(&semPlanif);
			}
}

void planificarSJFconDesalojo(){
	log_debug(logger,"Se activa el planificador");

			while(noSeCumplieronLosObjetivos()){
				t_entrenador *entrenador;

				sem_wait(&procesoEnReady);
				log_debug(logger,"Hurra, tengo algo en ready");

				if(!list_is_empty(listaDeReady)){
					//es necesario este if? si tengo el semaforo...

					sem_wait(&mutexEntrenadores);
					entrenador = entrenadorConMenorRafaga();
					entrenador->estado = EJEC;
					sem_post(&mutexEntrenadores);
					//Remuevo de la listaDeReady el primer entrenador ya que la ordene por antes menor rafaga
					list_remove(listaDeReady,0);
					//No esta mas en ready el entrenador, esta en EXEC.
					//El entrenador debe poder cambiar su estado para que no sea mas EXEC luego de ejecutar.

					activarHiloDe(entrenador->id);
				}
				sem_wait(&semPlanif);
			}
}

bool puedeExistirDeadlock(){
	t_entrenador *entrenador;
	bool verifica = false;

	for(int i = 0; i < list_size(team->entrenadores);i++){
		entrenador = list_get(team->entrenadores,i);

		if(entrenador->estado == BLOQUEADO || entrenador->estado == FIN){
			verifica = true;
		}
	}
	return verifica;
}

t_list *pokesNoObjetivoEnDeadlock(t_list *pokemonesPosibles,t_list *pokemonesObjetivoEntrenador){


	bool pokesEnDeadlock(void *poke1){
			bool verifica = false;
			char* objetivo;

			log_error(logger,"Ingresa al for para comparar objetivos con pokemones");

			for(int i = 0; i < list_size(pokemonesObjetivoEntrenador);i++){
				objetivo = list_get(pokemonesObjetivoEntrenador,i);

				log_error(logger,"Pokemon: %s, Objetivo[%d]: %s",poke1,i,objetivo);

				if(!string_equals_ignore_case((char*)poke1, objetivo))
					verifica = true;

			}

			return verifica;
		}

	return list_filter(pokemonesPosibles,pokesEnDeadlock);
}

void resolverDeadlockPotencial(t_entrenador *entrenadorPotencial, t_entrenador *entrenadorEnDeadlock){
	t_list *pokemonesNoObjetivoEnDeadlock = list_create();
	t_list *pokemonesNoObjetivoPotenciales = list_create();

	pokemonesNoObjetivoEnDeadlock = pokesNoObjetivoEnDeadlock(entrenadorEnDeadlock->pokemones,entrenadorEnDeadlock->objetivos);

	log_error(logger,"aca1");

	pokemonesNoObjetivoPotenciales = pokesNoObjetivoEnDeadlock(entrenadorPotencial->pokemones,entrenadorPotencial->objetivos);

	log_error(logger,"aca2");

	t_list *pokemonesAIntercambiarEntrenadorEnDeadlock = pokesNoObjetivoEnDeadlock(pokemonesNoObjetivoPotenciales,entrenadorEnDeadlock->objetivos);

	t_list *pokemonesAIntercambiarpotenciales = pokesNoObjetivoEnDeadlock(pokemonesNoObjetivoEnDeadlock,entrenadorPotencial->objetivos);

	if(list_size(pokemonesAIntercambiarEntrenadorEnDeadlock) != 0){
		log_error(logger,"Llega al mutex");
		sem_wait(&mutexEntrenadores);
		entrenadorEnDeadlock->estado = LISTO;
		entrenadorEnDeadlock->pokemonAAtrapar.pokemon = list_get(pokemonesAIntercambiarpotenciales,0);
		entrenadorEnDeadlock->pokemonAAtrapar.pos[0] = entrenadorPotencial->pos[0];
		entrenadorEnDeadlock->pokemonAAtrapar.pos[1] = entrenadorPotencial->pos[1];
		entrenadorEnDeadlock->datosDeadlock.pokemonAIntercambiar = malloc(strlen(list_get(pokemonesAIntercambiarEntrenadorEnDeadlock,0)) + 1);
		strcpy(entrenadorEnDeadlock->datosDeadlock.pokemonAIntercambiar,list_get(pokemonesAIntercambiarEntrenadorEnDeadlock,0));
		entrenadorEnDeadlock->datosDeadlock.idEntrenadorAIntercambiar = entrenadorPotencial->id;
		list_add(listaDeReady,entrenadorEnDeadlock);
		sem_post(&mutexEntrenadores);
		log_error(logger,"Pasa el mutex");
		sem_post(&procesoEnReady);
	}

}

void buscarEntrenadorParejaEnDeadlock(t_entrenador *entrenador){
	t_entrenador *entrenadorPosible;

	log_debug(logger,"Se Busca pareja en deadlock para entrenador %d",entrenador->id);

	for(int i = 0;i < list_size(team->entrenadores);i++){
		entrenadorPosible = list_get(team->entrenadores,i);

		if(entrenadorPosible->datosDeadlock.estaEnDeadlock && entrenadorPosible->id != entrenador->id)
			resolverDeadlockPotencial(entrenadorPosible,entrenador);
	}

}

void escaneoDeDeadlock(){

	log_debug(logger,"Se comienza el analisis de deadlock");

	if(puedeExistirDeadlock()){
		t_entrenador *entrenador;

		for(int i = 0;i < list_size(team->entrenadores);i++){
			entrenador = list_get(team->entrenadores,i);

			if(entrenador->cantidadMaxDePokes == list_size(entrenador->pokemones) && entrenador->estado != FIN){
				entrenador->datosDeadlock.estaEnDeadlock = true;
			}
		}

		for(int i = 0;i < list_size(team->entrenadores);i++){
			entrenador = list_get(team->entrenadores,i);

			if(entrenador->datosDeadlock.estaEnDeadlock){
				sem_wait(&resolviendoDeadlock);
				buscarEntrenadorParejaEnDeadlock(entrenador);
			}

		}
	}
}

void planificador(){
	switch(team->algoritmoPlanificacion){
			case FIFO:{
				planificarFifo();
				break;
			}
			case RR:{
				planificarRR();
				break;
			}
			case SJFSD:{
				planificarSJFsinDesalojo();
				break;
			}
			case SJFCD:{
				planificarSJFconDesalojo();
				break;
			}
			//en caso que tengamos otro algoritmo usamos la funcion de ese algoritmo
			default:{
				planificarFifo();
				break;
			}
	}
}
