#include "Game-Card.h"


void inicializarVariablesGlobales() {
	config = config_create("gamecard.config");
	logger = log_create("gamecard_logs", "GameCard", 1, LOG_LEVEL_TRACE);

	ipServidor=malloc(strlen(config_get_string_value(config,"IP_BROKER"))+1);
	strcpy(ipServidor,config_get_string_value(config,"IP_BROKER"));
	puertoServidor=malloc(strlen(config_get_string_value(config,"PUERTO_BROKER"))+1);
	strcpy(puertoServidor,config_get_string_value(config,"PUERTO_BROKER"));

	puntoDeMontaje=malloc(strlen(config_get_string_value(config,"PUNTO_MONTAJE_TALLGRASS"))+1);
	strcpy(puntoDeMontaje,config_get_string_value(config,"PUNTO_MONTAJE_TALLGRASS"));

	idProceso=-1;
	statusConexionBroker=0;
}

void destruirVariablesGlobales() {
	free(ipServidor);
	free(puertoServidor);
	log_destroy(logger);
	config_destroy(config);
}

mensajeNew * desarmarMensajeNEW(mensajeRecibido * mensajeRecibido){
	mensajeNew * mensaje = malloc(sizeof(mensajeNew));
	int offset = 0;
	memcpy(&mensaje->longPokemon,mensajeRecibido->mensaje+offset,sizeof(uint32_t));
    offset+=sizeof(uint32_t);

    mensaje->pokemon=malloc(mensaje->longPokemon);

    memcpy(mensaje->pokemon,mensajeRecibido->mensaje+offset,mensaje->longPokemon);
    offset+=mensaje->longPokemon;

    memcpy(&mensaje->posicionX,mensajeRecibido->mensaje+offset,sizeof(uint32_t));
    offset+=sizeof(uint32_t);

    memcpy(&mensaje->posicionY,mensajeRecibido->mensaje+offset,sizeof(uint32_t));
    offset+=sizeof(uint32_t);

    memcpy(&mensaje->cantPokemon,mensajeRecibido->mensaje+offset,sizeof(uint32_t));

    return mensaje;
}

mensajeCatch * desarmarMensajeCATCH(mensajeRecibido * mensajeRecibido){
	mensajeCatch * mensaje = malloc(sizeof(mensajeCatch));
	int offset = 0;
	memcpy(&mensaje->longPokemon,mensajeRecibido->mensaje+offset,sizeof(uint32_t));
    offset+=sizeof(uint32_t);

    mensaje->pokemon=malloc(mensaje->longPokemon);

    memcpy(mensaje->pokemon,mensajeRecibido->mensaje+offset,mensaje->longPokemon);
    offset+=mensaje->longPokemon;

    memcpy(&mensaje->posicionX,mensajeRecibido->mensaje+offset,sizeof(uint32_t));
    offset+=sizeof(uint32_t);

    memcpy(&mensaje->posicionY,mensajeRecibido->mensaje+offset,sizeof(uint32_t));

    return mensaje;
}

mensajeGet * desarmarMensajeGET(mensajeRecibido * mensajeRecibido){
	mensajeGet * mensaje = malloc(sizeof(mensajeGet));
	int offset = 0;
	memcpy(&mensaje->longPokemon,mensajeRecibido->mensaje+offset,sizeof(uint32_t));
    offset+=sizeof(uint32_t);

    mensaje->pokemon=malloc(mensaje->longPokemon);

    memcpy(mensaje->pokemon,mensajeRecibido->mensaje+offset,mensaje->longPokemon);

    return mensaje;
}

char * agregarFinDeCadena (char * cadena){
   char * cad = string_new();
   string_append(&cad,cadena);
   string_append(&cad,"\0");
   return cad;
}

char * posicionComoChar(uint32_t posx, uint32_t posy){
	char * cad;
	asprintf(&cad,"%d-%d",posx,posy);
	return cad;
}

void inicializarArchivoMetadata(char * rutaArchivo){
	t_config * metadata;
	metadata = config_create(rutaArchivo);
	config_set_value(metadata,"OPEN","N");
	config_set_value(metadata,"BLOCKS","[]");
	config_set_value(metadata,"SIZE","0");
	config_set_value(metadata,"DIRECTORY","N");
	config_save(metadata);
	config_destroy(metadata);
}

