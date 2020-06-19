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
   char * cad = malloc(strlen(cadena)+1);
   memcpy(cad,cadena,strlen(cadena));
   memcpy(cad+strlen(cadena),"\0",1);
   return cad;
}

char * posicionComoChar(uint32_t posx, uint32_t posy){
	char * cad = NULL;
	asprintf(cad,"%d-%d",posx,posy);
	return cad;
}

void procesarNEW(mensajeRecibido * mensajeRecibido){

	log_debug(logger, "[NEW] Procesando");
	/*
	 * TODO -> EnunciadP
	 */
	mensajeNew * mensaje = desarmarMensajeNEW(mensajeRecibido);


	char * pokemonConFinDeCadena = agregarFinDeCadena(mensaje->pokemon);
	char * posicionComoCadena = posicionComoChar(mensaje->posicionX,mensaje->posicionY);
	char * rutaPokemon = NULL;
	asprintf(rutaPokemon,"%s%s%s%s",puntoDeMontaje,"Files/",pokemonConFinDeCadena);
	char * rutaMetadataPokemon = NULL;
	asprintf(rutaMetadataPokemon,"%s%s%s%s",puntoDeMontaje,"Files/",pokemonConFinDeCadena,"/Metadata.bin");

	int pokemonFD = open(rutaMetadataPokemon,O_RDWR);

	if(pokemonFD<0){
		mkdir(rutaPokemon, 0777);
		pokemonFD = open(rutaMetadataPokemon, O_RDWR | O_CREAT,0777);
		//Inicializar archivo nuevo
	}
	else{

		t_config * archivo;
		archivo=config_create(rutaMetadataPokemon);
		config_set_value(archivo,"OPEN","Y");
		char ** bloques = config_get_array_value(config,"BLOCKS");

		int i=0;
		bool lasPosicionesYaExistian = false;
		while(bloques[i]!=NULL){
		    t_config * bloque;

		    char * rutaBloque = NULL;
		    asprintf(rutaBloque,"%s%s%s%s",puntoDeMontaje,"Blocks/",bloque[i],".bin");
			bloque=config_create(rutaBloque);
			if(config_has_property(bloque,posicionComoCadena))
			{
				int cantidadActual = config_get_int_value(bloque,posicionComoCadena);
				cantidadActual+=mensaje->cantPokemon;
				char * cantidadComoString;
				asprintf(cantidadComoString,"%d",cantidadActual);
				config_set_value(bloque,posicionComoCadena,cantidadComoString);
				lasPosicionesYaExistian = true;
				free(cantidadComoString);
				break;
			}
			i++;
		}
		if(!lasPosicionesYaExistian){
			/* TODO
			 *  - Ver si hay algun bloque con suficiente espacio libre.
			 *  - Sino, asignar un nuevo bloque
			 */
		}


	}


	/*
    struct stat sb;
    fstat(pokemonFD,&sb);
	char * mappedFile = mmap(NULL,sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,pokemonFD,0);
	log_info(logger,"%s",mappedFile);
	*/

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

void inicializarFileSystem(){

	char * rutaMetadata = cadenasConcatenadas(puntoDeMontaje,"Metadata/metadata.bin");
	int fd = open(rutaMetadata,O_RDONLY);
	//El archivo Metadata.bin siempre existe, si no se puede leer, el programa no puede ejecutarse.
	if(fd<0){
		log_error(logger,"No se pudo leer el archivo metadata.bin en el punto de montaje");
		exit(0);
	}
	struct stat sb;
	fstat(fd,&sb);
	log_info(logger,"File descriptor archivo: %d", fd);
	log_info(logger, "Tamaño del archivo leido: %d", sb.st_size);
	free(rutaMetadata);

	/* TODO
	 * 1) Cargar los datos del Metadata.bin en variables globales para su uso, ya que no van a cambiar durante la ejecución.
	 * 2) Chequear si existe el archivo Bitmap.bin en la ruta correspondiente.
	 *      |- Si existe, se continua la ejecución de las funciones del GameCard (esperar mensajes).
	 *      |- Si no existe, se crea la cantidad de bloques que indica el archivo Metadata.bin, y el archivo Bitmap.bin
	 *         usando bitarray de las commons.
	 *
	 *    Bitmap.bin es un archivo binario, y hay que leerlo/escribirlo como tal.
	 */


}

char * cadenasConcatenadas(char * cadena1, char * cadena2){
	char * cadenaFinal = malloc(strlen(cadena1)+strlen(cadena2)+1);
	strcpy(cadenaFinal,cadena1);
	strcat(cadenaFinal,cadena2);
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
