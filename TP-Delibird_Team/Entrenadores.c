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

void crearHiloEntrenador(t_entrenador* entrenador) {
	pthread_t nuevoHilo;
	t_listaHilos* nodoListaDeHilos = malloc(sizeof(t_listaHilos));

	pthread_create(&nuevoHilo, NULL, (void*) gestionarEntrenador, entrenador);

	nodoListaDeHilos->hilo = nuevoHilo;
	nodoListaDeHilos->id = entrenador->id;

	list_add(listaHilos, nodoListaDeHilos);

	pthread_detach(nuevoHilo);
}

void moverEntrenador(t_entrenador *entrenador){

	if(entrenador->pos[0] < entrenador->posAMover[0]){
			entrenador->pos[0]++;
		}
		else if(entrenador->pos[0] > entrenador->posAMover[0]){
			entrenador->pos[0]--;
		}
		if(entrenador->pos[1] < entrenador->posAMover[1]){
			entrenador->pos[1]++;
		}
		else if(entrenador->pos[1] > entrenador->posAMover[1]){
			entrenador->pos[1]--;
		}

}

/*MANEJA EL FUNCIONAMIENTO INTERNO DE CADA ENTRENADOR(trabajo en un hilo separado)*/
void gestionarEntrenador(t_entrenador *entrenador) {
	//me quedo esperando a estar en EJEC
	sem_wait(&semEntrenadores[entrenador->id]);

	while(entrenador->pos[0] != entrenador->posAMover[0] && entrenador->pos[1] != entrenador->posAMover[1]){
		sem_wait(&mutexEntrenadores);
		moverEntrenador(entrenador);
		sem_post(&mutexEntrenadores);
		usleep(atoi(config_get_string_value(config, "RETARDO_CICLO_CPU")) * 100000);
	}
	//TODO ver si el pokemonRecibido es el string correcto, o de dónde sacarlo
	enviarCatchDePokemon(ipServidor, puertoServidor, pokemonRecibido, entrenador->posAMover[0], entrenador->posAMover[1]);
	entrenador->estado = BLOQUEADO;

	//sem_signal(&semPlanif);
	//sem_wait(&semEntrenadores[entrenador->id]);
	//recibir caught
	//me bloqueo

}


