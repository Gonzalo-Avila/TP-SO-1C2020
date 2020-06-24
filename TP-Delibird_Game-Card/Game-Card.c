#include "Game-Card.h"

void inicializarVariablesGlobales() {
	config = config_create("gamecard.config");
	logger = log_create("gamecard_logs", "GameCard", 1, LOG_LEVEL_TRACE);

	ipServidor = malloc(
			strlen(config_get_string_value(config, "IP_BROKER")) + 1);
	strcpy(ipServidor, config_get_string_value(config, "IP_BROKER"));
	puertoServidor = malloc(
			strlen(config_get_string_value(config, "PUERTO_BROKER")) + 1);
	strcpy(puertoServidor, config_get_string_value(config, "PUERTO_BROKER"));

	puntoDeMontaje = malloc(
			strlen(config_get_string_value(config, "PUNTO_MONTAJE_TALLGRASS"))
					+ 1);
	strcpy(puntoDeMontaje,
			config_get_string_value(config, "PUNTO_MONTAJE_TALLGRASS"));

	tiempoDeRetardo = config_get_int_value(config, "TIEMPO_RETARDO_OPERACION");
	tiempoDeReintentoDeAcceso = config_get_int_value(config,
			"TIEMPO_DE_REINTENTO_OPERACION");

	idProceso = -1;
	statusConexionBroker = 0;
}

void destruirVariablesGlobales() {
	free(ipServidor);
	free(puertoServidor);
	log_destroy(logger);
	config_destroy(config);
}

mensajeNew * desarmarMensajeNEW(mensajeRecibido * mensajeRecibido) {
	mensajeNew * mensaje = malloc(sizeof(mensajeNew));
	int offset = 0;
	memcpy(&mensaje->longPokemon, mensajeRecibido->mensaje + offset,
			sizeof(uint32_t));
	offset += sizeof(uint32_t);

	mensaje->pokemon = malloc(mensaje->longPokemon);

	memcpy(mensaje->pokemon, mensajeRecibido->mensaje + offset,
			mensaje->longPokemon);
	offset += mensaje->longPokemon;

	memcpy(&mensaje->posicionX, mensajeRecibido->mensaje + offset,
			sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&mensaje->posicionY, mensajeRecibido->mensaje + offset,
			sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&mensaje->cantPokemon, mensajeRecibido->mensaje + offset,
			sizeof(uint32_t));

	return mensaje;
}

mensajeCatch * desarmarMensajeCATCH(mensajeRecibido * mensajeRecibido) {
	mensajeCatch * mensaje = malloc(sizeof(mensajeCatch));
	int offset = 0;
	memcpy(&mensaje->longPokemon, mensajeRecibido->mensaje + offset,
			sizeof(uint32_t));
	offset += sizeof(uint32_t);

	mensaje->pokemon = malloc(mensaje->longPokemon);

	memcpy(mensaje->pokemon, mensajeRecibido->mensaje + offset,
			mensaje->longPokemon);
	offset += mensaje->longPokemon;

	memcpy(&mensaje->posicionX, mensajeRecibido->mensaje + offset,
			sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&mensaje->posicionY, mensajeRecibido->mensaje + offset,
			sizeof(uint32_t));

	return mensaje;
}

mensajeGet * desarmarMensajeGET(mensajeRecibido * mensajeRecibido) {
	mensajeGet * mensaje = malloc(sizeof(mensajeGet));
	int offset = 0;
	memcpy(&mensaje->longPokemon, mensajeRecibido->mensaje + offset,
			sizeof(uint32_t));
	offset += sizeof(uint32_t);

	mensaje->pokemon = malloc(mensaje->longPokemon);

	memcpy(mensaje->pokemon, mensajeRecibido->mensaje + offset,
			mensaje->longPokemon);

	return mensaje;
}

char * agregarFinDeCadena(char * cadena) {
	char * cad = string_new();
	string_append(&cad, cadena);
	string_append(&cad, "\0");
	return cad;
}

char * posicionComoChar(uint32_t posx, uint32_t posy) {
	char * cad;
	asprintf(&cad, "%d-%d", posx, posy);
	return cad;
}