char * aniadirBloqueAVectorString(int numeroBloque, char ** bloquesActuales){

	char * cadenaAGuardar = string_new();
	int i=0;
	string_append(&cadenaAGuardar,"[");
	while(bloquesActuales[i]!=NULL){
		string_append(&cadenaAGuardar,bloquesActuales[i]);
		string_append(&cadenaAGuardar,",");
	}
	string_append(&cadenaAGuardar,string_itoa(numeroBloque));
	string_append(&cadenaAGuardar,"]");
	return cadenaAGuardar;
}

int asignarBloqueAArchivo(char * rutaMetadataArchivo){
	t_config * metadataArchivo;
	metadataArchivo = config_create(rutaMetadataArchivo);
	int indexBloqueLibre = buscarBloqueLibre();
	char ** bloquesActuales = config_get_array_value(metadataArchivo,"BLOCKS");
	char * cadenaAGuardar = aniadirBloqueAVectorString(indexBloqueLibre,bloquesActuales);
	bitarray_set_bit(bitarrayBloques,indexBloqueLibre);
	sync();
	config_set_value(metadataArchivo,"BLOCKS",cadenaAGuardar);
	config_save(metadataArchivo);
	config_destroy(metadataArchivo);
	free(cadenaAGuardar);
	return indexBloqueLibre;
}

int buscarBloqueLibre(){
	for(int i=0; i<cantidadDeBloques; i++){
	  int estadoBloqueActual = bitarray_test_bit(bitarrayBloques, i);
		 if(estadoBloqueActual==0){
			 return i;
		 }
	}
	return -1;
}

void crearNuevoPokemon(char * rutaMetadataPokemon, mensajeNew * datosDelPokemon){
	int pokemonFD = open(rutaMetadataPokemon, O_RDWR | O_CREAT, 0777);
	close(pokemonFD);
	inicializarArchivoMetadata(rutaMetadataPokemon);
	asignarBloqueAArchivo(rutaMetadataPokemon);

}

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
		/* TODO
		 *  - Ver si hay algun bloque con suficiente espacio libre.
		 *  - Sino, asignar un nuevo bloque
		 */
	}
}

void procesarNEW(mensajeRecibido * mensajeRecibido){

	log_debug(logger, "[NEW] Procesando");

	mensajeNew * mensaje = desarmarMensajeNEW(mensajeRecibido);


	char * pokemonConFinDeCadena = agregarFinDeCadena(mensaje->pokemon);
	char * posicionComoCadena = posicionComoChar(mensaje->posicionX,mensaje->posicionY);
	char * rutaPokemon;
	asprintf(&rutaPokemon,"%s%s%s",puntoDeMontaje,"/Files/",pokemonConFinDeCadena); //Armo la ruta del pokemon
	char * rutaMetadataPokemon;
	asprintf(&rutaMetadataPokemon,"%s%s",rutaPokemon,"/metadata.bin");			   //Armo la ruta del metadata del pokemon


	if(!existeElArchivo(rutaMetadataPokemon)){									    //Si el archivo no existe, el pokemon no existe y tengo que crearlo
		if(buscarBloqueLibre()==-1)
		{
			log_info(logger,"No se puede crear el nuevo pokemon: todos los bloques estan ocupados");
			return;
		}
		mkdir(rutaPokemon, 0777);
		crearNuevoPokemon(rutaMetadataPokemon, mensaje);
	}

	agregarPokemonsAPosicion(rutaMetadataPokemon, posicionComoCadena, mensaje);


	log_debug(logger, "[NEW] Enviando APPEARED");
	enviarMensajeBroker(APPEARED, mensajeRecibido->idCorrelativo, 3, (void*) "asd");
	log_debug(logger, "[NEW] APPEARED enviado");

	free(pokemonConFinDeCadena);
	free(rutaPokemon);
	free(rutaMetadataPokemon);
	free(posicionComoCadena);
	free(mensajeRecibido->mensaje);
	free(mensajeRecibido);
	free(mensaje->pokemon);
	free(mensaje);
}


