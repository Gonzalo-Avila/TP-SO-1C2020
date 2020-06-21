#include "Team.h"

/*Arma el Entrenador*/
t_entrenador* armarEntrenador(int id, char *posicionesEntrenador,char *objetivosEntrenador,
		char *pokemonesEntrenador, float estInicialEntrenador) {
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
	nuevoEntrenador->suspendido = false;
	nuevoEntrenador->datosSjf.duracionRafagaAnt = 0;
	nuevoEntrenador->datosSjf.estimadoRafagaAnt = 0;
	nuevoEntrenador->datosSjf.estimadoRafagaAct = estInicialEntrenador;

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
		float estimacion =(float)atof(config_get_string_value(config, "ESTIMACION_INICIAL"));//Puede romper el atof, puede estar redondeando la coma

	for (int contador = 0; contador < list_size(posiciones); contador++){
		unEntrenador = armarEntrenador(contador, list_get(posiciones, contador),
				list_get(objetivos, contador), list_get(pokemones, contador), estimacion);
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

void moverXDelEntrenador(t_entrenador *entrenador){

	if(entrenador->pos[0] < entrenador->pokemonAAtrapar.pos[0]){
			entrenador->pos[0]++;
		}
		else if(entrenador->pos[0] > entrenador->pokemonAAtrapar.pos[0]){
			entrenador->pos[0]--;
		}
	log_debug(logger,"El entrenador id: %d esta en la posicion [%d,%d]",entrenador->id,entrenador->pos[0],entrenador->pos[1]);

}

//	Nico | Se podría dejar los dos movimientos en una sola función y que reciba 0 o 1 para mover X o Y.
//		   No lo hice pq me parece más declarativo con las 2 funciones, pero se puede ver. Tambien se
//  	   Ahorraria el if de gestionarEntrenador.

void moverYDelEntrenador(t_entrenador *entrenador){

		if(entrenador->pos[1] < entrenador->pokemonAAtrapar.pos[1]){
			entrenador->pos[1]++;
		}
		else if(entrenador->pos[1] > entrenador->pokemonAAtrapar.pos[1]){
			entrenador->pos[1]--;
		}

	log_debug(logger,"El entrenador id: %d esta en la posicion [%d,%d]",entrenador->id,entrenador->pos[0],entrenador->pos[1]);

}

bool estaEnLosObjetivos(char *pokemon){

	bool esUnObjetivo(void *elemento) {
		bool verifica = false;

		if (string_equals_ignore_case(pokemon, (char *)elemento)) {
			verifica = true;
		}
		return verifica;
	}

	return list_any_satisfy(team->objetivo,esUnObjetivo);
}

void gestionarEntrenadorFIFO(t_entrenador *entrenador){
	 while(entrenador->estado != FIN){
			//me quedo esperando a estar en EJEC
			sem_wait(&semEntrenadores[entrenador->id]);
			bool alternadorXY = true;

			while(entrenador->pos[0] != entrenador->pokemonAAtrapar.pos[0] || entrenador->pos[1] != entrenador->pokemonAAtrapar.pos[1]){
				sem_wait(&mutexEntrenadores);

	//			Nico | Separado en X e Y para cumplir con el requerimiento que prohibe los movimientos diagonales.
				if(alternadorXY){
					moverXDelEntrenador(entrenador);
				}
				else{
					moverYDelEntrenador(entrenador);
				}
				sem_post(&mutexEntrenadores);

				alternadorXY = !alternadorXY;
				usleep(atoi(config_get_string_value(config, "RETARDO_CICLO_CPU")) * 1000000);
			}
			//TODO
	//		Comentamos pokemonRecibido porke funciona con otra logica.
	//			-acomodar appeared y caught.

			enviarCatchDePokemon(ipServidor, puertoServidor, entrenador);
			entrenador->estado = BLOQUEADO;
			entrenador->suspendido = true;

			//recibir caught
			//agrego el pokemon a la lista de pokemones del entrenador
			//remuevo al pokempn del obetivo global????
			//me bloqueo

			sem_post(&semPlanif);
	 }
}

void gestionarEntrenadorRR(t_entrenador* entrenador){
	 while(entrenador->estado != FIN){
				//me quedo esperando a estar en EJEC
				sem_wait(&semEntrenadores[entrenador->id]);
				bool alternadorXY = true;
				int quantum = atoi(config_get_string_value(config, "QUANTUM"));
				int contadorQuantum = quantum;

				while(entrenador->pos[0] != entrenador->pokemonAAtrapar.pos[0] || entrenador->pos[1] != entrenador->pokemonAAtrapar.pos[1]){
					printf("%d", contadorQuantum);
					if(!contadorQuantum){
						contadorQuantum = quantum;
						log_debug(logger, "El entrenador %d se quedó sin Quantum. Vuelve a la cola de ready.", entrenador->id);
						entrenador->estado = LISTO; //Nico | Podría primero mandarlo a blocked y dps a ready, para respetar el modelo.
						list_add(listaDeReady,entrenador);
						sem_post(&procesoEnReady);
						sem_post(&semPlanif);
					}

					sem_wait(&semEntrenadoresRR[entrenador->id]);
					sem_wait(&mutexEntrenadores);

		//			Nico | Separado en X e Y para cumplir con el requerimiento que prohibe los movimientos diagonales.
					if(alternadorXY){
						moverXDelEntrenador(entrenador);
					}
					else{
						moverYDelEntrenador(entrenador);
					}
					sem_post(&mutexEntrenadores);

					alternadorXY = !alternadorXY;
					usleep(atoi(config_get_string_value(config, "RETARDO_CICLO_CPU")) * 1000000);
					contadorQuantum--;
				}
				//TODO
		//		Comentamos pokemonRecibido porke funciona con otra logica.
		//			-acomodar appeared y caught.

				enviarCatchDePokemon(ipServidor, puertoServidor, entrenador);
				entrenador->estado = BLOQUEADO;
				entrenador->suspendido = true;

				//recibir caught
				//agrego el pokemon a la lista de pokemones del entrenador
				//remuevo al pokempn del obetivo global????
				//me bloqueo

				sem_post(&semPlanif);

	}
}

void gestionarEntrenadorSJFsinDesalojo(t_entrenador* entrenador){
	 while(entrenador->estado != FIN){
				//me quedo esperando a estar en EJEC
				sem_wait(&semEntrenadores[entrenador->id]);
				bool alternadorXY = true;
				int rafagaActual;
				while(entrenador->pos[0] != entrenador->pokemonAAtrapar.pos[0] || entrenador->pos[1] != entrenador->pokemonAAtrapar.pos[1]){
					sem_wait(&mutexEntrenadores);

		//			Separado en X e Y para cumplir con el requerimiento que prohibe los movimientos diagonales.
					if(alternadorXY){
						moverXDelEntrenador(entrenador);
					}
					else{
						moverYDelEntrenador(entrenador);
					}
					sem_post(&mutexEntrenadores);

					alternadorXY = !alternadorXY;
					usleep(atoi(config_get_string_value(config, "RETARDO_CICLO_CPU")) * 1000000);
					rafagaActual++;
				}
				entrenador->datosSjf.duracionRafagaAnt = rafagaActual;
				//TODO
		//		Comentamos pokemonRecibido porke funciona con otra logica.
		//			-acomodar appeared y caught.

				enviarCatchDePokemon(ipServidor, puertoServidor, entrenador);
				entrenador->estado = BLOQUEADO;
				entrenador->suspendido = true;

				//recibir caught
				//agrego el pokemon a la lista de pokemones del entrenador
				//remuevo al pokempn del obetivo global????
				//me bloqueo

				sem_post(&semPlanif);

	}
}

void gestionarEntrenadorSJFconDesalojo(t_entrenador* entrenador){
	 while(entrenador->estado != FIN){
				//me quedo esperando a estar en EJEC
				sem_wait(&semEntrenadores[entrenador->id]);
				bool alternadorXY = true;
				int rafagaActual=0;
				while(entrenador->pos[0] != entrenador->pokemonAAtrapar.pos[0] || entrenador->pos[1] != entrenador->pokemonAAtrapar.pos[1]){
					sem_wait(&mutexEntrenadores);
		//			Separado en X e Y para cumplir con el requerimiento que prohibe los movimientos diagonales.
					if(alternadorXY){
						moverXDelEntrenador(entrenador);
					}
					else{
						moverYDelEntrenador(entrenador);
					}
					sem_post(&mutexEntrenadores);

					alternadorXY = !alternadorXY;
					usleep(atoi(config_get_string_value(config, "RETARDO_CICLO_CPU")) * 1000000);
					rafagaActual++;
					//Actualizo estimacion
					entrenador->datosSjf.estimadoRafagaAct--;
					// Actualizo duracion real de rafaga
					if(!entrenador->datosSjf.fueDesalojado)
						entrenador->datosSjf.duracionRafagaAnt = rafagaActual;
					else entrenador->datosSjf.duracionRafagaAnt = entrenador->datosSjf.duracionRafagaAnt + rafagaActual;

					//Chequeo si hay nuevo entrenador en ready con menor rafaga que el actual
					// TODO
					// Tira segmentation fault, es en hayNuevoEntrenadorConMenorRafaga
					if(hayNuevoEntrenadorConMenorRafaga(entrenador)){
						rafagaActual=0;
						t_entrenador* entrenadorDesalojante = list_get(listaDeReady,0);
						log_debug(logger, "El entrenador %d fue desalojado por el entranador %d. Vuelve a la cola de ready.", entrenador->id, entrenadorDesalojante->id);
						entrenador->estado = LISTO; //Nico | Podría primero mandarlo a blocked y dps a ready, para respetar el modelo.
						list_add(listaDeReady,entrenador);
						entrenador->datosSjf.fueDesalojado = true;
						sem_post(&procesoEnReady);
						sem_post(&semPlanif);
					}
				}
				//TODO
		//		Comentamos pokemonRecibido porke funciona con otra logica.
		//			-acomodar appeared y caught.

				enviarCatchDePokemon(ipServidor, puertoServidor, entrenador);
				entrenador->estado = BLOQUEADO;
				entrenador->suspendido = true;

				//recibir caught
				//agrego el pokemon a la lista de pokemones del entrenador
				//remuevo al pokempn del obetivo global????
				//me bloqueo

				sem_post(&semPlanif);

	}
}

/*MANEJA EL FUNCIONAMIENTO INTERNO DE CADA ENTRENADOR(trabajo en un hilo separado)*/
void gestionarEntrenador(t_entrenador *entrenador) {
	switch(team->algoritmoPlanificacion){
		case FIFO:{
			gestionarEntrenadorFIFO(entrenador);
			break;
		}
		case RR:{
			gestionarEntrenadorRR(entrenador);
			break;
		}
		case SJFSD:{
			gestionarEntrenadorSJFsinDesalojo(entrenador);
			break;
		}
		case SJFCD:{
			gestionarEntrenadorSJFconDesalojo(entrenador);
			break;
		}
		default:{
			gestionarEntrenadorFIFO(entrenador);
			break;
		}
	}
}


