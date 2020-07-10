#include "Team.h"

void registrarCambioDeContexto(){
	cambiosDeContexto++;
}

void registrarCicloDeCPU(){
	ciclosDeCPUTotales++;
}

/*Arma el Entrenador*/
t_entrenador* armarEntrenador(int id, char *posicionesEntrenador,char *objetivosEntrenador,
		char *pokemonesEntrenador, float estInicialEntrenador) {
	t_entrenador* nuevoEntrenador = malloc(sizeof(t_entrenador));

	t_list *posicionEntrenador = list_create();
	t_list *objetivoEntrenador = list_create();
	t_list *pokemonEntrenador = list_create();

	//TODO - Solucionar los leaks sin que rompa nada
	//--------------------------------------------------------------------------
	array_iterate_element((char **) string_split(posicionesEntrenador, "|"),
			(void *) enlistar, posicionEntrenador);
	array_iterate_element((char **) string_split(objetivosEntrenador, "|"),
			(void *) enlistar, objetivoEntrenador);
	//--------------------------------------------------------------------------

	if(pokemonesEntrenador != NULL)
		array_iterate_element((char **) string_split(pokemonesEntrenador, "|"),(void *) enlistar, pokemonEntrenador);

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
	//Pongo variable fue desalojado en true, asi en la primer vuelta el algoritmo toma las estimaciones iniciales.
	nuevoEntrenador->datosSjf.fueDesalojado = true;
	nuevoEntrenador->datosDeadlock.estaEnDeadlock = false;
	nuevoEntrenador->cantidadMaxDePokes = list_size(nuevoEntrenador->objetivos);
	nuevoEntrenador->pokemonAAtrapar.pokemon = malloc(MAXSIZE);

	list_destroy(posicionEntrenador);
	return nuevoEntrenador;
}

