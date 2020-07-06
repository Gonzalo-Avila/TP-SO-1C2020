#include "Game-Card.h"



void procesarNEW(mensajeRecibido * mensajeRecibido) {

	log_debug(logger, "[NEW] Procesando");

	mensajeNew * msgNew = desarmarMensajeNEW(mensajeRecibido);

	char * posicionComoCadena = posicionComoChar(msgNew->posicionX,msgNew->posicionY); //"2-3"

	char * entradaCompletaComoCadena;
	asprintf(&entradaCompletaComoCadena, "%s%s%d%s", posicionComoCadena, "=",msgNew->cantPokemon, "\n"); //"2-3 = 5"

	char * rutaPokemon;
	asprintf(&rutaPokemon, "%s%s%s", puntoDeMontaje, "/Files/",msgNew->pokemon); //puntoDeMontaje/Files/pikachu

	char * rutaMetadataPokemon;
	asprintf(&rutaMetadataPokemon, "%s%s", rutaPokemon, "/metadata.bin"); //puntodeMontaje/Files/pikachu/metadata.bin

	int tamanioEntradaCompleta = strlen(entradaCompletaComoCadena);


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

		sem_wait(mutexMetadata);

		if (existeElArchivo(rutaMetadataPokemon)){
			metadataPokemon = config_create(rutaMetadataPokemon);

			if (strcmp(config_get_string_value(metadataPokemon, "OPEN"), "N") == 0) {

				config_set_value(metadataPokemon, "OPEN", "Y");
				config_save(metadataPokemon);

				sem_post(mutexMetadata);

				char * archivoMappeado = mapearArchivo(rutaMetadataPokemon,metadataPokemon);

				if (existenLasCoordenadas(archivoMappeado, posicionComoCadena)) {

						char* aEscribirEnBloques = string_new();
						int indexEntrada = 0;
						char** arrayDeEntradas = string_split(archivoMappeado, "\n");	//["5-5=3", "5-6=16" ,"3-1=201", NULL];
						char* entradaActual = arrayDeEntradas[indexEntrada];			// "5-5=3"

						while (entradaActual != NULL) {
							char** posicionCantidad = string_split(entradaActual, "=");	//["5-5", "3", NULL]
							char* posicion = posicionCantidad[0];
							char* cantidad = posicionCantidad[1];
							if (strcmp(posicion, posicionComoCadena) == 0) { 			// "5-5" vs posicionNew
								int cantidadNum = atoi(cantidad);
								cantidadNum = cantidadNum + msgNew->cantPokemon;
								cantidad = string_itoa(cantidadNum);
							}
							string_append(&aEscribirEnBloques, posicion);
							string_append(&aEscribirEnBloques, "=");
							string_append(&aEscribirEnBloques, cantidad);
							string_append(&aEscribirEnBloques, "\n");
							indexEntrada++;
							entradaActual=arrayDeEntradas[indexEntrada];
							liberarStringSplitteado(posicionCantidad);
							free(cantidad);
						}
						liberarStringSplitteado(arrayDeEntradas);

						int sizeAEscribir = strlen(aEscribirEnBloques);
						int cantidadDeBloquesAsignadosAArchivo = obtenerCantidadDeBloquesAsignados(rutaMetadataPokemon);
						if (cantidadDeBloquesAsignadosAArchivo * tamanioBloque>= sizeAEscribir) {
							escribirCadenaEnArchivo(rutaMetadataPokemon,aEscribirEnBloques);
							sleep(tiempoDeRetardo);

							char * sizeFinal = string_itoa(sizeAEscribir);
							config_set_value(metadataPokemon, "SIZE", sizeFinal);
							config_set_value(metadataPokemon, "OPEN", "N");

							sem_wait(mutexMetadata);
							config_save(metadataPokemon);
							sem_post(mutexMetadata);

							mensajeAppeared * msgAppeared = armarMensajeAppeared(msgNew);
							uint32_t sizeMensaje = msgAppeared->longPokemon + sizeof(uint32_t) * 3;
							log_debug(logger, "[NEW] Enviando APPEARED");
							enviarMensajeBroker(APPEARED, mensajeRecibido->idMensaje, sizeMensaje, msgAppeared);

							free(sizeFinal);
							free(msgAppeared->pokemon);
							free(msgAppeared);
						}
						else{
							int cantidadDisponible = cantidadDeBloquesAsignadosAArchivo * tamanioBloque + espacioLibreEnElFS();
							if(cantidadDisponible >= sizeAEscribir){

							int bloquesNecesarios = cantidadDeBloquesNecesariosParaSize(sizeAEscribir);
							sem_wait(mutexMetadata);
							asignarBloquesAArchivo(rutaMetadataPokemon, bloquesNecesarios-cantidadDeBloquesAsignadosAArchivo, metadataPokemon);
							sem_post(mutexMetadata);

							escribirCadenaEnArchivo(rutaMetadataPokemon, aEscribirEnBloques);
							sleep(tiempoDeRetardo);

							char * sizeFinal = string_itoa(sizeAEscribir);
							config_set_value(metadataPokemon, "SIZE", sizeFinal);
							config_set_value(metadataPokemon, "OPEN", "N");

							sem_wait(mutexMetadata);
							config_save(metadataPokemon);
							sem_post(mutexMetadata);

							mensajeAppeared * msgAppeared = armarMensajeAppeared(msgNew);
							uint32_t sizeMensaje = msgAppeared->longPokemon + sizeof(uint32_t) * 3;
							log_debug(logger, "[NEW] Enviando APPEARED");
							enviarMensajeBroker(APPEARED, mensajeRecibido->idMensaje, sizeMensaje, msgAppeared);
							free(sizeFinal);
							free(msgAppeared->pokemon);
							free(msgAppeared);
							}
							else{
								log_info(logger, "No se pudo actualizar la cantidad en las posiciones dadas, no hay espacio suficiente");
								return;
							}
						}
					free(aEscribirEnBloques);
					} else {

						char * rutaUltimoBloque = obtenerRutaUltimoBloque(rutaMetadataPokemon);
								int espacioLibreUltimoBloque = obtenerEspacioLibreDeBloque(rutaUltimoBloque);

								if (espacioLibreUltimoBloque >= tamanioEntradaCompleta) {
									escribirCadenaEnBloque(rutaUltimoBloque,entradaCompletaComoCadena);
									sleep(tiempoDeRetardo);

									char * sizeFinal = string_itoa(tamanioEntradaCompleta + config_get_int_value(metadataPokemon,"SIZE"));
									config_set_value(metadataPokemon, "SIZE", sizeFinal);
									config_set_value(metadataPokemon, "OPEN", "N");

									sem_wait(mutexMetadata);
									config_save(metadataPokemon);
									sem_post(mutexMetadata);

									mensajeAppeared * msgAppeared = armarMensajeAppeared(msgNew);
									uint32_t sizeMensaje = msgAppeared->longPokemon + sizeof(uint32_t) * 3;
									log_debug(logger, "[NEW] Enviando APPEARED");
									enviarMensajeBroker(APPEARED, mensajeRecibido->idMensaje, sizeMensaje, msgAppeared);

									free(sizeFinal);
									free(msgAppeared->pokemon);
									free(msgAppeared);
								} else {

									if (espacioLibreUltimoBloque + espacioLibreEnElFS()>= tamanioEntradaCompleta) {

										int bloquesNecesarios = cantidadDeBloquesNecesariosParaSize(tamanioEntradaCompleta - espacioLibreUltimoBloque);

										sem_wait(mutexMetadata);
										asignarBloquesAArchivo(rutaMetadataPokemon,bloquesNecesarios, metadataPokemon);
										sem_post(mutexMetadata);

										char * contenidoAAlmacenar;
										asprintf(&contenidoAAlmacenar, "%s%s", archivoMappeado,entradaCompletaComoCadena);
										escribirCadenaEnArchivo(rutaMetadataPokemon,contenidoAAlmacenar);
										sleep(tiempoDeRetardo);

										char * sizeFinal = string_itoa(tamanioEntradaCompleta + config_get_int_value(metadataPokemon,"SIZE"));
										config_set_value(metadataPokemon, "SIZE", sizeFinal);
										config_set_value(metadataPokemon, "OPEN", "N");

										sem_wait(mutexMetadata);
										config_save(metadataPokemon);
										sem_post(mutexMetadata);

										mensajeAppeared * msgAppeared = armarMensajeAppeared(msgNew);
										uint32_t sizeMensaje = msgAppeared->longPokemon+ sizeof(uint32_t) * 3;
										log_debug(logger, "[NEW] Enviando APPEARED");
										enviarMensajeBroker(APPEARED, mensajeRecibido->idMensaje,sizeMensaje, msgAppeared);

										free(sizeFinal);
										free(contenidoAAlmacenar);
										free(msgAppeared->pokemon);
										free(msgAppeared);

									}
									else{
										log_info(logger,"No se pueden crear las nuevas posiciones, no hay espacio suficiente");
										return;
									}
								}
								free(rutaUltimoBloque);
							}
						free(archivoMappeado);
						operacionFinalizada=true;
				}
				else{
					sem_post(mutexMetadata);
					sleep(tiempoDeReintentoDeAcceso);
				}
				config_destroy(metadataPokemon);
		}
		else {
			if (haySuficientesBloquesLibresParaSize(tamanioEntradaCompleta)) {

				mkdir(rutaPokemon, 0777);

				crearNuevoPokemon(rutaMetadataPokemon, msgNew);

				int cantidadDeBloquesAAsignar = cantidadDeBloquesNecesariosParaSize(tamanioEntradaCompleta);

				metadataPokemon = config_create(rutaMetadataPokemon);

				asignarBloquesAArchivo(rutaMetadataPokemon, cantidadDeBloquesAAsignar, metadataPokemon);

				escribirCadenaEnArchivo(rutaMetadataPokemon,entradaCompletaComoCadena);

				sleep(tiempoDeRetardo);

				char * sizeFinal = string_itoa(tamanioEntradaCompleta);
				config_set_value(metadataPokemon, "SIZE", sizeFinal);
				config_set_value(metadataPokemon, "OPEN", "N");
				config_save(metadataPokemon);

				sem_post(mutexMetadata);

				config_destroy(metadataPokemon);

				mensajeAppeared * msgAppeared = armarMensajeAppeared(msgNew);
				uint32_t sizeMensaje = msgAppeared->longPokemon+ sizeof(uint32_t) * 3;
				log_debug(logger, "[NEW] Enviando APPEARED");
				enviarMensajeBroker(APPEARED, mensajeRecibido->idMensaje,sizeMensaje, msgAppeared);

				free(sizeFinal);
				free(msgAppeared->pokemon);
				free(msgAppeared);
			}
			else {
				sem_post(mutexMetadata);
				log_info(logger,"No se puede crear el nuevo pokemon, no hay espacio suficiente");
			}
			operacionFinalizada=true;
		}
	}

	free(rutaPokemon);
	free(rutaMetadataPokemon);
	free(posicionComoCadena);
	free(entradaCompletaComoCadena);
	free(msgNew->pokemon);
	free(msgNew);
	free(mensajeRecibido->mensaje);
	free(mensajeRecibido);
}


mensajeNew * desarmarMensajeNEW(mensajeRecibido * mensajeRecibido) {
	mensajeNew * mensaje = malloc(sizeof(mensajeNew));
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
	offset += sizeof(uint32_t);

	memcpy(&mensaje->cantPokemon, mensajeRecibido->mensaje + offset,sizeof(uint32_t));

	return mensaje;
}


void crearNuevoPokemon(char * rutaMetadataPokemon, mensajeNew * datosDelPokemon) {
	int pokemonFD = open(rutaMetadataPokemon, O_RDWR | O_CREAT, 0777);
	close(pokemonFD);
	inicializarArchivoMetadata(rutaMetadataPokemon);
}

mensajeAppeared * armarMensajeAppeared(mensajeNew * msgNew) {
	mensajeAppeared * msgAppeared = malloc(sizeof(mensajeAppeared));
	msgAppeared->longPokemon = msgNew->longPokemon;
	msgAppeared->pokemon = malloc(msgAppeared->longPokemon);
	memcpy(msgAppeared->pokemon, msgNew->pokemon, msgAppeared->longPokemon);
	msgAppeared->posicionX = msgNew->posicionX;
	msgAppeared->posicionY = msgNew->posicionY;
	return msgAppeared;
}