void inicializarArchivoMetadata(char * rutaArchivo) {
	t_config * metadata;
	metadata = config_create(rutaArchivo);
	config_set_value(metadata, "OPEN", "Y");
	config_set_value(metadata, "BLOCKS", "[]");
	config_set_value(metadata, "SIZE", "0");
	config_set_value(metadata, "DIRECTORY", "N");
	config_save(metadata);
	config_destroy(metadata);
}

char * aniadirBloqueAVectorString(int numeroBloque, char ** bloquesActuales) {

	char * cadenaAGuardar = string_new();
	int i = 0;
	string_append(&cadenaAGuardar, "[");
	while (bloquesActuales[i] != NULL) {
		string_append(&cadenaAGuardar, bloquesActuales[i]);
		string_append(&cadenaAGuardar, ",");
	}
	string_append(&cadenaAGuardar, string_itoa(numeroBloque));
	string_append(&cadenaAGuardar, "]");
	return cadenaAGuardar;
}

/*
 int asignarBloqueAArchivo(char * rutaMetadataArchivo){
 t_config * metadataArchivo;
 metadataArchivo = config_create(rutaMetadataArchivo);
 int indexBloqueLibre = buscarBloqueLibre();
 char ** bloquesActuales = config_get_array_value(metadataArchivo,"BLOCKS");
 char * cadenaAGuardar = aniadirBloqueAVectorString(indexBloqueLibre,bloquesActuales);
 bitarray_set_bit(bitarrayBloques,indexBloqueLibre);
 //msync();
 config_set_value(metadataArchivo,"BLOCKS",cadenaAGuardar);
 config_save(metadataArchivo);
 config_destroy(metadataArchivo);
 free(cadenaAGuardar);
 return indexBloqueLibre;
 }*/

void asignarBloquesAArchivo(char * rutaMetadataArchivo, int cantidadDeBloques) {
	t_config * metadataArchivo;
	metadataArchivo = config_create(rutaMetadataArchivo);

	for (int i = 0; i < cantidadDeBloques; i++) {

		int indexBloqueLibre = buscarBloqueLibre();
		char ** bloquesActuales = config_get_array_value(metadataArchivo,
				"BLOCKS");
		char * cadenaAGuardar = aniadirBloqueAVectorString(indexBloqueLibre,
				bloquesActuales);
		bitarray_set_bit(bitarrayBloques, indexBloqueLibre);
		//msync();
		config_set_value(metadataArchivo, "BLOCKS", cadenaAGuardar);
		config_save(metadataArchivo);

		free(cadenaAGuardar);
	}
	config_destroy(metadataArchivo);
}

int buscarBloqueLibre() {
	for (int i = 0; i < cantidadDeBloques; i++) {
		int estadoBloqueActual = bitarray_test_bit(bitarrayBloques, i);
		if (estadoBloqueActual == 0) {
			return i;
		}
	}
	return -1;
}

void crearNuevoPokemon(char * rutaMetadataPokemon, mensajeNew * datosDelPokemon) {
	int pokemonFD = open(rutaMetadataPokemon, O_RDWR | O_CREAT, 0777);
	close(pokemonFD);
	inicializarArchivoMetadata(rutaMetadataPokemon);
}

/*
 void agregarPokemonsAPosicion(char* rutaMetadataPokemon, char* posicionComoCadena, mensajeNew* mensaje){

 t_config* archivo;
 archivo = config_create(rutaMetadataPokemon);
 config_set_value(archivo, "OPEN", "Y");
 config_save(archivo);
 char** bloques = config_get_array_value(archivo, "BLOCKS");
 int i = 0;
 bool lasPosicionesYaExistian = false;
 while (bloques[i] != NULL) {
 t_config* bloque;
 char* rutaBloque;
 asprintf(&rutaBloque, "%s%s%s%s", puntoDeMontaje, "/Blocks/",
 bloques[i], ".bin");
 bloque = config_create(rutaBloque);
 if (config_has_property(bloque, posicionComoCadena)) {
 int cantidadActual = config_get_int_value(bloque,
 posicionComoCadena);
 cantidadActual += mensaje->cantPokemon;
 char* cantidadComoString;
 asprintf(&cantidadComoString, "%d", cantidadActual);
 config_set_value(bloque, posicionComoCadena, cantidadComoString);
 config_save(archivo);
 lasPosicionesYaExistian = true;
 free(cantidadComoString);
 config_destroy(bloque);
 break;
 }
 config_destroy(bloque);
 i++;
 }
 if (!lasPosicionesYaExistian) {
 char * posicionesConCantidad;
 asprintf(&posicionesConCantidad,"%s%s%d",posicionComoCadena,"=",mensaje->cantPokemon);
 int sizeCadena = strlen(posicionesConCantidad);
 }
 }*/

