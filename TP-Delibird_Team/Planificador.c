#include "Team.h"

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

t_entrenador *entrenadorMasCercanoEnEspera(int posX, int posY) {
	t_list* listaDistancias = list_create();
	t_dist *distancia = malloc(sizeof(t_dist));
	int idEntrenadorConDistMenor;
	int j = 0;

	for (int i = 0; i < list_size(team->entrenadores); i++) {
		distancia = setearDistanciaEntrenadores(i, posX, posY);
		list_add(listaDistancias, distancia);
	}

	list_sort(listaDistancias, menorDist);

	while(listaDistancias){
		idEntrenadorConDistMenor = ((t_dist*) list_get(listaDistancias, j))->id;

		if(estaEnEspera(((t_entrenador*) list_get(team->entrenadores,idEntrenadorConDistMenor))) &&
				puedaAtraparPokemones((t_entrenador*)list_get(team->entrenadores,idEntrenadorConDistMenor)))
			break;
		j++;
	}//esta estructura se fija si el entrenador esta en espera.

	//problema: si no tengo ningun entrenador en espera se queda en el while?
	return ((t_entrenador*) list_get(team->entrenadores,
			idEntrenadorConDistMenor));
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
	t_entrenador* entrenadorMasCercano = malloc(sizeof(t_entrenador));

	entrenadorMasCercano = entrenadorMasCercanoEnEspera(x,y);
	sem_wait(&mutexEntrenadores);
	entrenadorMasCercano->estado = LISTO;//me aseguro de que tengo uno en READY antes de llamar para planificar
	entrenadorMasCercano->pokemonAAtrapar.pokemon = pokemon;
	entrenadorMasCercano->pokemonAAtrapar.pos[0] = x;
	entrenadorMasCercano->pokemonAAtrapar.pos[1] = y;
	list_add(listaDeReady,entrenadorMasCercano);
	sem_post(&mutexEntrenadores);
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
			//en caso que tengamos otro algoritmo usamos la funcion de ese algoritmo
			default:{
				planificarFifo();
				break;
			}
	}
}
