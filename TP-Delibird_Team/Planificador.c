#include "Team.h"

void crearHiloPlanificador(){
	pthread_create(&hiloPlanificador, NULL, (void*) planificador, NULL);
}

void registrarDeadlockResuelto(){
	deadlocksResueltos++;
}

e_algoritmo obtenerAlgoritmoPlanificador() {
	char* algoritmo;

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
	//t_entrenador *entrenador = malloc(sizeof(t_entrenador));
	t_entrenador *entrenador;
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
	return list_size(entrenador->pokemones) < entrenador->cantidadMaxDePokes;
}

//Retorna id -1 si no encuentra un entrenador para planificar.
int entrenadorMasCercanoEnEspera(int posX, int posY) {
	t_list* listaDistancias = list_create();
	t_dist * distancia;
	int idEntrenadorConDistMenor = -1;
	int idEntrenadorAux;
	int j = 0;

	for (int i = 0; i < list_size(team->entrenadores); i++) {
		distancia = setearDistanciaEntrenadores(i, posX, posY);
		list_add(listaDistancias, distancia);
	}

	list_sort(listaDistancias, menorDist);

	while(j < list_size(listaDistancias)){

		idEntrenadorAux = ((t_dist*) list_get(listaDistancias, j))->id;

		if(estaEnEspera(((t_entrenador*) list_get(team->entrenadores,idEntrenadorAux))) && puedaAtraparPokemones((t_entrenador*)list_get(team->entrenadores,idEntrenadorAux))){
			idEntrenadorConDistMenor = idEntrenadorAux;
			break;
		}

		j++;
	}//esta estructura se fija si el entrenador esta en espera.

	list_destroy_and_destroy_elements(listaDistancias,(void *)destructorGeneral);

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
	sem_wait(&mutexEntrenadores);
	while(i < list_size(team->entrenadores)){
		entrenador = list_get(team->entrenadores,i);

		if(entrenador->estado != FIN){
			verifica = true;
			break;
		}
		i++;
	}
	sem_post(&mutexEntrenadores);

	return verifica;
}