void procesarCATCH(mensajeRecibido * mensajeRecibido){

	log_debug(logger, "[CATCH] Procesando");
	/*
	 * TODO -> EnunciadO
	 */

	log_debug(logger, "[CATCH] Enviando CAUGHT");
	enviarMensajeBroker(CAUGHT, mensajeRecibido->idCorrelativo, 3, (void*) "asd");
	log_debug(logger, "[CATCH] CAUGHT enviado");
}

void procesarGET(mensajeRecibido * mensajeRecibido){

	log_debug(logger, "[GET] Procesando");
	/*
	 * TODO -> EnunciadO
	 */

	log_debug(logger, "[GET] Enviando LOCALIZED");
	enviarMensajeBroker(LOCALIZED, mensajeRecibido->idCorrelativo, 3, (void*) "asd");
	log_debug(logger, "[GET] LOCALIZED enviado");
}

void enviarMensajeBroker(cola colaDestino, uint32_t idCorrelativo, uint32_t sizeMensaje, void * mensaje){
	int socketBroker = crearConexionCliente(ipServidor, puertoServidor);
	enviarMensajeABroker(socketBroker, colaDestino, idCorrelativo, sizeMensaje, mensaje);
	close(socketBroker);
}

bool existeElArchivo(char * rutaArchivo){
	int fd = open(rutaArchivo,O_RDONLY);

	if(fd<0){
		close(fd);
		return false;
	}

	return true;
}

void obtenerParametrosDelFS(char * rutaMetadata){
	t_config * metadata;
	metadata = config_create(rutaMetadata);
	tamanioBloque = config_get_int_value(metadata,"BLOCK_SIZE");
	cantidadDeBloques = config_get_int_value(metadata,"BLOCKS");
	config_destroy(metadata);
}

char * mapearArchivo(char * rutaArchivo){
	int fd = open(rutaArchivo,O_RDWR);
	struct stat sb;
	fstat(fd,&sb);
	char * mappedFile = mmap(NULL,sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,fd,0);
	return mappedFile;
}

void inicializarFileSystem(){

	char * rutaMetadata = cadenasConcatenadas(puntoDeMontaje,"/Metadata/metadata.bin");
	char * rutaBitmap = cadenasConcatenadas(puntoDeMontaje,"/Metadata/bitmap.bin");
	char * rutaBlocks = cadenasConcatenadas(puntoDeMontaje,"/Blocks/");

	if(!existeElArchivo(rutaMetadata)){
		log_error(logger,"No se encontró el archivo metadata en el punto de montaje. El proceso GameCard no puede continuar");
		exit(0);
	}

	obtenerParametrosDelFS(rutaMetadata);

	if(existeElArchivo(rutaBitmap)){
		int fd = open(rutaBitmap,O_RDWR);
		struct stat sb;
		fstat(fd,&sb);
		bitmap = mmap(NULL,sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,fd,0);
		bitarrayBloques = bitarray_create_with_mode(bitmap, sb.st_size, MSB_FIRST);
	}
	else{
		int fd = open(rutaBitmap, O_RDWR | O_CREAT, 0777);
		ftruncate(fd,cantidadDeBloques/8);
		struct stat sb;
		fstat(fd,&sb);
		bitmap = mmap(NULL,sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,fd,0);
		bitarrayBloques = bitarray_create_with_mode(bitmap, sb.st_size, MSB_FIRST);
		sync();
		for(int i=0; i<cantidadDeBloques ; i++){
			bitarray_clean_bit(bitarrayBloques, i);

			char * rutaBloque;
			asprintf(&rutaBloque,"%s%d%s",rutaBlocks,i,".bin");
			int nuevoBloque = open(rutaBloque,O_RDWR | O_CREAT, 0777);
			//¿Tiene sentido hacer un truncate con el tamaño de bloque?
			close(nuevoBloque);
			free(rutaBloque);
		}
	}
	free(rutaBitmap);
	free(rutaMetadata);
	free(rutaBlocks);
}

char * cadenasConcatenadas(char * cadena1, char * cadena2){
	char * cadenaFinal = string_new();
	string_append(&cadenaFinal,cadena1);
	string_append(&cadenaFinal,cadena2);
	return cadenaFinal;
}

int main(){
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
