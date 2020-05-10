#include "Broker.h"

void * cacheBroker;
int CACHESIZE;

void inicializarCache() {
	/*
	 * La cache tiene que ser un espacio fijo, reservado al momento de ejecutar el broker. Tiene que ser un espacio tipo void
	 * porque no se sabe que se va a guardar ahi, pero tambien tiene que estar inicializado para poder diferenciar que espacios
	 * estan vacios. Hay que averiguar como hacer esto bien, puse un caracter para llenar.
	 */
	CACHESIZE = config_get_int_value(config, "TAMANO_MEMORIA");
	cacheBroker = malloc(CACHESIZE);
	memset(cacheBroker, 'f', CACHESIZE);
}

//----------------------- [CACHE] -------------------------//


int chequearSiAlcanza(int sizeMensaje, void * posicionActual,
		int memoriaRecorrida) {
	/* Chequea la disponibilidad de tantas posiciones contiguas como sea el tamaño del mensaje. Si en el proceso encuentra alguna
	 * ocupada retorna 0 (no alcanza el espacio), si termina sin encontrar ninguna ocupada retorna 1 (espacio suficiente).
	 */
	for (int offset = 1; offset < sizeMensaje; offset++) {
		if (memoriaRecorrida == CACHESIZE
				|| *(char *) (posicionActual + offset) == 'f') //LAZY EVALUATION POWA
			return 0;
		memoriaRecorrida++;
	}
	return 1;

}

void * buscarEspacio(int sizeMensaje, void *posicionInicial) {
	/* Busca secuencialmente algun byte marcado como libre, cuando lo encuentra llama a otra funcion para ver si alcanza el
	 * espacio para guardar el mensaje
	 *
	 */
	int espacioSuficiente = 0;
	int offset = 0;
	while (!espacioSuficiente) {
		if (offset == CACHESIZE)
			return NULL;

		if (*(char *) (posicionInicial + offset) == 'f') {
			espacioSuficiente = chequearSiAlcanza(sizeMensaje,
					posicionInicial + offset, offset);
		}
		offset++;

	}
	return posicionInicial + offset;
}


int tamanioDelMensaje(int offset, int *cacheExcedida) {
	/* Partiendo desde una posición ocupada, cuenta todas las posiciónes contiguas llenas para determinar el tamaño del mensaje
	 *
	 */
	int tamanio = 0;
	while (*(char *) (cacheBroker + offset) != 'f') {
		tamanio++;
		offset++;
		if (offset == CACHESIZE) {
			*cacheExcedida = 1;
			break;
		}
	}
	return tamanio;
}

void * buscarProximoMensaje(int offset, int * tamanio, int *cacheExcedida) {
	/* Partiendo desde una posición vacia, busca secuencialmente hasta encontrar una posición ocupada
	 *
	 */
	while (*(char *) (cacheBroker + offset) != 'f') {
		offset++;
		if (offset == CACHESIZE) {
			*cacheExcedida = 1;
			return NULL;
		}
	}
	*tamanio = tamanioDelMensaje(offset, cacheExcedida);

	return cacheBroker + offset;

}



void compactarMemoria() {
	/* Libera los "huecos" de la memoria cache, haciendo que todos los mensajes esten pegados uno detras del otro.
	 *
	 */
	int tamanio;
	int cacheExcedida = 0;
	void * posicion;
	for (int offset = 0; offset < CACHESIZE; offset++) {
		if (*(char *) (cacheBroker + offset) == 'f') {
			posicion = buscarProximoMensaje(offset, &tamanio, &cacheExcedida);
			if (cacheExcedida == 0) {
				memcpy(cacheBroker + offset, posicion, tamanio);
				memset(posicion, 'f', tamanio);
			} else {
				break;
			}
		}
	}
}


void eliminarMensaje() {
	//TODO
}

void cachearConBuddySystem(void * mensaje, int sizeMensaje){

}

/* Guarda el mensaje en memoria cache mediante algoritmo first fit
 * 1) Busca ponerlo en un espacio libre.
 * 2) Si no puede, compacta la memoria y vuelve a probar.
 * 3) Si no puede, borra el mensaje mas viejo y vuelve a probar.
 * 4) Si no puede, repite 2 y 3 hasta que lo mete.
 *
 * - El algoritmo deberia hacer una n cantidad de busquedas de espacio antes de compactar (??????????)
 * - Hay que validar el caso extremo en el que el mensaje que se quiere guardar no entre en toda cache, en esta función o en otra
 * - Hay que ver como poner las distintos criterios de victima
 */

void usarBestFit(){

}

void usarFirstFit(void * mensaje, int sizeMensaje){
	void * particion;
	particion = buscarEspacio(sizeMensaje, cacheBroker);
	while (particion == NULL) {
		compactarMemoria();
		particion = buscarEspacio(sizeMensaje, cacheBroker);
		if (particion == NULL) {
			eliminarMensaje();
			particion = buscarEspacio(sizeMensaje, cacheBroker);
		}
	}
	memcpy(particion, mensaje, sizeMensaje);
}

void cachearConParticionesDinamicas(void * mensaje, int sizeMensaje){
	if(strcmp("FF",config_get_string_value(config,"ALGORITMO_PARTICION_LIBRE"))==0)
       usarFirstFit(mensaje,sizeMensaje);
	else
       usarBestFit(mensaje,sizeMensaje);
}

void cachearMensaje(void * mensaje, int sizeMensaje){
   if(strcmp("BD",config_get_string_value(config, "ALGORITMO_MEMORIA"))==0)
	   cachearConBuddySystem(mensaje, sizeMensaje);
   else
	   cachearConParticionesDinamicas(mensaje, sizeMensaje);
}

void enviarMensajesCacheados(int socketSuscriptor, int codSuscripcion) {
	//TODO

}