int obtenerCantidadDeBloquesAsignados(char* rutaMetadata) {
	t_config * metadata;
	metadata = config_create(rutaMetadata);
	char ** bloquesArchivo = config_get_array_value(metadata, "BLOCKS");
	int index = 0;
	while (bloquesArchivo[index] != NULL) {
		index++;
	}
	return index;
}

int obtenerCantidadDeBloquesLibres() {
	int cantidadDeBloquesLibres = 0;
	for (int i = 0; i < cantidadDeBloques; i++) {
		if (!bitarray_test_bit(bitarrayBloques, i)) {
			cantidadDeBloquesLibres++;
		}
	}
	return cantidadDeBloquesLibres;
}

bool haySuficientesBloquesLibresParaSize(int size) {
	int cantidadDeBloquesLibres = 0;
	cantidadDeBloquesLibres = obtenerCantidadDeBloquesLibres(
			cantidadDeBloquesLibres);
	return cantidadDeBloquesLibres * tamanioBloque >= size;
}

int cantidadDeBloquesNecesariosParaSize(int size) {
	int cantidad = 0;
	while (cantidad * tamanioBloque < size) {
		cantidad++;
	}
	return cantidad;
}

void escribirCadenaEnArchivo(char * rutaMetadataArchivo, char * cadena) {

	t_config * metadata;
	metadata = config_create(rutaMetadataArchivo);
	char ** bloquesArchivo = config_get_array_value(metadata, "BLOCKS");
	char * rutaBloqueActual;
	int numeroDeBloqueUsado = 0;//No es el numero de bloque, sino la posicion en el metadata. So fucking hard to explain
	asprintf(&rutaBloqueActual, "%s%s%s%s", puntoDeMontaje, "/Blocks/",
			bloquesArchivo[numeroDeBloqueUsado], ".bin");
	int caracteresEscritosEnBloqueActual = 0;
	int caracteresEscritosTotales = 0;
	char caracterActual = cadena[caracteresEscritosTotales];

	FILE * bloqueActual = fopen(rutaBloqueActual, "w");

	while (caracterActual != '\0') {
		if (caracteresEscritosEnBloqueActual == tamanioBloque) {
			fclose(bloqueActual);
			free(rutaBloqueActual);
			numeroDeBloqueUsado++;
			asprintf(&rutaBloqueActual, "%s%s%s%s", puntoDeMontaje, "/Blocks/",
					bloquesArchivo[numeroDeBloqueUsado], ".bin");
			bloqueActual = fopen(rutaBloqueActual, "w");
			caracteresEscritosEnBloqueActual = 0;
		}
		fputc(caracterActual, bloqueActual);
		caracteresEscritosTotales++;
		caracteresEscritosEnBloqueActual++;
		caracterActual = cadena[caracteresEscritosTotales];
	}

	fclose(bloqueActual);
	config_destroy(metadata);
	free(rutaBloqueActual);
}

