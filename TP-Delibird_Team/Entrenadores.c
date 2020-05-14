#include "Team.h"

/*Arma el Entrenador*/
t_entrenador* armarEntrenador(int id, char *posicionesEntrenador,
		char *objetivosEntrenador, char *pokemonesEntrenador) {
	t_entrenador* nuevoEntrenador = malloc(sizeof(t_entrenador));
	t_list *posicionEntrenador = list_create();
	t_list *objetivoEntrenador = list_create();
	t_list *pokemonEntrenador = list_create();

	array_iterate_element((char **) string_split(posicionesEntrenador, "|"),
			(void *) enlistar, posicionEntrenador);
	array_iterate_element((char **) string_split(objetivosEntrenador, "|"),
			(void *) enlistar, objetivoEntrenador);
	array_iterate_element((char **) string_split(pokemonesEntrenador, "|"),
			(void *) enlistar, pokemonEntrenador);

	for (int i = 0; i < 2; i++) {
		nuevoEntrenador->pos[i] = atoi(list_get(posicionEntrenador, i));
	}
	nuevoEntrenador->id = id;
	nuevoEntrenador->objetivos = objetivoEntrenador;
	nuevoEntrenador->pokemones = pokemonEntrenador;
	nuevoEntrenador->estado = NUEVO; // Debugeando de mi cuenta que sin esta linea de codigo solo el ultimo elemento lo pasa a new

	list_destroy(posicionEntrenador);

	return nuevoEntrenador;
}

/* Genera los Entrenadores con los datos del Config */
void generarEntrenadores() {
	t_entrenador* unEntrenador = malloc(sizeof(t_entrenador));
	t_list* posiciones = list_create();
	t_list* objetivos = list_create();
	t_list* pokemones = list_create();

	obtenerDeConfig("POSICIONES_ENTRENADORES", posiciones);
	obtenerDeConfig("OBJETIVO_ENTRENADORES", objetivos);
	obtenerDeConfig("POKEMON_ENTRENADORES", pokemones);
	for (int contador = 0; contador < list_size(posiciones); contador++) {
		unEntrenador = armarEntrenador(contador, list_get(posiciones, contador),
				list_get(objetivos, contador), list_get(pokemones, contador));
		list_add(team->entrenadores, unEntrenador);
	}
	list_destroy(posiciones);
	list_destroy(objetivos);
	list_destroy(pokemones);
}

void setearObjetivosDeTeam() {
	t_entrenador *entrenador = malloc(sizeof(t_entrenador));

	for (int i = 0; i < list_size(team->entrenadores); i++) {
		entrenador = list_get(team->entrenadores, i);
		for (int j = 0; j < list_size(entrenador->objetivos); j++) {
			list_add(team->objetivo, list_get(entrenador->objetivos, j));
		}
	}
}

void setearCondsEntrenadores(){
	for(int i = 0; i < list_size(team->entrenadores);i++){
			pthread_cond_t *cond = malloc(sizeof(pthread_cond_t));
			t_entrenador *entrenador = malloc(sizeof(t_entrenador));

			pthread_cond_init(cond,1);
			entrenador = list_get(team->entrenadores,i);

			entrenador->cond = cond;
		}
}

void crearHiloEntrenador(t_entrenador* entrenador) {
	pthread_t nuevoHilo;
	t_listaHilos* nodoListaDeHilos = malloc(sizeof(t_listaHilos));

	pthread_create(&nuevoHilo, NULL, (void*) gestionarEntrenador, entrenador);

	nodoListaDeHilos->hilo = nuevoHilo;
	nodoListaDeHilos->id = entrenador->id;

	list_add(listaHilos, nodoListaDeHilos);

	pthread_detach(nuevoHilo);
}

t_dist *setearDistanciaPokemones(int id, int x, int y){
	t_dist *distancia = malloc(sizeof(t_dist));

//	distancia->dist = calcularDistancia((t_posicionEnMapa*)list_get(listaPosicionesInternas,0)),,x, y);
	//Todo fijarse como agarra la posicion del pokemon de las internas.
	//preguntar.
	distancia->id = id;

	return distancia;
}
int encontrarPokemonMasCercano(int x,int y){
	t_list* listaDistancias = list_create();
	t_dist *distancia = malloc(sizeof(t_dist));

	for (int i = 0; i < list_size(listaPosicionesInternas); i++) {
		distancia = setearDistanciaPokemones(i, x, y);
		list_add(listaDistancias, distancia);
	}
	list_sort(listaDistancias, menorDist);

	t_dist *idPokemonConDistMenor = ((t_dist*)list_get(listaDistancias,0))->id;

	return ((t_entrenador*) list_get(listaPosicionesInternas,
				idPokemonConDistMenor));
}

/*MANEJA EL FUNCIONAMIENTO INTERNO DE CADA ENTRENADOR(trabajo en un hilo separado)*/
void gestionarEntrenador(t_entrenador *entrenador) {

	pthread_mutex_lock(&mutexHilosEntrenadores);
	pthread_cond_wait(entrenador->cond,&mutexHilosEntrenadores);
	pthread_mutex_unlock(&mutexHilosEntrenadores);

	//pokemon mas cercano al entrenador
	int idPokeMasCercano = encontrarPokemonMasCercano(entrenador->pos[0],entrenador->pos[1]);
	//Todo esto es muy feo tengo que buscar una forma mas facil de acomodar esto.
	//Preguntar esto.
	while(entrenador->pos[0] != (int)list_get(((t_posicionEnMapa*)list_get(listaPosicionesInternas,idPokeMasCercano))->x,0)){
		sem_wait(mutexEntrenadores);
		if(entrenador->pos[0] < 'X'/*X POKEMON*/){
			entrenador->pos[0]++;
		}
		else if(entrenador->pos[0] > 'X'/*X POKEMON*/){
			entrenador->pos[0]--;
		}
		if(entrenador->pos[1] < 'Y'/*Y POKEMON*/){
			entrenador->pos[1]++;
		}
		else if(entrenador->pos[1] > 'Y' /*Y POKEMON*/){
			entrenador->pos[1]--;
		}
		if(entrenador->pos[0] == 'X'/*X POKEMON*/ && entrenador->pos[1] == 'Y'/* Y POKEMON */){
			//send(CATCH pokemon);
			//esperaCAUGHT
			/*if(pokemones.contains(objetivos)){
				estado = FIN;
			}
			else{
				estado = BLOCKED;
			}*/
		sem_post(mutexEntrenadores);
		}
		usleep(atoi(config_get_string_value(config, "RETARDO_CICLO_CPU")) * 100000);
	}

	//send catch
	//pthread_cond_wait();


}


