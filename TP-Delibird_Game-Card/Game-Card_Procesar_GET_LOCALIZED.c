#include "Game-Card.h"


void procesarGET(mensajeRecibido * mensajeRecibido) {

	log_debug(logger, "Procesando mensaje GET...");

	mensajeGet * msgGet = desarmarMensajeGET(mensajeRecibido);

	char * rutaPokemon;
	asprintf(&rutaPokemon, "%s%s%s", puntoDeMontaje, "/Files/",msgGet->pokemon); //puntoDeMontaje/Files/pikachu

	char * rutaMetadataPokemon;
	asprintf(&rutaMetadataPokemon, "%s%s", rutaPokemon, "/metadata.bin"); //puntodeMontaje/Files/pikachu/metadata.bin

	t_list * posicionesList = list_create();

	sem_wait(&mutexListaDeSemaforos);
	if(!existeSemaforo(rutaMetadataPokemon)){
			mutexPokemon * nuevoSemaforo =  crearNuevoSemaforo(rutaMetadataPokemon);
			list_add(semaforosPokemon,nuevoSemaforo);
	}
	sem_t * mutexMetadata = obtenerMutexPokemon(rutaMetadataPokemon);
	sem_post(&mutexListaDeSemaforos);

	t_config * metadataPokemon;
	bool operacionFinalizada = false;

	while(!operacionFinalizada){

		metadataPokemon=config_create(rutaMetadataPokemon);
		if (existeElArchivo(rutaMetadataPokemon)) {

			log_info(logger, "Existe %s", msgGet->pokemon);
			if (strcmp(config_get_string_value(metadataPokemon, "OPEN"), "N") == 0) {

				config_set_value(metadataPokemon, "OPEN", "Y");
				config_save(metadataPokemon);
				sem_post(mutexMetadata);

				char * archivoMappeado = mapearArchivo(rutaMetadataPokemon, metadataPokemon);

				int indexEntrada = 0;

				char** arrayDeEntradas = string_split(archivoMappeado, "\n");	//["5-4=3", "5-6=16" ,"3-1=201", NULL];

				char* entradaActual = arrayDeEntradas[indexEntrada];			// "5-4=3"

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

					liberarStringSplitteado(posicionCantidad);
					liberarStringSplitteado(posSplitteada);
				}

				liberarStringSplitteado(arrayDeEntradas);

				config_set_value(metadataPokemon, "OPEN", "N");
				sleep(tiempoDeRetardo);

				sem_wait(mutexMetadata);
				config_save(metadataPokemon);
				sem_post(mutexMetadata);

				mensajeLocalized * msgLoc = armarMensajeLocalized(msgGet, posicionesList);
				int sizeMensaje = sizeof(uint32_t) + msgGet->longPokemon + sizeof(uint32_t) + list_size(posicionesList) * 2 * sizeof(uint32_t);

				log_debug(logger, "[GET] Enviando LOCALIZED");
				enviarMensajeBroker(LOCALIZED, mensajeRecibido->idMensaje, sizeMensaje, msgLoc);

				operacionFinalizada=true;

				free(archivoMappeado);


				free(msgLoc->pokemon);
				list_destroy(msgLoc->paresDeCoordenada);
				free(msgLoc);
			}
			else{
				sem_post(mutexMetadata);
				sleep(tiempoDeReintentoDeAcceso);
			}
			config_destroy(metadataPokemon);
		}
		else{
			int sizeMensaje = sizeof(uint32_t) + msgGet->longPokemon + sizeof(uint32_t);
			log_info(logger, "No existe el pokemon %s solicitado", msgGet->pokemon);
			mensajeLocalized * msgLoc = armarMensajeLocalized(msgGet, posicionesList);
			enviarMensajeBroker(LOCALIZED, mensajeRecibido->idMensaje, sizeMensaje, msgLoc);

			free(msgLoc->pokemon);
			list_destroy(msgLoc->paresDeCoordenada);
			free(msgLoc);
			operacionFinalizada=true;
		}
	}

	log_debug(logger, "[GET] LOCALIZED enviado");
	list_destroy_and_destroy_elements(posicionesList,(void *) destroyer);
	free(rutaPokemon);
	free(rutaMetadataPokemon);
	free(msgGet->pokemon);
	free(msgGet);
	free(mensajeRecibido->mensaje);
	free(mensajeRecibido);

}
void destroyer(void * nodo){
	posiciones * elem = (posiciones *) nodo;
	free(elem);
}
mensajeLocalized * armarMensajeLocalized(mensajeGet * msgGet, t_list* posicionesList){

	mensajeLocalized * msgLoc = malloc(sizeof(mensajeLocalized));
	msgLoc->longPokemon = msgGet->longPokemon;
	msgLoc->pokemon = malloc(msgGet->longPokemon);
	memcpy(msgLoc->pokemon, msgGet->pokemon, msgGet->longPokemon);
	msgLoc->paresDeCoordenada = list_create();
	if(list_size(posicionesList)>0){
		msgLoc->listSize = list_size(posicionesList);
		list_add_all(msgLoc->paresDeCoordenada, posicionesList);
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

	return mensaje;

}