void ponerEnReadyAlMasCercano(int x, int y, char* pokemon){
	t_entrenador* entrenadorMasCercano;
	int idEntrenadorMasCercano;

	idEntrenadorMasCercano = entrenadorMasCercanoEnEspera(x,y);

	if(idEntrenadorMasCercano != -1){

		entrenadorMasCercano = (t_entrenador*) list_get(team->entrenadores,idEntrenadorMasCercano);

		sem_wait(&mutexEntrenadores);
		entrenadorMasCercano->estado = LISTO;
		strcpy(entrenadorMasCercano->pokemonAAtrapar.pokemon, pokemon);
		entrenadorMasCercano->pokemonAAtrapar.pos[0] = x;
		entrenadorMasCercano->pokemonAAtrapar.pos[1] = y;
		list_add(listaDeReady,entrenadorMasCercano);
		sem_post(&mutexEntrenadores);
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
	activarHiloDe(id);

	for(int i = 0; i<quantum; i++)
		sem_post(&semEntrenadoresRR[id]);
}

void planificarFifo(){
		log_debug(logger,"Se activa el planificador");

		while(1){
			t_entrenador *entrenador;

			sem_wait(&procesoEnReady);
			if(noSeCumplieronLosObjetivos()){

				if(!list_is_empty(listaDeReady)){
					log_debug(logger,"Hurra, tengo algo en ready");

					sem_wait(&mutexEntrenadores);
					entrenador = list_get(listaDeReady,0);
					entrenador->estado = EJEC;
					sem_post(&mutexEntrenadores);

					list_remove(listaDeReady,0);
					activarHiloDe(entrenador->id);
					sem_wait(&semPlanif);
				}
			}
			else{
				log_debug(logger,"Se cumplieron los objetivos, finaliza el Planificador");
				break;
			}
		}
}

void planificarRR(){
	int quantum = atoi(config_get_string_value(config, "QUANTUM"));
	log_debug(logger,"Se activa el planificador");
	t_entrenador *entrenador;

	while(1){
			sem_wait(&procesoEnReady);

			if(noSeCumplieronLosObjetivos()){

				if(!list_is_empty(listaDeReady)){
					log_debug(logger,"Hurra, tengo algo en ready");

					sem_wait(&mutexEntrenadores);
					entrenador = list_get(listaDeReady,0);
					entrenador->estado = EJEC;
					sem_post(&mutexEntrenadores);

					list_remove(listaDeReady,0);

					activarHiloDeRR(entrenador->id, quantum);
					sem_wait(&semPlanif);
				}
			}
			else{
				log_debug(logger,"Se cumplieron los objetivos, finaliza el Planificador");
				break;
			}
		}
}

void planificarSJFsinDesalojo(){
	log_debug(logger,"Se activa el planificador");
	while(1){
		t_entrenador *entrenador;

		sem_wait(&procesoEnReady);

		if(noSeCumplieronLosObjetivos()){

			if(!list_is_empty(listaDeReady)){
				log_debug(logger,"Hurra, tengo algo en ready");

				sem_wait(&mutexEntrenadores);
				entrenador = entrenadorConMenorRafaga();
				entrenador->estado = EJEC;
				sem_post(&mutexEntrenadores);

				list_remove(listaDeReady,0);

				activarHiloDe(entrenador->id);
				sem_wait(&semPlanif);
			}
		}
		else{
			log_debug(logger,"Se cumplieron los objetivos, finaliza el Planificador");
			break;
	}   }
}

void planificarSJFconDesalojo(){
	log_debug(logger,"Se activa el planificador");
	while(1){
			t_entrenador *entrenador;

			sem_wait(&procesoEnReady);

			if(noSeCumplieronLosObjetivos()){

				if(!list_is_empty(listaDeReady)){
					log_debug(logger,"Hurra, tengo algo en ready");

					sem_wait(&mutexEntrenadores);
					entrenador = entrenadorConMenorRafaga();
					entrenador->estado = EJEC;
					sem_post(&mutexEntrenadores);

					list_remove(listaDeReady,0);

					activarHiloDe(entrenador->id);
					sem_wait(&semPlanif);
				}
			}
			else{
				log_debug(logger,"Se cumplieron los objetivos, finaliza el Planificador");
				break;
		}  }
}

bool puedeExistirDeadlock(){
	t_entrenador *entrenador;
	bool verifica = false;

	for(int i = 0; i < list_size(team->entrenadores);i++){
		entrenador = list_get(team->entrenadores,i);

		sem_wait(&mutexEntrenadores);
		if(entrenador->estado != FIN){
			verifica = true;
		}
		sem_post(&mutexEntrenadores);
	}
	return verifica;
}

//Obtiene una lista con los elementos de la primera lista que no estan en la segunda lista
//
//[Pikachu, Charmander] - [Charmander - Squirtle - Bulbasaur]
//
t_list *pokesNoObjetivoEnDeadlock(t_list *pokemonesPosibles,t_list *pokemonesObjetivoEntrenador){


	bool noLoNecesita(void *pokemon){
			bool verifica = true;
			char* objetivo;

			for(int i = 0; i < list_size(pokemonesObjetivoEntrenador);i++){
				objetivo = (char*)list_get(pokemonesObjetivoEntrenador,i);

				if(string_equals_ignore_case((char*)pokemon, objetivo))
				{
					verifica = false;
					break;
				}

			}

			return verifica;
		}

	return list_filter(pokemonesPosibles,noLoNecesita);
}

bool estaEnDeadlock(void *entrenador){
	bool verifica = false;

	if(((t_entrenador*)entrenador)->cantidadMaxDePokes == list_size(((t_entrenador*)entrenador)->pokemones) && ((t_entrenador*)entrenador)->estado != FIN){
		verifica = true;
		((t_entrenador*)entrenador)->datosDeadlock.estaEnDeadlock = true;
	}
	else{
		((t_entrenador*)entrenador)->datosDeadlock.estaEnDeadlock = false;
	}

	return verifica;
}

bool tieneAlgunoQueNecesita(t_list *lista1, t_list *lista2){
	char *elemento;
	bool verifica = false;

	bool estaEn(void *elem){
		return string_equals_ignore_case((char*)elem,elemento);
	}

	for(int i = 0 ; i < list_size(lista2);i++){
		elemento = list_get(lista2,i);

		if(list_any_satisfy(lista1, estaEn)){
			verifica = true;
			break;
		}
	}

	return verifica;
}



void realizarIntercambio(t_entrenador *entrenador, t_entrenador *entrenadorAIntercambiar){
		t_list *pokemonesNoRequeridos = pokesNoObjetivoEnDeadlock(entrenador->pokemones,entrenador->objetivos);
		t_list *pokemonesNoRequeridosAIntercambiar = pokesNoObjetivoEnDeadlock(entrenadorAIntercambiar->pokemones,entrenadorAIntercambiar->objetivos);

		bool pokemonAIntercambiar(void * elemento){
			bool verifica = false;

			for(int i = 0; i < list_size(pokemonesNoRequeridosAIntercambiar);i++){

				if(string_equals_ignore_case((char*)elemento, (char*)list_get(pokemonesNoRequeridosAIntercambiar,i))){
					verifica = true;
					break;
				}
			}
			return verifica;
		}

		char *pokeAIntercambiar = list_find(entrenador->objetivos,pokemonAIntercambiar);
		char *pokeADarQueNoQuiero= malloc(strlen(list_get(pokemonesNoRequeridos,0)) + 1);

		strcpy(pokeADarQueNoQuiero,list_get(pokemonesNoRequeridos,0));
		//Vos tenes uno que yo necesito, ahora tengo yo alguno que vos necesitas?

		log_debug(logger,"Pokemon a intercambiar del entrenador %d: %s\nPokemon a intercambiar del entrenador %d: %s",entrenador->id,list_get(pokemonesNoRequeridos,0),entrenadorAIntercambiar->id,pokeAIntercambiar);

		sem_wait(&mutexEntrenadores);
		entrenador->estado = LISTO;
		strcpy(entrenador->pokemonAAtrapar.pokemon,pokeAIntercambiar);
		entrenador->pokemonAAtrapar.pos[0] = entrenadorAIntercambiar->pos[0];
		entrenador->pokemonAAtrapar.pos[1] = entrenadorAIntercambiar->pos[1];

		entrenador->datosDeadlock.pokemonAIntercambiar = malloc(strlen(pokeADarQueNoQuiero) + 1);

		strcpy(entrenador->datosDeadlock.pokemonAIntercambiar,pokeADarQueNoQuiero);

		entrenador->datosDeadlock.idEntrenadorAIntercambiar = entrenadorAIntercambiar->id;
		list_add(listaDeReady,entrenador);

		list_destroy(pokemonesNoRequeridos);
		list_destroy(pokemonesNoRequeridosAIntercambiar);
		sem_post(&mutexEntrenadores);
		sem_post(&procesoEnReady);
}

void imprimirListaDeCadenas(t_list * listaDeCadenas){
	for(int i=0; i<list_size(listaDeCadenas);i++){
		log_info(logger,"%s",(char*)list_get(listaDeCadenas,i));
	}
}
void resolverDeadlock(t_list *entrenadoresEnDeadlock){
	t_entrenador *entrenador = list_get(entrenadoresEnDeadlock,0);
	t_entrenador *entrenadorAIntercambiar;

	bool swapDePokemonesValidos(void *entrPotencial){
		bool verifica;
		t_entrenador *entrPotencial1 = (t_entrenador*)entrPotencial;

		t_list *pokemonesNoRequeridosDeEntrPotencial = pokesNoObjetivoEnDeadlock(entrPotencial1->pokemones,entrPotencial1->objetivos);

		if(entrPotencial1->id != entrenador->id && tieneAlgunoQueNecesita(entrenador->objetivos,pokemonesNoRequeridosDeEntrPotencial))
			verifica = true;

		list_destroy(pokemonesNoRequeridosDeEntrPotencial);
		return verifica;
	}

	entrenadorAIntercambiar = list_find(entrenadoresEnDeadlock,swapDePokemonesValidos);

	realizarIntercambio(entrenador,entrenadorAIntercambiar);

}


void escaneoDeDeadlock(){
	log_debug(logger,"Se comienza el analisis de deadlock");

	if(puedeExistirDeadlock()){
		t_list *entrenadoresEnDeadlock = list_filter(team->entrenadores,estaEnDeadlock);
		int contadorDeDeadlocks = 0;

		while(!list_is_empty(entrenadoresEnDeadlock)){
			resolverDeadlock(entrenadoresEnDeadlock);
			sem_wait(&resolviendoDeadlock);
			list_destroy(entrenadoresEnDeadlock);
			entrenadoresEnDeadlock = list_filter(team->entrenadores,estaEnDeadlock);
			contadorDeDeadlocks++;
			registrarDeadlockResuelto();
		}
		log_debug(logger,"Se termino la resolucion de deadlocks");
		log_info(loggerOficial, "Finalizo la resolucion de deadlocks. Deadlocks resueltos = %d.", contadorDeDeadlocks);
	}
	else{
		log_debug(logger,"No hay deadlocks pendientes");
	}
	sem_post(&procesoEnReady);
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
