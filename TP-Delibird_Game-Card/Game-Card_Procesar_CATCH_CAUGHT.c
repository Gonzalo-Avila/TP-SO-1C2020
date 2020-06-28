#include "Game-Card.h"

void procesarCATCH(mensajeRecibido * mensajeRecibido) {

	log_debug(logger, "[CATCH] Procesando");

	mensajeCatch * msgCatch = desarmarMensajeCATCH(mensajeRecibido);

	char * posicionComoCadena = posicionComoChar(msgCatch->posicionX,msgCatch->posicionY); //"2-3"

	char * rutaPokemon;
	asprintf(&rutaPokemon, "%s%s%s", puntoDeMontaje, "/Files/",msgCatch->pokemon); //puntoDeMontaje/Files/pikachu

	char * rutaMetadataPokemon;
	asprintf(&rutaMetadataPokemon, "%s%s", rutaPokemon, "/metadata.bin"); //puntodeMontaje/Files/pikachu/metadata.bin

	int sizeMensaje = sizeof(uint32_t);

	sem_wait(&semExistenciaPokemon);
	if (existeElArchivo(rutaMetadataPokemon)) {

		if(!existeSemaforo(rutaMetadataPokemon)){
			mutexPokemon * nuevoSemaforo =  crearNuevoSemaforo(rutaMetadataPokemon);
			list_add(semaforosPokemon,nuevoSemaforo);
		}
		sem_post(&semExistenciaPokemon);

		sem_t * mutexMetadata = obtenerMutexPokemon(rutaMetadataPokemon);
		t_config * metadataPokemon = intentarAbrirMetadataPokemon(mutexMetadata, rutaMetadataPokemon);

		char * archivoMappeado = mapearArchivo(rutaMetadataPokemon,metadataPokemon);

		if (string_contains(archivoMappeado, posicionComoCadena)) {

			// TODO
			// Implementar lucidchart



		}else{
			log_info(logger, "No existe %s en esa posicion", msgCatch->pokemon);
			mensajeCaught * msgCaught = armarMensajeCaught(FAIL);
			log_debug(logger, "[NEW] Enviando APPEARED");
			enviarMensajeBroker(CAUGHT, mensajeRecibido->idMensaje,sizeMensaje, msgCaught);

		}

		config_set_value(metadataPokemon, "OPEN", "N");

		sem_wait(mutexMetadata);
		config_save(metadataPokemon);
		sem_post(mutexMetadata);

		config_destroy(metadataPokemon);

		return;
	}else{
		sem_post(&semExistenciaPokemon);
		log_info(logger, "No existe el pokemon %s", msgCatch->pokemon);
		mensajeCaught * msgCaught = armarMensajeCaught(FAIL);
		log_debug(logger, "[NEW] Enviando APPEARED");
		enviarMensajeBroker(CAUGHT, mensajeRecibido->idMensaje,sizeMensaje, msgCaught);
		return;
	}

//	log_debug(logger, "[CATCH] Enviando CAUGHT");
//	enviarMensajeBroker(CAUGHT, mensajeRecibido->idCorrelativo, 3, (void*) "asd");
	log_debug(logger, "[CATCH] CAUGHT enviado");
}

mensajeCatch * desarmarMensajeCATCH(mensajeRecibido * mensajeRecibido) {
	mensajeCatch * mensaje = malloc(sizeof(mensajeCatch));
	int offset = 0;
	char finDeCadena='\0';
	memcpy(&mensaje->longPokemon, mensajeRecibido->mensaje + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	mensaje->pokemon = malloc(mensaje->longPokemon+1);

	memcpy(mensaje->pokemon, mensajeRecibido->mensaje + offset,mensaje->longPokemon);
	offset += mensaje->longPokemon;

	memcpy(mensaje->pokemon+mensaje->longPokemon,&finDeCadena,1);


	memcpy(&mensaje->posicionX, mensajeRecibido->mensaje + offset,sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&mensaje->posicionY, mensajeRecibido->mensaje + offset,sizeof(uint32_t));

	return mensaje;
}

mensajeCaught * armarMensajeCaught(resultado res){
	mensajeCaught * msgCaught = malloc(sizeof(mensajeCaught));
	msgCaught->resultado = res;
	return msgCaught;
}
