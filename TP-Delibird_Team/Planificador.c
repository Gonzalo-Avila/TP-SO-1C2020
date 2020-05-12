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
bool estaEnEspera(t_entrenador *trainer) {
	bool verifica = false;
	if (((trainer->estado) == NUEVO) || ((trainer->estado) == BLOQUEADO))
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
	distancia->idEntrenador = id;

	return distancia;
}

bool menorDist(void *dist1, void *dist2) {
	bool verifica = false;

	if (((t_dist*) dist1)->dist < ((t_dist*) dist2)->dist)
		verifica = true;

	return verifica;
}

t_entrenador *entrenadorMasCercanoEnEspera(int posX, int posY) {
	t_list* listaDistancias = list_create();
	t_dist *distancia = malloc(sizeof(t_dist));
	int idEntrenadorConDistMenor;
	int i = 0;

	for (int i = 0; i < list_size(team->entrenadores); i++) {
		distancia = setearDistanciaEntrenadores(i, posX, posY);
		list_add(listaDistancias, distancia);
	}
	list_sort(listaDistancias, menorDist);

	while(listaDistancias){
		idEntrenadorConDistMenor = ((t_dist*) list_get(listaDistancias, i))->idEntrenador;

		if(estaEnEspera(((t_entrenador*) list_get(team->entrenadores,
				idEntrenadorConDistMenor))))
			break;
		i++;
	}//esta estructura se fija si el entrenador esta en espera.
	return ((t_entrenador*) list_get(team->entrenadores,
			idEntrenadorConDistMenor));
}

t_list *obtenerEntrenadoresReady(){
	t_list *entrenadoresReady = list_create();
	t_entrenador *entrenador = malloc(sizeof(t_entrenador));

	for(int i = 0;i < list_size(team->entrenadores);i++){
		entrenador = list_get(team->entrenadores,i);

		if(entrenador->estado == LISTO)
			list_add(entrenadoresReady,entrenador);
		}

	return entrenadoresReady;
}

bool hayaAlgunEntrenadorActivo(){
	bool verifica = false;
	t_entrenador *entrenador = malloc(sizeof(t_entrenador));

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


void activarHiloDe(int id){

}

	//Agarro un mensaje recibido del Broker q sea APP o LOC Y me sirva.
	//Antes entro a REGION CRITICA y bloqueo para que pueda sacar solo yo
	//y evitar que otro hilo meta mas mensajes mientras saco.
void planificarFifo(t_list *entrenadoresReady){
	//signal(hiloEntrenadorMasCercano);
	//pthread_cond_signal(list_get(listaCondsEntrenadores,entrenadorMasCercano->id));

		//se bloquea el planificador hasta que termine el ciclo de ejecucion del hilo.
		//wait(semPlanificador)
		t_entrenador *entrenador = malloc(sizeof(t_entrenador));

		entrenadoresReady = obtenerEntrenadoresReady();

		entrenador = list_get(entrenadoresReady,0);
		entrenador->estado = EJEC;
		activarHiloDe(entrenador->id);
}

void planificador(){
	t_list *entrenadoresReady = list_create();
	t_entrenador* entrenadorMasCercano = malloc(sizeof(t_entrenador));
	t_list *posx = list_create();
	t_list *posy = list_create();

	sem_wait(mutexMensajes);
	posx = ((t_pokemonesLocalized*)list_get(listaMensajesRecibidosLocalized,0))->x;
	posy = ((t_pokemonesLocalized*)list_get(listaMensajesRecibidosLocalized,0))->y;
	sem_post(mutexMensajes);

	entrenadorMasCercano = entrenadorMasCercanoEnEspera((int)list_get(posx,0),(int)list_get(posy,0));
	entrenadorMasCercano->estado = LISTO;//me aseguro de que tengo uno en READY antes de llamar para planificar

	for(int i = 0;i < list_size(team->entrenadores);i++){
		t_entrenador entr = malloc(sizeof(t_entrenador));
		entr = list_get(team->entrenadores,i);

		if(entr->estado == LISTO)
			list_add(entrenadoresReady,entr);
	}

	switch(team->algoritmoPlanificacion){
			case FIFO:
				planificarFifo(entrenadoresReady);
				break;
			//en caso que tengamos otro algoritmo usamos la funcion de ese algoritmo
			default:
				planificarFifo();
				break;
	}
}
