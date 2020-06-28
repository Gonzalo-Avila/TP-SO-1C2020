#include "Game-Card.h"


void procesarGET(mensajeRecibido * mensajeRecibido) {

	log_debug(logger, "[GET] Procesando");

	mensajeGet * msgGet = desarmarMensajeGET(mensajeRecibido);

	char * rutaPokemon;
	asprintf(&rutaPokemon, "%s%s%s", puntoDeMontaje, "/Files/",msgGet->pokemon); //puntoDeMontaje/Files/pikachu

	char * rutaMetadataPokemon;
	asprintf(&rutaMetadataPokemon, "%s%s", rutaPokemon, "/metadata.bin"); //puntodeMontaje/Files/pikachu/metadata.bin

	t_list * posicionesList = list_create();

	sem_wait(&semExistenciaPokemon);
	if (existeElArchivo(rutaMetadataPokemon)) {

		log_info(logger, "Existe %s", msgGet->pokemon);

		if(!existeSemaforo(rutaMetadataPokemon)){
			mutexPokemon * nuevoSemaforo =  crearNuevoSemaforo(rutaMetadataPokemon);
			list_add(semaforosPokemon,nuevoSemaforo);
		}
		sem_post(&semExistenciaPokemon);

		sem_t * mutexMetadata = obtenerMutexPokemon(rutaMetadataPokemon);
		t_config * metadataPokemon = intentarAbrirMetadataPokemon(mutexMetadata, rutaMetadataPokemon);

		char * archivoMappeado = mapearArchivo(rutaMetadataPokemon, metadataPokemon);

		int indexEntrada = 0;

		char** arrayDeEntradas = string_split(archivoMappeado, "\n");	//["5-4=3", "5-6=16" ,"3-1=201", NULL];

		char* entradaActual = arrayDeEntradas[indexEntrada];			// "5-4=3"

		// TODO
		// Si hay mas de 1 posicion hace una iteracion demas y rompe
		while(entradaActual!=NULL){
			char** posicionCantidad = string_split(entradaActual, "=");	//["5-4", "3", NULL]
			char* posicion = posicionCantidad[0];						// "5-4"

			log_debug(logger, "posicion encontrada: %s", posicion);

			char** posSplitteada = string_split(posicion, "-");			//["5", "4", NULL]

			posiciones * posXY = malloc(sizeof(posiciones));

			posXY->posicionX = atoi(posSplitteada[0]);					// 5
			posXY->posicionY = atoi(posSplitteada[1]);					// 4

			list_add(posicionesList, (void*) posXY);					// "4"		-> [5, 4, ...]

			indexEntrada++;
			entradaActual = arrayDeEntradas[indexEntrada];
		}

		config_set_value(metadataPokemon, "OPEN", "N");
		sleep(tiempoDeRetardo);

		sem_wait(mutexMetadata);
		config_save(metadataPokemon);
		sem_post(mutexMetadata);

		config_destroy(metadataPokemon);

		//		size = <longNombrePokemon> + <nombrePokemon> + <cantCoordenadas> + [<Posx> + <PosY>]*
		mensajeLocalized * msgLoc = armarMensajeLocalized(msgGet, posicionesList);
		int sizeMensaje = sizeof(uint32_t) + msgGet->longPokemon + sizeof(uint32_t) + list_size(posicionesList) * 2 * sizeof(uint32_t);

		log_debug(logger, "[GET] Enviando LOCALIZED");
		enviarMensajeBroker(LOCALIZED, mensajeRecibido->idMensaje, sizeMensaje, msgLoc);

		free(archivoMappeado);
		list_destroy(posicionesList);
		//list_destroy(msgLoc->paresDeCoordenada); msgLoc->paresDeCoordenada es lo mismo que posicionesList
		free(msgLoc->pokemon);
		free(msgLoc);

	}else{

		sem_post(&semExistenciaPokemon);
		int sizeMensaje = sizeof(uint32_t) + msgGet->longPokemon + sizeof(uint32_t);
		log_info(logger, "No existe el pokemon %s solicitado", msgGet->pokemon);
		mensajeLocalized * msgLoc = armarMensajeLocalized(msgGet, posicionesList);
		enviarMensajeBroker(LOCALIZED, mensajeRecibido->idMensaje, sizeMensaje, msgLoc);
		return;
	}

	log_debug(logger, "[GET] LOCALIZED enviado");

	free(rutaPokemon);
	free(rutaMetadataPokemon);
	free(msgGet->pokemon);
	free(msgGet);

}

mensajeLocalized * armarMensajeLocalized(mensajeGet * msgGet, t_list* posicionesList){

	mensajeLocalized * msgLoc = malloc(sizeof(mensajeLocalized));
	msgLoc->longPokemon = msgGet->longPokemon;
	msgLoc->pokemon = malloc(msgGet->longPokemon);
	memcpy(msgLoc->pokemon, msgGet->pokemon, msgGet->longPokemon);
	if(list_size(posicionesList)>0){
		msgLoc->listSize = list_size(posicionesList);
		msgLoc->paresDeCoordenada = list_create();
		list_add_all(msgLoc->paresDeCoordenada, posicionesList);
		//Para debugging. Anda ok.
		/*void imprimir(void* elem){
			posiciones * posis = (posiciones *) elem;
			log_info(logger, "posX: %d, posY: %d", posis->posicionX, posis->posicionY);
		}
		list_iterate(posicionesList, imprimir);
		list_iterate(msgLoc->posicionYCant, imprimir);*/
	}else{
		msgLoc->listSize = 0;
	}
	return msgLoc;

}

mensajeGet * desarmarMensajeGET(mensajeRecibido * mensajeRecibido) {

	mensajeGet * mensaje = malloc(sizeof(mensajeGet));
	int offset = 0;
	char finDeCadena='\0';

	memcpy(&(mensaje->longPokemon), mensajeRecibido->mensaje + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	mensaje->pokemon = malloc(mensaje->longPokemon+1);

	memcpy(mensaje->pokemon, mensajeRecibido->mensaje + offset, mensaje->longPokemon);
	offset += mensaje->longPokemon;

	memcpy(mensaje->pokemon+mensaje->longPokemon,&finDeCadena,1);
	log_info(logger, "%s", mensaje->pokemon );

	//memcpy(mensaje->pokemon, mensajeRecibido->mensaje + offset,mensaje->longPokemon);

	return mensaje;

}


