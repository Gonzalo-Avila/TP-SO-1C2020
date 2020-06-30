#include "Game-Card.h"

void procesarCATCH(mensajeRecibido * mensajeRecibido) {

	log_debug(logger, "[CATCH] Procesando");

	mensajeCatch * msgCatch = desarmarMensajeCATCH(mensajeRecibido);

	char * posicionComoCadena = posicionComoChar(msgCatch->posicionX,msgCatch->posicionY); //"2-3"

	char * rutaPokemon;
	asprintf(&rutaPokemon, "%s%s%s", puntoDeMontaje, "/Files/",msgCatch->pokemon); //puntoDeMontaje/Files/pikachu

	char * rutaMetadataPokemon;
	asprintf(&rutaMetadataPokemon, "%s%s", rutaPokemon, "/metadata.bin"); //puntodeMontaje/Files/pikachu/metadata.bin

	int sizeMensaje = sizeof(resultado);

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

			if (strcmp(config_get_string_value(metadataPokemon, "OPEN"), "N") == 0) {

				config_set_value(metadataPokemon, "OPEN", "Y");
				config_save(metadataPokemon);
				sem_post(mutexMetadata);

				char * archivoMappeado = mapearArchivo(rutaMetadataPokemon,metadataPokemon);

				if (existenLasCoordenadas(archivoMappeado, posicionComoCadena)) {

					//Armar la cadena habiendo aplicado el catch y obtener su size
					char* aEscribirEnBloques = string_new();
					int indexEntrada = 0;
					char** arrayDeEntradas = string_split(archivoMappeado, "\n");	//["5-5=3", "5-6=16" ,"3-1=201", NULL];
					char* entradaActual = arrayDeEntradas[indexEntrada];			// "5-5=3
					int cantidadNum=0;

					while (entradaActual != NULL) {
						char** posicionCantidad = string_split(entradaActual, "=");	//["5-5", "3", NULL]
						char* posicion = posicionCantidad[0];
						char* cantidad = posicionCantidad[1];
						cantidadNum=atoi(cantidad);
						if (strcmp(posicion, posicionComoCadena) == 0) { 			// "5-5" vs posicionCatch
							cantidadNum = atoi(cantidad);
							cantidadNum = cantidadNum - 1;
							cantidad = string_itoa(cantidadNum);
						}
						if(cantidadNum>0){
						string_append(&aEscribirEnBloques, posicion);
						string_append(&aEscribirEnBloques, "=");
						string_append(&aEscribirEnBloques, cantidad);
						string_append(&aEscribirEnBloques, "\n");
						}
						indexEntrada++;
						entradaActual=arrayDeEntradas[indexEntrada];
						liberarStringSplitteado(posicionCantidad);

					}
					liberarStringSplitteado(arrayDeEntradas);
					int sizeAEscribir = strlen(aEscribirEnBloques);
					//---------------------------------------------------------

					int cantidadDeBloquesAsignadosActualmente = obtenerCantidadDeBloquesAsignados(rutaMetadataPokemon);

					int cantidadDeBloquesSobrantes = cantidadDeBloquesAsignadosActualmente - cantidadDeBloquesNecesariosParaSize(sizeAEscribir);
					desasignarBloquesAArchivo(metadataPokemon,cantidadDeBloquesSobrantes,cantidadDeBloquesAsignadosActualmente);

					cantidadDeBloquesAsignadosActualmente=obtenerCantidadDeBloquesAsignados(rutaMetadataPokemon);
					if(cantidadDeBloquesAsignadosActualmente==0){
						remove(rutaMetadataPokemon);
						rmdir(rutaPokemon);
					}
					else{
						escribirCadenaEnArchivo(rutaMetadataPokemon,aEscribirEnBloques);
						config_set_value(metadataPokemon,"SIZE",string_itoa(sizeAEscribir));
					}

					config_set_value(metadataPokemon, "OPEN", "N");
					sleep(tiempoDeRetardo);

					sem_wait(mutexMetadata);
					config_save(metadataPokemon);
					sem_post(mutexMetadata);

					mensajeCaught * msgCaught = armarMensajeCaught(OK);
					enviarMensajeBroker(CAUGHT, mensajeRecibido->idMensaje,sizeMensaje, msgCaught);
				}
				else{
					config_set_value(metadataPokemon, "OPEN", "N");
					sleep(tiempoDeRetardo);

					sem_wait(mutexMetadata);
					config_save(metadataPokemon);
					sem_post(mutexMetadata);

					log_info(logger, "No existe %s en esa posicion", msgCatch->pokemon);
					mensajeCaught * msgCaught = armarMensajeCaught(FAIL);
					log_debug(logger, "[NEW] Enviando APPEARED");
					enviarMensajeBroker(CAUGHT, mensajeRecibido->idMensaje,sizeMensaje, msgCaught);
				}
				operacionFinalizada=true;
			}
			else{
				sem_post(mutexMetadata);
				sleep(tiempoDeReintentoDeAcceso);
			}
			config_destroy(metadataPokemon);
		}
		else{
			sem_post(mutexMetadata);
			log_info(logger, "No existe el pokemon %s", msgCatch->pokemon);
			mensajeCaught * msgCaught = armarMensajeCaught(FAIL);
			log_debug(logger, "[NEW] Enviando APPEARED");
			enviarMensajeBroker(CAUGHT, mensajeRecibido->idMensaje,sizeMensaje, msgCaught);
			operacionFinalizada=true;
		}
	}
	free(msgCatch->pokemon);
	free(msgCatch);
	free(posicionComoCadena);
	free(rutaPokemon);
	free(rutaMetadataPokemon);

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