/* Genera los Entrenadores con los datos del Config */
void generarEntrenadores() {
	//t_entrenador* unEntrenador = malloc(sizeof(t_entrenador));
	t_entrenador * unEntrenador;
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

void removerObjetivosCumplidos(char *pokemon,t_list *listaObjetivos){

	bool esUnObjetivo(void *objetivo){
			bool verifica = false;

			if(string_equals_ignore_case((char *)objetivo, pokemon))
				verifica = true;

			return verifica;
		}

	list_remove_by_condition(listaObjetivos,esUnObjetivo);
}

void setearObjetivosDeTeam() {
	char *pokemon;

	bool esUnObjetivo(void *objetivo){
		bool verifica = false;

		if(string_equals_ignore_case((char *)objetivo, pokemon))
			verifica = true;

		return verifica;
	}

	for (int i = 0; i < list_size(team->entrenadores); i++) {
		t_entrenador *entrenador;

		entrenador = list_get(team->entrenadores, i);

		for (int j = 0; j < list_size(entrenador->objetivos); j++) {
				list_add(team->objetivo, list_get(entrenador->objetivos, j));
		}
	}

	for(int i = 0; i < list_size(team->entrenadores); i++){
		t_entrenador *entrenador;

		entrenador = list_get(team->entrenadores, i);

		for (int j = 0; j < list_size(entrenador->pokemones); j++) {

			pokemon = (char *)list_get(entrenador->pokemones,j);

			removerObjetivosCumplidos(pokemon,team->objetivo);
			removerObjetivosCumplidos(pokemon,entrenador->objetivos);
		}

		seCumplieronLosObjetivosDelEntrenador(entrenador);
	}
	//Se verifica por deadlock en caso que venga ya sin objetivos globales.
	verificarDeadlock();
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

void loggearPosicion(t_entrenador* entrenador) {
	log_debug(logger,
			"El entrenador id: %d se movio a la posicion [%d,%d]",
			entrenador->id, entrenador->pos[0], entrenador->pos[1]);
	log_debug(loggerOficial,
			"El entrenador id: %d se movio a la posicion [%d,%d]",
			entrenador->id, entrenador->pos[0], entrenador->pos[1]);
}

void moverXDelEntrenador(t_entrenador *entrenador){

	if(entrenador->pos[0] < entrenador->pokemonAAtrapar.pos[0]){
		entrenador->pos[0]++;
		loggearPosicion(entrenador);
		}
	else if(entrenador->pos[0] > entrenador->pokemonAAtrapar.pos[0]){
		entrenador->pos[0]--;
		loggearPosicion(entrenador);
		}
	else{
		//Si el entrenador ya esta en el X del pokemon, que mueva la Y.
		moverYDelEntrenador(entrenador);
	}

}

void moverYDelEntrenador(t_entrenador *entrenador){

	if(entrenador->pos[1] < entrenador->pokemonAAtrapar.pos[1]){
		entrenador->pos[1]++;
		loggearPosicion(entrenador);
	}
	else if(entrenador->pos[1] > entrenador->pokemonAAtrapar.pos[1]){
		entrenador->pos[1]--;
		loggearPosicion(entrenador);
	}
	else{
		//Si el entrenador ya esta en el Y del pokemon, que mueva la X
		moverXDelEntrenador(entrenador);
	}


}

bool estaEnLosObjetivos(char *pokemon){
	bool esUnObjetivo(void *elemento) {
		bool verifica = false;

		if (string_equals_ignore_case(pokemon, (char *)elemento)) {
			verifica = true;
		}
		else{
		}
		return verifica;
	}

	return list_any_satisfy(team->objetivo,esUnObjetivo);
}

void removerPokemonDeListaSegunCondicion(t_list* lista,char *pokemon){

	bool esElPokemonAAtrapar(void *poke){
		bool verifica = false;

		if(string_equals_ignore_case((char*)poke, pokemon))
			verifica = true;

		return verifica;
	}

	list_remove_by_condition(lista,esElPokemonAAtrapar);
}

void verificarDeadlock() {
	if(list_is_empty(team->objetivo)){
		log_info(loggerOficial, "Se inicia la deteccion de deadlock.");
		escaneoDeDeadlock();
	}
}
void seCumplieronLosObjetivosDelEntrenador(t_entrenador* entrenador) {
	if(list_is_empty(entrenador->objetivos)){
		sem_wait(&mutexEntrenadores);
		entrenador->estado = FIN;
		sem_post(&mutexEntrenadores);
	}
}


void intercambiar(t_entrenador *entrenador){
	t_entrenador *entrenadorParejaIntercambio = (t_entrenador*)list_get(team->entrenadores,entrenador->datosDeadlock.idEntrenadorAIntercambiar);
	log_debug(logger,"Estoy intercambiando el pokemon %s por %s",entrenador->datosDeadlock.pokemonAIntercambiar,entrenador->pokemonAAtrapar.pokemon);
	log_info(loggerOficial, "Se comienza el intercambio entre el entrenador %d y el entrenador %d", entrenador->id, entrenadorParejaIntercambio->id);

	sem_wait(&mutexEntrenadores);
	list_add(entrenador->pokemones,entrenador->pokemonAAtrapar.pokemon);
	removerPokemonDeListaSegunCondicion(entrenadorParejaIntercambio->pokemones,entrenador->pokemonAAtrapar.pokemon);
	list_add(entrenadorParejaIntercambio->pokemones,entrenador->datosDeadlock.pokemonAIntercambiar);
	removerPokemonDeListaSegunCondicion(entrenador->pokemones,entrenador->datosDeadlock.pokemonAIntercambiar);
	sem_post(&mutexEntrenadores);

	removerPokemonDeListaSegunCondicion(entrenador->objetivos,entrenador->pokemonAAtrapar.pokemon);
	removerPokemonDeListaSegunCondicion(entrenadorParejaIntercambio->objetivos,entrenador->datosDeadlock.pokemonAIntercambiar);

	seCumplieronLosObjetivosDelEntrenador(entrenador);
	seCumplieronLosObjetivosDelEntrenador(entrenadorParejaIntercambio);

	//Corresponde al requerimiento de que el intercambio debe demorar 5 ciclos de CPU.
	usleep(atoi(config_get_string_value(config, "RETARDO_CICLO_CPU")) * 5000000);

	for(int i = 0; i < 5; i++){
		registrarCicloDeCPU();
	}

	log_debug(logger,"Se finalizo el intercambio entre el entrenador %d y el entrenador %d",entrenador->id,entrenadorParejaIntercambio->id);
	log_info(loggerOficial,"Se finalizo el intercambio entre el entrenador %d y el entrenador %d",entrenador->id,entrenadorParejaIntercambio->id);
}

void gestionarEntrenadorFIFO(t_entrenador *entrenador){
	 while(1){
		sem_wait(&semEntrenadores[entrenador->id]);
		registrarCambioDeContexto();

		if(entrenador->estado != FIN){

			if(entrenador->estado == EJEC){

				bool alternadorXY = true;

				while(entrenador->pos[0] != entrenador->pokemonAAtrapar.pos[0] || entrenador->pos[1] != entrenador->pokemonAAtrapar.pos[1]){
					sem_wait(&mutexEntrenadores);

					if(alternadorXY){
						moverXDelEntrenador(entrenador);
					}
					else{
						moverYDelEntrenador(entrenador);
					}
					sem_post(&mutexEntrenadores);

					alternadorXY = !alternadorXY;

					registrarCicloDeCPU();
					usleep(atoi(config_get_string_value(config, "RETARDO_CICLO_CPU")) * 1000000);
				}
				if(!entrenador->datosDeadlock.estaEnDeadlock){
					enviarCatchDePokemon(ipServidor, puertoServidor, entrenador);
					registrarCambioDeContexto();

					log_info(loggerOficial, "Se desaloja al entrenador %d. Motivo: alcanzo su objetivo y envio un CATCH.", entrenador->id);

					sem_wait(&mutexEntrenadores);
					entrenador->estado = BLOQUEADO;
					entrenador->suspendido = true;
					sem_post(&mutexEntrenadores);
				}
				else{
					intercambiar(entrenador);
					sem_post(&resolviendoDeadlock);//Semaforo de finalizacion de deadlock.
				}
				sem_post(&semPlanif);
			 }
		}
		else{
			break;
		}
	}

}

void gestionarEntrenadorRR(t_entrenador* entrenador){

	while(1){
		sem_wait(&semEntrenadores[entrenador->id]);
		registrarCambioDeContexto();

		if(entrenador->estado != FIN){

			if(entrenador->estado == EJEC){

				bool alternadorXY = true;
				int quantum = atoi(config_get_string_value(config, "QUANTUM"));
				int contadorQuantum = quantum;

				while(entrenador->pos[0] != entrenador->pokemonAAtrapar.pos[0] || entrenador->pos[1] != entrenador->pokemonAAtrapar.pos[1]){
					if(contadorQuantum == 0){
						contadorQuantum = quantum;
						log_debug(logger, "El entrenador %d se quedó sin Quantum. Vuelve a la cola de ready.", entrenador->id);
						log_info(loggerOficial, "Se desaloja al entrenador %d. Motivo: Quantum agotado.", entrenador->id);
						registrarCambioDeContexto();
						entrenador->estado = LISTO; //Nico | Podría primero mandarlo a blocked y dps a ready, para respetar el modelo.
						list_add(listaDeReady,entrenador);
						sem_post(&procesoEnReady);
						sem_post(&semPlanif);
					}

					sem_wait(&semEntrenadoresRR[entrenador->id]);
					sem_wait(&mutexEntrenadores);

					if(alternadorXY){
						moverXDelEntrenador(entrenador);
					}
					else{
						moverYDelEntrenador(entrenador);
					}
					sem_post(&mutexEntrenadores);

					alternadorXY = !alternadorXY;

					registrarCicloDeCPU();
					usleep(atoi(config_get_string_value(config, "RETARDO_CICLO_CPU")) * 1000000);
					contadorQuantum--;
				}

				if(!entrenador->datosDeadlock.estaEnDeadlock){
					enviarCatchDePokemon(ipServidor, puertoServidor, entrenador);
					registrarCambioDeContexto();
					log_info(loggerOficial, "Se desaloja al entrenador %d. Motivo: alcanzo su objetivo y envio un CATCH.", entrenador->id);

					sem_wait(&mutexEntrenadores);
					entrenador->estado = BLOQUEADO;
					entrenador->suspendido = true;
					sem_post(&mutexEntrenadores);
				}
				else{
					intercambiar(entrenador);
					sem_post(&resolviendoDeadlock);//Semaforo de finalizacion de deadlock.
				}
				sem_post(&semPlanif);

			}
		}
		else{
			break;
		}
	}
}

void gestionarEntrenadorSJFsinDesalojo(t_entrenador* entrenador){
	while(1){
		sem_wait(&semEntrenadores[entrenador->id]);
		registrarCambioDeContexto();

		if(entrenador->estado != FIN){

			if(entrenador->estado == EJEC){

					bool alternadorXY = true;
					int rafagaActual;
					while(entrenador->pos[0] != entrenador->pokemonAAtrapar.pos[0] || entrenador->pos[1] != entrenador->pokemonAAtrapar.pos[1]){
						sem_wait(&mutexEntrenadores);

						if(alternadorXY){
							moverXDelEntrenador(entrenador);
						}
						else{
							moverYDelEntrenador(entrenador);
						}
						sem_post(&mutexEntrenadores);

						alternadorXY = !alternadorXY;

						registrarCicloDeCPU();
						usleep(atoi(config_get_string_value(config, "RETARDO_CICLO_CPU")) * 1000000);
						rafagaActual++;
					}
					entrenador->datosSjf.duracionRafagaAnt = rafagaActual;
					entrenador->datosSjf.fueDesalojado = false;

					if(!entrenador->datosDeadlock.estaEnDeadlock){
					enviarCatchDePokemon(ipServidor, puertoServidor, entrenador);
					registrarCambioDeContexto();
					log_info(loggerOficial, "Se desaloja al entrenador %d. Motivo: alcanzo su objetivo y envio un CATCH.", entrenador->id);

					sem_wait(&mutexEntrenadores);
					entrenador->estado = BLOQUEADO;
					entrenador->suspendido = true;
					sem_post(&mutexEntrenadores);
					}
					else{
					intercambiar(entrenador);
					sem_post(&resolviendoDeadlock);//Semaforo de finalizacion de deadlock.
				}
					sem_post(&semPlanif);
			}

		}
	}
}

void gestionarEntrenadorSJFconDesalojo(t_entrenador* entrenador){
	while(1){
		sem_wait(&semEntrenadores[entrenador->id]);
		registrarCambioDeContexto();

		if(entrenador->estado != FIN){

			if(entrenador->estado == EJEC){

					bool alternadorXY = true;
					int rafagaActual=0;
					while(entrenador->pos[0] != entrenador->pokemonAAtrapar.pos[0] || entrenador->pos[1] != entrenador->pokemonAAtrapar.pos[1]){
						sem_wait(&mutexEntrenadores);

						if(alternadorXY){
							moverXDelEntrenador(entrenador);
						}
						else{
							moverYDelEntrenador(entrenador);
						}
						sem_post(&mutexEntrenadores);

						alternadorXY = !alternadorXY;

						registrarCicloDeCPU();
						usleep(atoi(config_get_string_value(config, "RETARDO_CICLO_CPU")) * 1000000);
						rafagaActual++;
						//Actualizo estimacion
						entrenador->datosSjf.estimadoRafagaAct--;
						// Actualizo duracion real de rafaga
						if(!entrenador->datosSjf.fueDesalojado)
							entrenador->datosSjf.duracionRafagaAnt = rafagaActual;
						else entrenador->datosSjf.duracionRafagaAnt = entrenador->datosSjf.duracionRafagaAnt + rafagaActual;
						//Chequeo si hay nuevo entrenador en ready con menor rafaga que el actual
						//TODO
						//Chequear funcionamiento del desalojo. Necesitamos tener mas de un entrenador en ready.
						if(hayNuevoEntrenadorConMenorRafaga(entrenador)){
							rafagaActual=0;
							t_entrenador* entrenadorDesalojante = list_get(listaDeReady,0);
							log_debug(logger, "El entrenador %d fue desalojado por el entrenador %d. Vuelve a la cola de ready.", entrenador->id, entrenadorDesalojante->id);
							log_info(loggerOficial, "Se desaloja al entrenador %d. Motivo: fue desalojado por el entrenador %d.", entrenador->id, entrenadorDesalojante->id);
							registrarCambioDeContexto();
							entrenador->estado = LISTO; //Nico | Podría primero mandarlo a blocked y dps a ready, para respetar el modelo.
							list_add(listaDeReady,entrenador);
							entrenador->datosSjf.fueDesalojado = true;
							list_remove(listaDeReady,0);
							activarHiloDe(entrenadorDesalojante->id);
							//sem_post(&procesoEnReady);
							//sem_post(&semPlanif);
						}
					}
					entrenador->datosSjf.fueDesalojado = false;

					if(!entrenador->datosDeadlock.estaEnDeadlock){
						enviarCatchDePokemon(ipServidor, puertoServidor, entrenador);
						registrarCambioDeContexto();
						log_info(loggerOficial, "Se desaloja al entrenador %d. Motivo: alcanzo su objetivo y envio un CATCH.", entrenador->id);
						sem_wait(&mutexEntrenadores);
						entrenador->estado = BLOQUEADO;
						entrenador->suspendido = true;
						sem_post(&mutexEntrenadores);
					}
					else{
						intercambiar(entrenador);;
						sem_post(&resolviendoDeadlock);//Semaforo de finalizacion de deadlock.
					}
					sem_post(&semPlanif);
			}

		}
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