void escribirCadenaEnBloque(char * rutaBloque, char * cadena) {
	FILE * bloque = fopen(rutaBloque, "a");
	fputs(cadena, bloque);
	fclose(bloque);
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

char * mapearArchivo(char * rutaMetadata) {
	t_config * metadata;
	FILE * bloqueActual;
	int sizeArchivo, numeroBloqueActual = 0, caracteresLeidosEnBloqueActual = 0;
	char ** bloquesArchivo;
	char * archivoMapeado;
	char * rutaBloqueActual;

	metadata = config_create(rutaMetadata);
	sizeArchivo = config_get_int_value(metadata, "SIZE");
	bloquesArchivo = config_get_array_value(metadata, "BLOCKS");
	archivoMapeado = malloc(sizeArchivo);
	asprintf(&rutaBloqueActual, "%s%s%s%s", puntoDeMontaje, "/BLOCKS/",
			bloquesArchivo[numeroBloqueActual], ".bin");
	bloqueActual = fopen(rutaBloqueActual, "r");

	for (int i = 0; i < sizeArchivo; i++) {
		if (caracteresLeidosEnBloqueActual == tamanioBloque) {
			fclose(bloqueActual);
			free(rutaBloqueActual);
			numeroBloqueActual++;
			asprintf(&rutaBloqueActual, "%s%s%s%s", puntoDeMontaje, "/Blocks/",
					bloquesArchivo[numeroBloqueActual], ".bin");
			bloqueActual = fopen(rutaBloqueActual, "r");
			caracteresLeidosEnBloqueActual = 0;
		}
		archivoMapeado[i] = fgetc(bloqueActual);
		caracteresLeidosEnBloqueActual++;
	}

	fclose(bloqueActual);
	free(rutaBloqueActual);
	config_destroy(metadata);

	return archivoMapeado;
}
char * obtenerRutaUltimoBloque(char * metadataArchivo) {
	t_config * metadata = config_create(metadataArchivo);
	char ** bloques = config_get_array_value(metadata, "BLOCKS");
	char * rutaUltimoBloque;
	int index = 0;

	while (bloques[index] != NULL) {
		index++;
	}
	if (index == 0)
		return NULL;

	asprintf(&rutaUltimoBloque, "%s%s%s%s", puntoDeMontaje, "/Blocks/",
			bloques[index - 1], ".bin");
	config_destroy(metadata);
	return rutaUltimoBloque;
}

int obtenerSizeOcupadoDeBloque(char * rutaBloque) {
	struct stat stat_file;
	stat(rutaBloque, &stat_file);
	return stat_file.st_size;
}

int obtenerEspacioLibreDeBloque(char * rutaBloque) {
	return tamanioBloque - obtenerSizeOcupadoDeBloque(rutaBloque);
}

int espacioLibreEnElFS() {
	return obtenerCantidadDeBloquesLibres() * tamanioBloque;
}
void procesarNEW(mensajeRecibido * mensajeRecibido) {

	log_debug(logger, "[NEW] Procesando");

	mensajeNew * msgNew = desarmarMensajeNEW(mensajeRecibido);

	char * pokemonConFinDeCadena = agregarFinDeCadena(msgNew->pokemon);
	char * posicionComoCadena = posicionComoChar(msgNew->posicionX,
			msgNew->posicionY);
	char * entradaCompletaComoCadena;
	asprintf(&entradaCompletaComoCadena, "%s%s%d%s", posicionComoCadena, "=",
			msgNew->cantPokemon, "\n");
	char * rutaPokemon;
	asprintf(&rutaPokemon, "%s%s%s", puntoDeMontaje, "/Files/",
			pokemonConFinDeCadena);
	char * rutaMetadataPokemon;
	asprintf(&rutaMetadataPokemon, "%s%s", rutaPokemon, "/metadata.bin");

	int tamanioEntradaCompleta = strlen(entradaCompletaComoCadena);

	if (existeElArchivo(rutaMetadataPokemon)) {

		//TODO - Lado derecho del if
		char * archivoMappeado = mapearArchivo(rutaMetadataPokemon);

		if (string_contains(archivoMappeado, posicionComoCadena)) {
			/*	TODO
			 *	Implementar "ya esta la posicion"
			 */

			char* aEscribirEnBloques = string_new();
			int indexEntrada = 0;
			char** arrayDeEntradas = string_split(archivoMappeado, "\n");	//["5-5=3", "5-6=16" ,"3-1=201", NULL];
			char* entradaActual = arrayDeEntradas[indexEntrada];			// "5-5=3"

			while (entradaActual != NULL) {
				char** posicionCantidad = string_split(entradaActual, "=");	//["5-5", "3", NULL]
				char* posicion = entradaActual[0];
				char* cantidad = entradaActual[1];
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
			}

			int sizeAEscribir = strlen(aEscribirEnBloques);
			int cantidadDeBloquesAsignadosAArchivo =
					obtenerCantidadDeBloquesAsignados(rutaMetadataPokemon);
			if (cantidadDeBloquesAsignadosAArchivo * tamanioBloque>= sizeAEscribir) {
				escribirCadenaEnArchivo(rutaMetadataPokemon,
						aEscribirEnBloques);
				sleep(tiempoDeRetardo);
				//TODO - Implementar semaforo para impedir que dos procesos accedan al metadata al mismo tiempo
				t_config * metadataPokemon;
				metadataPokemon = config_create(rutaMetadataPokemon);
				char * sizeFinal = string_itoa(sizeAEscribir);
				config_set_value(metadataPokemon, "SIZE", sizeFinal);
				config_set_value(metadataPokemon, "OPEN", "N");
				config_save(metadataPokemon);
				config_destroy(metadataPokemon);
				//--------------------------------------------------------
				mensajeAppeared * msgAppeared = armarMensajeAppeared(msgNew);
				uint32_t sizeMensaje = msgAppeared->longPokemon + sizeof(uint32_t) * 3;
				log_debug(logger, "[NEW] Enviando APPEARED");
				enviarMensajeBroker(APPEARED, mensajeRecibido->idMensaje, sizeMensaje, msgAppeared);

				free(sizeFinal);
			} else {
				int cantidadDisponible = cantidadDeBloquesAsignadosAArchivo * tamanioBloque + espacioLibreEnElFS();
				if(cantidadDisponible >= sizeAEscribir){
					int bloquesNecesarios = cantidadDeBloquesNecesariosParaSize(sizeAEscribir);
					asignarBloquesAArchivo(rutaMetadataPokemon, bloquesNecesarios-cantidadDeBloquesAsignadosAArchivo);
					escribirCadenaEnArchivo(rutaMetadataPokemon, aEscribirEnBloques);
					sleep(tiempoDeRetardo);
					//TODO - Implementar semaforo para impedir que dos procesos accedan al metadata al mismo tiempo
					t_config * metadataPokemon;
					metadataPokemon = config_create(rutaMetadataPokemon);
					char * sizeFinal = string_itoa(sizeAEscribir);
					config_set_value(metadataPokemon, "SIZE", sizeFinal);
					config_set_value(metadataPokemon, "OPEN", "N");
					config_save(metadataPokemon);
					config_destroy(metadataPokemon);
					//--------------------------------------------------------

					mensajeAppeared * msgAppeared = armarMensajeAppeared(msgNew);
					uint32_t sizeMensaje = msgAppeared->longPokemon + sizeof(uint32_t) * 3;
					log_debug(logger, "[NEW] Enviando APPEARED");
					enviarMensajeBroker(APPEARED, mensajeRecibido->idMensaje, sizeMensaje, msgAppeared);

					free(sizeFinal);
					free(aEscribirEnBloques);
				}else{
					log_info(logger, "No se pudo actualizar la cantidad en las posiciones dadas, no hay espacio suficiente");
					return;
				}
			}
			free(aEscribirEnBloques);
		} else {

			char * rutaUltimoBloque = obtenerRutaUltimoBloque(
					rutaMetadataPokemon);
			int espacioLibreUltimoBloque = obtenerEspacioLibreDeBloque(
					rutaUltimoBloque);

			if (espacioLibreUltimoBloque >= tamanioEntradaCompleta) {
				escribirCadenaEnBloque(rutaUltimoBloque,
						entradaCompletaComoCadena);
				sleep(tiempoDeRetardo);
				//TODO - Implementar semaforo para impedir que dos procesos accedan al metadata al mismo tiempo
				t_config * metadataPokemon;
				metadataPokemon = config_create(rutaMetadataPokemon);
				char * sizeFinal = string_itoa(
						tamanioEntradaCompleta
								+ config_get_int_value(metadataPokemon,
										"SIZE"));
				config_set_value(metadataPokemon, "SIZE", sizeFinal);
				config_set_value(metadataPokemon, "OPEN", "N");
				config_save(metadataPokemon);
				config_destroy(metadataPokemon);
				//--------------------------------------------------------
				mensajeAppeared * msgAppeared = armarMensajeAppeared(msgNew);
				uint32_t sizeMensaje = msgAppeared->longPokemon
						+ sizeof(uint32_t) * 3;
				log_debug(logger, "[NEW] Enviando APPEARED");
				enviarMensajeBroker(APPEARED, mensajeRecibido->idMensaje,
						sizeMensaje, msgAppeared);

				free(sizeFinal);
			} else {

				if (espacioLibreUltimoBloque + espacioLibreEnElFS()
						>= tamanioEntradaCompleta) {

					int bloquesNecesarios = cantidadDeBloquesNecesariosParaSize(
							tamanioEntradaCompleta - espacioLibreUltimoBloque);
					asignarBloquesAArchivo(rutaMetadataPokemon,
							bloquesNecesarios);
					char * contenidoAAlmacenar;
					asprintf(&contenidoAAlmacenar, "%s%s", archivoMappeado,
							entradaCompletaComoCadena);
					escribirCadenaEnArchivo(rutaMetadataPokemon,
							contenidoAAlmacenar);
					sleep(tiempoDeRetardo);
					//TODO - Implementar semaforo para impedir que dos procesos accedan al metadata al mismo tiempo
					t_config * metadataPokemon;
					metadataPokemon = config_create(rutaMetadataPokemon);
					char * sizeFinal = string_itoa(
							tamanioEntradaCompleta
									+ config_get_int_value(metadataPokemon,
											"SIZE"));
					config_set_value(metadataPokemon, "SIZE", sizeFinal);
					config_set_value(metadataPokemon, "OPEN", "N");
					config_save(metadataPokemon);
					config_destroy(metadataPokemon);
					//--------------------------------------------------------
					mensajeAppeared * msgAppeared = armarMensajeAppeared(
							msgNew);
					uint32_t sizeMensaje = msgAppeared->longPokemon
							+ sizeof(uint32_t) * 3;
					log_debug(logger, "[NEW] Enviando APPEARED");
					enviarMensajeBroker(APPEARED, mensajeRecibido->idMensaje,
							sizeMensaje, msgAppeared);

					free(sizeFinal);
					free(contenidoAAlmacenar);

				} else {
					log_info(logger,
							"No se pueden crear las nuevas posiciones, no hay espacio suficiente");
					return;
				}

			}
			free(rutaUltimoBloque);
		}
		free(archivoMappeado);
	} else {

		if (haySuficientesBloquesLibresParaSize(tamanioEntradaCompleta)) {
			mkdir(rutaPokemon, 0777);
			crearNuevoPokemon(rutaMetadataPokemon, msgNew);
			int cantidadDeBloquesAAsignar = cantidadDeBloquesNecesariosParaSize(
					tamanioEntradaCompleta);
			asignarBloquesAArchivo(rutaMetadataPokemon,
					cantidadDeBloquesAAsignar);
			escribirCadenaEnArchivo(rutaMetadataPokemon,
					entradaCompletaComoCadena);
			sleep(tiempoDeRetardo);
			//TODO - Implementar semaforo para impedir que dos procesos accedan al metadata al mismo tiempo
			t_config * metadataPokemon;
			metadataPokemon = config_create(rutaMetadataPokemon);
			char * sizeFinal = string_itoa(tamanioEntradaCompleta);
			config_set_value(metadataPokemon, "SIZE", sizeFinal);
			config_set_value(metadataPokemon, "OPEN", "N");
			config_save(metadataPokemon);
			config_destroy(metadataPokemon);
			//--------------------------------------------------------
			mensajeAppeared * msgAppeared = armarMensajeAppeared(msgNew);
			uint32_t sizeMensaje = msgAppeared->longPokemon
					+ sizeof(uint32_t) * 3;
			log_debug(logger, "[NEW] Enviando APPEARED");
			enviarMensajeBroker(APPEARED, mensajeRecibido->idMensaje,
					sizeMensaje, msgAppeared);

			free(sizeFinal);
		} else {
			log_info(logger,
					"No se puede crear el nuevo pokemon, no hay espacio suficiente");
			return;
		}
	}

	log_debug(logger, "[NEW] APPEARED enviado");

	free(pokemonConFinDeCadena);
	free(rutaPokemon);
	free(rutaMetadataPokemon);
	free(posicionComoCadena);
	free(entradaCompletaComoCadena);

	free(mensajeRecibido->mensaje);
	free(mensajeRecibido);
	free(msgNew->pokemon);
	free(msgNew);
}

void procesarCATCH(mensajeRecibido * mensajeRecibido) {

	log_debug(logger, "[CATCH] Procesando");
	/*
	 * TODO -> EnunciadO
	 */

	log_debug(logger, "[CATCH] Enviando CAUGHT");
	enviarMensajeBroker(CAUGHT, mensajeRecibido->idCorrelativo, 3,
			(void*) "asd");
	log_debug(logger, "[CATCH] CAUGHT enviado");
}

void procesarGET(mensajeRecibido * mensajeRecibido) {

	log_debug(logger, "[GET] Procesando");
	/*
	 * TODO -> EnunciadO
	 */

	log_debug(logger, "[GET] Enviando LOCALIZED");
	enviarMensajeBroker(LOCALIZED, mensajeRecibido->idCorrelativo, 3,
			(void*) "asd");
	log_debug(logger, "[GET] LOCALIZED enviado");
}

int enviarMensajeBroker(cola colaDestino, uint32_t idCorrelativo,
		uint32_t sizeMensaje, void * mensaje) {
	int socketBroker = crearConexionCliente(ipServidor, puertoServidor);
	if (socketBroker < 0) {
		log_error(logger, "No se pudo establecer la conexión con el broker");
		//TODO - Implementar rutina de desconexión de todos los hilos, intento de reconexión, etc
		return socketBroker;
	}
	enviarMensajeABroker(socketBroker, colaDestino, idCorrelativo, sizeMensaje,
			mensaje);
	uint32_t respuestaBroker;
	recv(socketBroker, &respuestaBroker, sizeof(uint32_t), MSG_WAITALL);
	close(socketBroker);
	return respuestaBroker;
}

bool existeElArchivo(char * rutaArchivo) {
	int fd = open(rutaArchivo, O_RDONLY);

	if (fd < 0) {
		return false;
	}

	close(fd);
	return true;
}

void obtenerParametrosDelFS(char * rutaMetadata) {
	t_config * metadata;
	metadata = config_create(rutaMetadata);
	tamanioBloque = config_get_int_value(metadata, "BLOCK_SIZE");
	cantidadDeBloques = config_get_int_value(metadata, "BLOCKS");
	config_destroy(metadata);
}

/*char * mapearArchivo(char * rutaArchivo){
 int fd = open(rutaArchivo,O_RDWR);
 struct stat sb;
 fstat(fd,&sb);
 char * mappedFile = mmap(NULL,sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,fd,0);
 return mappedFile;
 }*/

void inicializarFileSystem() {

	char * rutaMetadata = cadenasConcatenadas(puntoDeMontaje,
			"/Metadata/metadata.bin");
	char * rutaBitmap = cadenasConcatenadas(puntoDeMontaje,
			"/Metadata/bitmap.bin");
	char * rutaBlocks = cadenasConcatenadas(puntoDeMontaje, "/Blocks/");

	if (!existeElArchivo(rutaMetadata)) {
		log_error(logger,
				"No se encontró el archivo metadata en el punto de montaje. El proceso GameCard no puede continuar");
		exit(0);
	}

	obtenerParametrosDelFS(rutaMetadata);

	if (existeElArchivo(rutaBitmap)) {
		int fd = open(rutaBitmap, O_RDWR);
		struct stat sb;
		fstat(fd, &sb);
		bitmap = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
				0);
		bitarrayBloques = bitarray_create_with_mode(bitmap, sb.st_size,
				MSB_FIRST);
	} else {
		int fd = open(rutaBitmap, O_RDWR | O_CREAT, 0777);
		ftruncate(fd, cantidadDeBloques / 8);
		struct stat sb;
		fstat(fd, &sb);
		bitmap = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
				0);
		bitarrayBloques = bitarray_create_with_mode(bitmap, sb.st_size,
				MSB_FIRST);
		//msync(); ver como es la firma de esto
		for (int i = 0; i < cantidadDeBloques; i++) {
			bitarray_clean_bit(bitarrayBloques, i);

			char * rutaBloque;
			asprintf(&rutaBloque, "%s%d%s", rutaBlocks, i, ".bin");
			int nuevoBloque = open(rutaBloque, O_RDWR | O_CREAT, 0777);
			//TODO
			//¿Tiene sentido hacer un truncate con el tamaño de bloque?
			close(nuevoBloque);
			free(rutaBloque);
		}
	}
	free(rutaBitmap);
	free(rutaMetadata);
	free(rutaBlocks);
}

char * cadenasConcatenadas(char * cadena1, char * cadena2) {
	char * cadenaFinal = string_new();
	string_append(&cadenaFinal, cadena1);
	string_append(&cadenaFinal, cadena2);
	return cadenaFinal;
}

int main() {
	//Se setean todos los datos
	inicializarVariablesGlobales();
	log_info(logger, "Se ha iniciado el cliente GameCard\n");

	inicializarFileSystem();

	crearConexionBroker();
	crearConexionGameBoy();

	log_info(logger, "El proceso gamecard finalizó su ejecución\n");
	cerrarConexiones();
	destruirVariablesGlobales();

	return 0;
}
